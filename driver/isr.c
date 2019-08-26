/*
Copyright 2019 Commtech, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
copies of the Software, and to permit persons to whom the Software is 
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in 
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
THE SOFTWARE.
*/

#include <driver.h>
#include <port.h>
#include <frame.h>
#include <isr.h>


#if defined(EVENT_TRACING)
#include "isr.tmh"
#endif

#pragma warning(disable:4267)

NTSTATUS data_write(struct asynccom_port *port, const unsigned char *data, unsigned byte_count)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_OBJECT_ATTRIBUTES  attributes;
	WDFUSBPIPE pipe;
	WDFREQUEST write_request;
	WDFMEMORY write_memory;
	const unsigned char *outgoing_data = 0;
	unsigned char *reversed_data = 0, parsed_data[4096] = { 0 };
	size_t i = 0;

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE, "%s: Entering.", __FUNCTION__);

	return_val_if_untrue(port, STATUS_UNSUCCESSFUL);
	return_val_if_untrue(data, STATUS_UNSUCCESSFUL);
	return_val_if_untrue(byte_count > 0, STATUS_UNSUCCESSFUL);

	outgoing_data = data;
	pipe = port->data_write_pipe;

#ifdef __BIG_ENDIAN
	reversed_data = (unsigned char *)ExAllocatePoolWithTag(NonPagedPool, byte_count, 'ataD');

	for (i = 0; i < byte_count; i++)
		reversed_data[i] = data[byte_count - i - 1];

	outgoing_data = reversed_data;
#endif

	for (i = 0; i < byte_count; i++)
	{
		parsed_data[i * 2] = outgoing_data[i];
		parsed_data[(i * 2) + 1] = 0x00;
	}

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.ParentObject = pipe;
	status = WdfRequestCreate(&attributes, WdfUsbTargetPipeGetIoTarget(pipe), &write_request);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE, "%s: Cannot create new write request.\n", __FUNCTION__);
		WdfObjectDelete(write_request);
		return status;
	}
	attributes.ParentObject = write_request;
	status = WdfMemoryCreate(&attributes, NonPagedPool, 0, byte_count*2, &write_memory, NULL);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE, "%s: WdfMemoryCreate failed! status: 0x%x\n", __FUNCTION__, status);
		return status;
	}
	status = WdfMemoryCopyFromBuffer(write_memory, 0, (void *)parsed_data, byte_count*2);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE, "%s: Cannot copy buffer to memory.\n", __FUNCTION__);
		WdfObjectDelete(write_request);
		return status;
	}
	status = WdfUsbTargetPipeFormatRequestForWrite(pipe, write_request, write_memory, NULL);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE, "%s: Cannot format request for write.\n", __FUNCTION__);
		WdfObjectDelete(write_request);
		return status;
	}

	//WdfRequestSetCompletionRoutine(write_request, basic_completion, pipe);
	WdfSpinLockAcquire(port->ostream_spinlock);
	if (WdfRequestSend(write_request, WdfUsbTargetPipeGetIoTarget(pipe), WDF_NO_SEND_OPTIONS) == FALSE) {
		WdfSpinLockRelease(port->ostream_spinlock);
		status = WdfRequestGetStatus(write_request);
		WdfObjectDelete(write_request);
		return status;
	}
	WdfSpinLockRelease(port->ostream_spinlock);

	if (!NT_SUCCESS(status)) TraceEvents(TRACE_LEVEL_WARNING, DBG_WRITE, "%s: Error status: 0x%x.", __FUNCTION__, status);
	if (reversed_data) ExFreePoolWithTag(reversed_data, 'ataD');

	return status;
}

void data_received(__in  WDFUSBPIPE Pipe, __in  WDFMEMORY Buffer, __in  size_t NumBytesTransferred, __in  WDFCONTEXT Context)
{
	struct asynccom_port *port = 0;
	NTSTATUS status = STATUS_SUCCESS;
	unsigned char *read_buffer = 0;
	unsigned current_memory = 0, memory_cap = 0;
	static int rejected_last_stream = 0;
	size_t payload_size = 0, receive_length = 0, buffer_size = 0;

	UNREFERENCED_PARAMETER(Pipe);

	port = (PASYNCCOM_PORT)Context;
	return_if_untrue(port);
	return_if_untrue(NumBytesTransferred > 2);

	receive_length = NumBytesTransferred;

	if (port->current_read_request && port->timeouts.ReadIntervalTimeout != 0 && port->timeouts.ReadIntervalTimeout != MAXULONG) WdfTimerStart(port->read_request_interval_timer, WDF_REL_TIMEOUT_IN_MS(port->timeouts.ReadIntervalTimeout));
	if (receive_length % 2) TraceEvents(TRACE_LEVEL_ERROR, DBG_READ, "%s: receive_length not even: %d.", __FUNCTION__, receive_length);
	read_buffer = WdfMemoryGetBuffer(Buffer, &buffer_size);

	payload_size = (size_t)read_buffer[0];
	payload_size = (payload_size << 8) | (size_t)read_buffer[1];

	current_memory = asynccom_port_get_input_memory_usage(port);
	memory_cap = asynccom_port_get_input_memory_cap(port);

#ifdef __BIG_ENDIAN
#else
	{
		unsigned i = 0;
		unsigned char storage;

		for (i = 0; i < receive_length; i = i + 2) {
			storage = read_buffer[i];
			read_buffer[i] = read_buffer[i + 1];
			read_buffer[i + 1] = storage;
		}
	}
#endif

	if (payload_size + current_memory > memory_cap) {
		payload_size = memory_cap - current_memory;
		TraceEvents(TRACE_LEVEL_ERROR, DBG_READ, "%s: Payload too large, new payload size: %d.", __FUNCTION__, payload_size);
	}
	if (payload_size < 1) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_READ, "%s: No room left!", __FUNCTION__);
		return;
	}
	if (payload_size < receive_length - 1) memmove(read_buffer, read_buffer + 2, payload_size);
	else {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_READ, "%s: Payload larger than buffer: Payload: %d, Receive Length: %d.", __FUNCTION__, payload_size, receive_length);
		return;
	}

	WdfSpinLockAcquire(port->istream_spinlock);
	status = asynccom_frame_add_data(port->istream, read_buffer, (unsigned)payload_size);
	WdfSpinLockRelease(port->istream_spinlock);

	if (asynccom_port_get_input_memory_usage(port) >(asynccom_port_get_input_memory_cap(port)*.8)) event_occurred(port, SERIAL_EV_RX80FULL);
	event_occurred(port, SERIAL_EV_RXCHAR);

	if (status == FALSE) TraceEvents(TRACE_LEVEL_ERROR, DBG_READ, "%s: Failed to add data to istream.", __FUNCTION__);
	else TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE, "%s: Stream <= %i byte%s", __FUNCTION__, (int)payload_size, (payload_size == 1) ? " " : "s");
	rejected_last_stream = 0;
	WdfDpcEnqueue(port->process_read_dpc);
	return;
}

BOOLEAN data_received_failed(WDFUSBPIPE Pipe, NTSTATUS Status, USBD_STATUS UsbdStatus)
{
	UNREFERENCED_PARAMETER(Status);
	UNREFERENCED_PARAMETER(UsbdStatus);
	UNREFERENCED_PARAMETER(Pipe);

	TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: Data ContinuousReader failed - did you unplug?", __FUNCTION__);
	return TRUE;
}

void serial_read_timeout(IN WDFTIMER Timer)
{
	struct asynccom_port *port = 0;
	PREQUEST_CONTEXT context = 0;

	port = GetPortContext(WdfTimerGetParentObject(Timer));
	if (!port) return;
	WdfTimerStop(port->read_request_total_timer, FALSE);
	WdfTimerStop(port->read_request_interval_timer, FALSE);
	if (port->current_read_request) {
		context = GetRequestContext(port->current_read_request);
		context->information = asynccom_frame_remove_data(port->istream, (unsigned char *)context->data_buffer, context->length);
		if(clear_cancel_routine(port->current_read_request) != STATUS_CANCELLED)
		complete_current_request(port, STATUS_TIMEOUT, &port->current_read_request);
	}
}

void event_occurred(struct asynccom_port *port, ULONG event)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "%s: event: %d", __FUNCTION__, event);
	if (!port->current_mask_value) return;
	if (port->current_mask_value & event) port->current_mask_history |= event;
	if (port->current_wait_request && port->current_mask_history)
	{
		PREQUEST_CONTEXT req_context;
		req_context = GetRequestContext(port->current_wait_request);
		*(ULONG *)req_context->data_buffer |= port->current_mask_history;
		port->current_mask_history = 0;
		if (clear_cancel_routine(port->current_wait_request) != STATUS_CANCELLED)
			complete_current_request(port, STATUS_SUCCESS, &port->current_wait_request);
	}
}

void process_timeouts(struct asynccom_port *port)
{
	WDFREQUEST old_request = NULL;
	PREQUEST_CONTEXT context;
	PSERIAL_TIMEOUTS new_timeouts;
	PVOID buffer = 0;
	size_t buffer_size = 0;

	old_request = port->current_read_request;
	port->current_read_request = NULL;

	if (old_request)
	{
		context = GetRequestContext(old_request);
		context->status = WdfRequestRetrieveInputBuffer(old_request, sizeof(SERIAL_TIMEOUTS), &buffer, &buffer_size);
		if (NT_SUCCESS(context->status)) {
			new_timeouts = (PSERIAL_TIMEOUTS)buffer;
			port->timeouts.ReadIntervalTimeout = new_timeouts->ReadIntervalTimeout;
			port->timeouts.ReadTotalTimeoutMultiplier = new_timeouts->ReadTotalTimeoutMultiplier;
			port->timeouts.ReadTotalTimeoutConstant = new_timeouts->ReadTotalTimeoutConstant;
			port->timeouts.WriteTotalTimeoutConstant = new_timeouts->WriteTotalTimeoutConstant;
			port->timeouts.WriteTotalTimeoutMultiplier = new_timeouts->WriteTotalTimeoutMultiplier;
		}
		WdfRequestCompleteWithInformation(old_request, context->status, context->information);
	}
}

int get_next_request(struct asynccom_port *port, WDFQUEUE Queue, WDFREQUEST *Request, IN PFN_WDF_REQUEST_CANCEL CancelRoutine)
{
	NTSTATUS status;
	PREQUEST_CONTEXT context;
	UNREFERENCED_PARAMETER(CancelRoutine);

	if (*Request) return 1;
	status = WdfIoQueueRetrieveNextRequest(Queue, Request);
	if (!NT_SUCCESS(status))
	{
		*Request = NULL;
		if (status == STATUS_NO_MORE_ENTRIES) return 0;
		TraceEvents(TRACE_LEVEL_ERROR, DBG_READ, "%s: WdfIoQueueRetrieveNextRequest failed: %X", __FUNCTION__, status);
		return 0;
	}
	context = GetRequestContext(*Request);
	//set_cancel_routine(*Request, CancelRoutine);
	if (context->major_function == IRP_MJ_READ) 
	{
		WdfTimerStop(port->read_request_total_timer, FALSE);
		WdfTimerStop(port->read_request_interval_timer, FALSE);

		// Interval = 0, Constant = 0, Multiplier = 0
		// No timeouts are used - wait for data indefinitely.
		if (port->timeouts.ReadIntervalTimeout == 0 && port->timeouts.ReadTotalTimeoutConstant == 0 && port->timeouts.ReadTotalTimeoutMultiplier == 0) return 1;

		// Interval = MAX, Constant = 0, Multiplier = 0
		// Return immediately, even with nothing.
		if (port->timeouts.ReadIntervalTimeout == MAXULONG && port->timeouts.ReadTotalTimeoutConstant == 0 && port->timeouts.ReadTotalTimeoutMultiplier == 0) {
			context->information = asynccom_frame_remove_data(port->istream, (unsigned char *)context->data_buffer, context->length);
			if (clear_cancel_routine(port->current_read_request) != STATUS_CANCELLED)
				complete_current_request(port, STATUS_SUCCESS, &port->current_read_request);
			return 0;
		}

		// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/ntddser/ns-ntddser-_serial_timeouts
		// TODO:????
		// If both ReadTotalTimeoutMultiplier and ReadTotalTimeoutConstant are zero, 
		// and ReadIntervalTimeout is less than MAXULONG and greater than zero, a read 
		// operation times out only if the interval between a pair of consecutively 
		// received bytes exceeds ReadIntervalTimeout. If these three time-out values 
		// are used, and the serial port's input buffer is empty when a read request is 
		// sent to the port, this request never times out until after the port receives 
		// at least one byte of new data.


		if ((port->timeouts.ReadTotalTimeoutConstant != 0 && port->timeouts.ReadTotalTimeoutConstant != MAXULONG)
			|| (port->timeouts.ReadTotalTimeoutMultiplier != 0 && port->timeouts.ReadTotalTimeoutMultiplier != MAXULONG))
			WdfTimerStart(port->read_request_total_timer, WDF_REL_TIMEOUT_IN_MS((context->length * port->timeouts.ReadTotalTimeoutMultiplier) + port->timeouts.ReadTotalTimeoutConstant));

		if ((port->timeouts.ReadIntervalTimeout != 0) && (port->timeouts.ReadIntervalTimeout != MAXULONG))
			WdfTimerStart(port->read_request_interval_timer, WDF_REL_TIMEOUT_IN_MS(port->timeouts.ReadIntervalTimeout));
	}
	return 1;
}

void process_read(struct asynccom_port *port)
{
	PREQUEST_CONTEXT context;
	
	context = GetRequestContext(port->current_read_request);
	if (asynccom_frame_is_empty(port->istream)) return;
	if ((asynccom_frame_get_length(port->istream) >= context->length))
	{
		WdfTimerStop(port->read_request_total_timer, FALSE);
		WdfTimerStop(port->read_request_interval_timer, FALSE);
		context->information = asynccom_frame_remove_data(port->istream, (unsigned char *)context->data_buffer, context->length);
		if (clear_cancel_routine(port->current_read_request) != STATUS_CANCELLED)
			complete_current_request(port, STATUS_SUCCESS, &port->current_read_request);
	}
}

void complete_current_request(_In_ struct asynccom_port *port, _In_ NTSTATUS status_to_use, WDFREQUEST *current_request)
{
	WDFREQUEST old_request = NULL;
	PREQUEST_CONTEXT context;
	UNREFERENCED_PARAMETER(port);

	// Take the request so no one else messes with it.
	old_request = *current_request;
	*current_request = NULL;

	// Only play with the request if it actually exists.
	if (old_request)
	{
		DbgPrint("Completing the request at: %p.\n", old_request);
		context = GetRequestContext(old_request);
		context->status = status_to_use;
		WdfRequestCompleteWithInformation(old_request, context->status, context->information);
	}

}

void set_cancel_routine(IN WDFREQUEST Request, IN PFN_WDF_REQUEST_CANCEL CancelRoutine)
{
	PREQUEST_CONTEXT context;

	context = GetRequestContext(Request);
	WdfRequestMarkCancelable(Request, CancelRoutine);
	context->cancel_routine = CancelRoutine;
	context->cancelled = FALSE;
}

NTSTATUS clear_cancel_routine(IN WDFREQUEST Request)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PREQUEST_CONTEXT context;

	context = GetRequestContext(Request);
	if (context->cancel_routine)
	{
		status = WdfRequestUnmarkCancelable(Request);
		if (NT_SUCCESS(status)) context->cancel_routine = NULL;
	}
	return status;
}

void cancel_wait(IN WDFREQUEST Request)
{
	struct asynccom_port *port = 0;
	PREQUEST_CONTEXT context;

	port = GetPortContext(WdfIoQueueGetDevice(WdfRequestGetIoQueue(Request)));
	context = GetRequestContext(Request);
	context->cancelled = TRUE;
	complete_current_request(port, STATUS_CANCELLED, &port->current_wait_request);
}

void cancel_read(IN WDFREQUEST Request)
{
	struct asynccom_port *port = 0;
	PREQUEST_CONTEXT context;

	port = GetPortContext(WdfIoQueueGetDevice(WdfRequestGetIoQueue(Request)));
	context = GetRequestContext(Request);
	context->cancelled = TRUE;
	complete_current_request(port, STATUS_CANCELLED, &port->current_read_request);
}

void cancel_write(IN WDFREQUEST Request)
{
	struct asynccom_port *port = 0;
	PREQUEST_CONTEXT context;

	port = GetPortContext(WdfIoQueueGetDevice(WdfRequestGetIoQueue(Request)));
	context = GetRequestContext(Request);
	context->cancelled = TRUE;
	complete_current_request(port, STATUS_CANCELLED, &port->current_write_request);
}

void AsyncComProcessRead(WDFDPC Dpc)
{
	struct asynccom_port *port = 0;
	PREQUEST_CONTEXT context = NULL;

	port = GetPortContext(WdfDpcGetParentObject(Dpc));
	if (!get_next_request(port, port->read_queue, &port->current_read_request, cancel_read)) return;
	context = GetRequestContext(port->current_read_request);
	if (context->ioctl_code == IOCTL_SERIAL_SET_TIMEOUTS) process_timeouts(port);
	if (context->major_function == IRP_MJ_READ) process_read(port);
	if (!is_queue_empty(port->read_queue) && !port->current_read_request) WdfDpcEnqueue(port->process_read_dpc);
}

void AsyncComProcessWrite(WDFDPC Dpc)
{
	struct asynccom_port *port = 0;
	PREQUEST_CONTEXT context = NULL;

	port = GetPortContext(WdfDpcGetParentObject(Dpc));
	if (!get_next_request(port, port->write_queue, &port->current_write_request, cancel_write))
	{
		event_occurred(port, SERIAL_EV_TXEMPTY);
		return;
	}

	context = GetRequestContext(port->current_write_request);
	if (!context) return;
	context->status = data_write(port, (unsigned char *)context->data_buffer, context->length);
	if (NT_SUCCESS(context->status)) context->information = context->length;
	if (clear_cancel_routine(port->current_write_request) != STATUS_CANCELLED)
		complete_current_request(port, context->status, &port->current_write_request);
	if(!is_queue_empty(port->write_queue)) WdfDpcEnqueue(port->process_write_dpc);
}

void AsyncComEvtIoCancelOnQueue(_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request)
{
	PREQUEST_CONTEXT req_context;
	UNREFERENCED_PARAMETER(Queue);
	req_context = GetRequestContext(Request);

	req_context->status = STATUS_CANCELLED;
	req_context->information = 0;
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "%s: Trying to cancel a request..", __FUNCTION__);
	WdfRequestCompleteWithInformation(Request, req_context->status, req_context->information);
}

void AsyncComEvtIoRead(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t Length)
{
	struct asynccom_port *port = 0;
	NTSTATUS status = STATUS_SUCCESS;
	PREQUEST_CONTEXT context;

	if (Length == 0) {
		WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, Length);
		return;
	}

	port = GetPortContext(WdfIoQueueGetDevice(Queue));
	if (!port) {
		WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);
		return;
	}
	context = GetRequestContext(Request);
	context->length = Length;
	context->information = 0;
	context->status = STATUS_UNSUCCESSFUL;
	context->major_function = IRP_MJ_READ;
	status = WdfRequestRetrieveOutputBuffer(Request, context->length, (PVOID*)&context->data_buffer, NULL);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_READ, "%s: WdfRequestRetrieveOutputBuffer failed %!STATUS!", __FUNCTION__, status);
		context->status = status;
		WdfRequestComplete(Request, status);
		return;
	}
	DbgPrint("New read request at: %p.\n", Request);
	status = WdfRequestForwardToIoQueue(Request, port->read_queue);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_READ, "%s: WdfRequestForwardToIoQueue failed: %X", __FUNCTION__, status);
		WdfRequestComplete(Request, status);
		return;
	}
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_READ, "%s: Queueing read for %d bytes.", __FUNCTION__, Length);
	WdfDpcEnqueue(port->process_read_dpc);
}

void AsyncComEvtIoWrite(_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request, _In_ size_t Length)
{
	NTSTATUS status = STATUS_SUCCESS;
	struct asynccom_port *port = 0;
	PREQUEST_CONTEXT context;
	WDF_REQUEST_PARAMETERS params;

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE, "%s: Entering.", __FUNCTION__);

	port = GetPortContext(WdfIoQueueGetDevice(Queue));

	if (Length == 0) {
		WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, Length);
		return;
	}
	context = GetRequestContext(Request);
	context->major_function = IRP_MJ_WRITE;
	WDF_REQUEST_PARAMETERS_INIT(&params);
	WdfRequestGetParameters(Request, &params);
	context->length = (unsigned)params.Parameters.Write.Length;
	context->information = 0;
	status = WdfRequestRetrieveInputBuffer(Request, context->length, (PVOID*)&context->data_buffer, NULL);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE, "%s: WdfRequestRetrieveInputBuffer failed %!STATUS!", __FUNCTION__, status);
		WdfRequestComplete(Request, status);
		return;
	}
	DbgPrint("New write request at: %p.\n", Request);
	status = WdfRequestForwardToIoQueue(Request, port->write_queue);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE, "%s: WdfRequestForwardToIoQueue failed: %X", __FUNCTION__, status);
		WdfRequestComplete(Request, status);
		return;
	}

	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE, "%s: Queueing write for %d bytes.", __FUNCTION__, Length);
	WdfDpcEnqueue(port->process_write_dpc);
}

void AsyncComEvtIoStop(_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request, _In_ ULONG ActionFlags)
{
	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP, "%s: Entering.", __FUNCTION__);
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(ActionFlags);
	PREQUEST_CONTEXT context;

	if (!Request) return;
	if (ActionFlags &  WdfRequestStopActionSuspend) {
		WdfRequestStopAcknowledge(Request, FALSE); // Don't requeue
	}
	else if (ActionFlags &  WdfRequestStopActionPurge) {
		WdfRequestCancelSentRequest(Request);
	}
	else
	{
		context = GetRequestContext(Request);
		context->status = STATUS_CANCELLED;
		context->information = 0;
		WdfRequestUnmarkCancelable(Request);
		DbgPrint("%s: Canceling a request..\n", __FUNCTION__);
		WdfRequestCompleteWithInformation(Request, context->status, context->information);
	}

	return;
}