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

VOID AsyncComEvtIoRead(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t Length)
{
	struct asynccom_port *port = 0;
	NTSTATUS status = STATUS_SUCCESS;

	if (Length == 0) {
		WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, Length);
		return;
	}

	port = GetPortContext(WdfIoQueueGetDevice(Queue));
	if (!port) {
		WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);
		return;
	}

	status = WdfRequestForwardToIoQueue(Request, port->read_queue2);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_READ, "%s: WdfRequestForwardToIoQueue failed: %X", __FUNCTION__, status);
		WdfRequestComplete(Request, status);
		return;
	}
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_READ, "%s: Queueing read for %d bytes.", __FUNCTION__, Length);
	WdfDpcEnqueue(port->process_read_dpc);
}

VOID AsyncComEvtIoWrite(_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request, _In_ size_t Length)
{
	NTSTATUS status = STATUS_SUCCESS;
	struct asynccom_port *port = 0;

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE, "%s: Entering.", __FUNCTION__);

	port = GetPortContext(WdfIoQueueGetDevice(Queue));
	if (Length == 0) {
		WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, Length);
		return;
	}

	status = WdfRequestForwardToIoQueue(Request, port->write_queue2);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE, "%s: WdfRequestForwardToIoQueue failed: %X", __FUNCTION__, status);
		WdfRequestComplete(Request, status);
		return;
	}

	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE, "%s: Queueing write for %d bytes.", __FUNCTION__, Length);
	WdfDpcEnqueue(port->process_write_dpc);
}

VOID AsyncComEvtIoStop(_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request, _In_ ULONG ActionFlags)
{
	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP, "%s: Entering.", __FUNCTION__);
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(ActionFlags);
	if (ActionFlags &  WdfRequestStopActionSuspend) {
		WdfRequestStopAcknowledge(Request, FALSE); // Don't requeue
	}
	else if (ActionFlags &  WdfRequestStopActionPurge) {
		WdfRequestCancelSentRequest(Request);
	}
	return;
}

void complete_current_read_request(struct asynccom_port *port)
{
	WDFREQUEST old_request = NULL;
	PREQUEST_CONTEXT context;

	old_request = port->current_read_request;
	port->current_read_request = NULL;

	// If there's a current request, complete it. This function only gets called when there's enough data
	// or when the current read request has timed out. So in any scenario, this should work fine.
	if (old_request)
	{
		// Kill any running timers.
		WdfTimerStop(port->read_request_total_timer, FALSE);
		WdfTimerStop(port->read_request_interval_timer, FALSE);

		context = GetRequestContext(old_request);
		// Technically, STATUS_TIMEOUT is inside the realm of 'success' for NT_SUCCESS.
		// This is just to make sure I don't try to throw data into a bad pointer.
		if (NT_SUCCESS(context->status)) context->information = asynccom_frame_remove_data(port->istream, (unsigned char *)context->data_buffer, context->length);
		TraceEvents(TRACE_LEVEL_INFORMATION, DBG_READ, "%s: completing read with %X, info: %d", __FUNCTION__, context->status, (int)context->information);
		WdfRequestCompleteWithInformation(old_request, context->status, context->information);
	}

	return;
}

void complete_current_write_request(struct asynccom_port *port)
{
	WDFREQUEST old_request = NULL;
	PREQUEST_CONTEXT context;

	old_request = port->current_write_request;
	port->current_write_request = NULL;

	if (old_request)
	{
		context = GetRequestContext(old_request);
		if (NT_SUCCESS(context->status)) context->information = context->length;
		TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE, "%s: completing write with %X, info: %d", __FUNCTION__, context->status, (int)context->information);
		WdfRequestCompleteWithInformation(old_request, context->status, context->information);
	}
	return;
}

void complete_current_wait_request(struct asynccom_port *port, NTSTATUS status, ULONG info, ULONG matches)
{
	WDFREQUEST old_request = NULL;
	PREQUEST_CONTEXT context;

	old_request = port->current_wait_request;
	port->current_wait_request = 0;
	port->current_mask_history = 0;

	if (old_request)
	{
		context = GetRequestContext(old_request);
		context->information = info;
		context->status = status;
		context->data_buffer = (PVOID)&matches;
		TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "%s: completing wait with %X, info: %d", __FUNCTION__, context->status, (ULONG)context->information);
		WdfRequestCompleteWithInformation(old_request, context->status, context->information);
	}
}

int get_next_read_request(struct asynccom_port *port)
{
	NTSTATUS status;
	PREQUEST_CONTEXT context;
	WDF_REQUEST_PARAMETERS params;

	// We already have a request! Tell them we have one.
	if (port->current_read_request) return 1;

	status = WdfIoQueueRetrieveNextRequest(port->read_queue2, &port->current_read_request);
	if (!NT_SUCCESS(status))
	{
		// Didn't work. Reset the read request, just in case.
		port->current_read_request = NULL;
		// Well, there are no more left, that's why we couldn't get another.
		if (status == STATUS_NO_MORE_ENTRIES) return 0;
		TraceEvents(TRACE_LEVEL_ERROR, DBG_READ, "%s: WdfIoQueueRetrieveNextRequest failed: %X", __FUNCTION__, status);
		return 0;
	}

	// Initialize the context for the new request.
	context = GetRequestContext(port->current_read_request);
	WDF_REQUEST_PARAMETERS_INIT(&params);
	WdfRequestGetParameters(port->current_read_request, &params);
	context->length = (unsigned)params.Parameters.Read.Length;
	context->information = 0;
	context->status = STATUS_UNSUCCESSFUL;
	status = WdfRequestRetrieveOutputBuffer(port->current_read_request, context->length, (PVOID*)&context->data_buffer, NULL);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_READ, "%s: WdfRequestRetrieveOutputBuffer failed %!STATUS!", __FUNCTION__, status);
		context->status = status;
		complete_current_read_request(port);
		return 0;
	}

	// If ReadIntervalTimeout, ReadTotalTimeoutMultiplier, and ReadTotalTimeoutConstant are all zero, read operations never time out.
	if (port->timeouts.ReadIntervalTimeout == 0 && port->timeouts.ReadTotalTimeoutConstant == 0 && port->timeouts.ReadTotalTimeoutMultiplier == 0) {
		// Just in case.
		WdfTimerStop(port->read_request_total_timer, FALSE);
		WdfTimerStop(port->read_request_interval_timer, FALSE);
		return 1;
	}

	//	If ReadIntervalTimeout is set to MAXULONG, and both ReadTotalTimeoutConstant and ReadTotalTimeoutMultiplier are zero, 
	// a read request completes immediately with the bytes that have already been received, even if no bytes have been received.
	if (port->timeouts.ReadIntervalTimeout == MAXULONG && port->timeouts.ReadTotalTimeoutConstant == 0 && port->timeouts.ReadTotalTimeoutMultiplier == 0) {
		context->status = STATUS_SUCCESS;
		complete_current_read_request(port);
		return 0;
	}

	// This should start a timer for N*multiplier + constant milliseconds, where N is the number of expected bytes.
	if ((port->timeouts.ReadTotalTimeoutConstant != 0 && port->timeouts.ReadTotalTimeoutConstant != MAXULONG)
		|| (port->timeouts.ReadTotalTimeoutMultiplier != 0 && port->timeouts.ReadTotalTimeoutMultiplier != MAXULONG))
		WdfTimerStart(port->read_request_total_timer, WDF_REL_TIMEOUT_IN_MS((context->length * port->timeouts.ReadTotalTimeoutMultiplier) + port->timeouts.ReadTotalTimeoutConstant));

	/* TODO:
	If both ReadIntervalTimeout and ReadTotalTimeoutMultiplier are set to MAXULONG, and ReadTotalTimeoutConstant is set to a value greater than zero and less than MAXULONG, a read request behaves as follows:
	If there are any bytes in the serial port's input buffer, the read request completes immediately with the bytes that are in the buffer and returns the STATUS_SUCCESS status code.
	If there are no bytes in the input buffer, the serial port waits until a byte arrives, and then immediately completes the read request with the one byte of data and returns the STATUS_SUCCESS status code.
	If no bytes arrive within the time specified by ReadTotalTimeoutConstant, the read request times out, sets the Information field of the I/O status block to zero, and returns the STATUS_TIMEOUT status code.
	*/

	// Interval timer isn't enabled until the first byte is received.
	// With that in mind, I feel no need to address the interval timer in this function.
	
	// 04/27/2018 I have learned that our serialfc-windows drivers use an interval timer
	// before the first byte, and it seems to be required for some terminal programs
	// to release the handle to the drivers, so I'm adding it.
	if ((port->timeouts.ReadIntervalTimeout !=0) && (port->timeouts.ReadIntervalTimeout != MAXULONG)) WdfTimerStart(port->read_request_interval_timer, WDF_REL_TIMEOUT_IN_MS(port->timeouts.ReadIntervalTimeout));

	return 1;
}

int get_next_write_request(struct asynccom_port *port)
{
	NTSTATUS status;
	PREQUEST_CONTEXT context;
	WDF_REQUEST_PARAMETERS params;

	// We already have a request! Tell them we have one.
	if (port->current_write_request) return 1;

	status = WdfIoQueueRetrieveNextRequest(port->write_queue2, &port->current_write_request);
	if (!NT_SUCCESS(status))
	{
		port->current_write_request = NULL;
		if (status == STATUS_NO_MORE_ENTRIES) return 0;
		TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE, "%s: WdfIoQueueRetrieveNextRequest failed: %X", __FUNCTION__, status);
		return 0;
	}

	// Initialize the context for the new request.
	context = GetRequestContext(port->current_write_request);
	WDF_REQUEST_PARAMETERS_INIT(&params);
	WdfRequestGetParameters(port->current_write_request, &params);
	context->length = (unsigned)params.Parameters.Write.Length;
	context->information = 0;
	context->status = STATUS_UNSUCCESSFUL;
	status = WdfRequestRetrieveInputBuffer(port->current_write_request, context->length, (PVOID*)&context->data_buffer, NULL);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE, "%s: WdfRequestRetrieveInputBuffer failed %!STATUS!", __FUNCTION__, status);
		context->status = status;
		complete_current_write_request(port);
		return 0;
	}

	return 1;
}

void AsynccomProcessRead(WDFDPC Dpc)
{
	struct asynccom_port *port = 0;
	PREQUEST_CONTEXT context = NULL;

	port = GetPortContext(WdfDpcGetParentObject(Dpc));
	if (!get_next_read_request(port)) return;

	if (asynccom_frame_is_empty(port->istream)) return;
	context = GetRequestContext(port->current_read_request);
	if (!context) return;
	if ((asynccom_frame_get_length(port->istream) >= context->length)) {
		context->status = STATUS_SUCCESS;
		complete_current_read_request(port);
	}

	return;
}

void AsynccomProcessWrite(WDFDPC Dpc)
{
	struct asynccom_port *port = 0;
	PREQUEST_CONTEXT context = NULL;

	port = GetPortContext(WdfDpcGetParentObject(Dpc));
	if (!get_next_write_request(port))
	{
		event_occurred(port, SERIAL_EV_TXEMPTY);
		return;
	}

	context = GetRequestContext(port->current_write_request);
	if (!context) return;
	context->status = asynccom_port_data_write(port, (unsigned char *)context->data_buffer, context->length);
	complete_current_write_request(port);
	WdfDpcEnqueue(port->process_write_dpc);
	return;
}

NTSTATUS asynccom_port_data_write(struct asynccom_port *port, const unsigned char *data, unsigned byte_count)
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

void asynccom_port_received_data(__in  WDFUSBPIPE Pipe, __in  WDFMEMORY Buffer, __in  size_t NumBytesTransferred, __in  WDFCONTEXT Context)
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

BOOLEAN FX3EvtReadFailed(WDFUSBPIPE Pipe, NTSTATUS Status, USBD_STATUS UsbdStatus)
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
	TraceEvents(TRACE_LEVEL_WARNING, DBG_READ, "%s: Timer expired!", __FUNCTION__);
	if (port->current_read_request) {
		TraceEvents(TRACE_LEVEL_WARNING, DBG_READ, "%s: Read still exists, timing out.", __FUNCTION__);
		context = GetRequestContext(port->current_read_request);
		context->status = STATUS_TIMEOUT;
		complete_current_read_request(port);
	}
}

NTSTATUS asynccom_port_purge(_In_ struct asynccom_port *port, ULONG mask) {


	if (mask & SERIAL_PURGE_TXABORT) {
		//WdfIoQueuePurgeSynchronously(port->write_queue);
		//WdfIoQueuePurgeSynchronously(port->write_queue2);
		WdfIoQueuePurge(port->write_queue, WDF_NO_EVENT_CALLBACK, WDF_NO_CONTEXT);
		WdfIoQueuePurge(port->write_queue2, WDF_NO_EVENT_CALLBACK, WDF_NO_CONTEXT);
		WdfIoQueueStart(port->write_queue);
		WdfIoQueueStart(port->write_queue2);
	}

	if (mask & SERIAL_PURGE_RXABORT) {
		//WdfIoQueuePurgeSynchronously(port->read_queue);
		//WdfIoQueuePurgeSynchronously(port->read_queue2);
		WdfIoQueuePurge(port->read_queue, WDF_NO_EVENT_CALLBACK, WDF_NO_CONTEXT);
		WdfIoQueuePurge(port->read_queue2, WDF_NO_EVENT_CALLBACK, WDF_NO_CONTEXT);
		WdfIoQueueStart(port->read_queue);
		WdfIoQueueStart(port->read_queue2);
	}

	if (mask & SERIAL_PURGE_RXCLEAR) {
		WdfSpinLockAcquire(port->istream_spinlock);
		asynccom_frame_clear(port->istream);
		WdfSpinLockRelease(port->istream_spinlock);
	}
	return STATUS_SUCCESS;
}

void basic_completion(_In_ WDFREQUEST Request, _In_ WDFIOTARGET Target, _In_ PWDF_REQUEST_COMPLETION_PARAMS CompletionParams, _In_ WDFCONTEXT Context)
{
	PWDF_USB_REQUEST_COMPLETION_PARAMS usb_completion_params;
	NTSTATUS    status;

	UNREFERENCED_PARAMETER(Target);
	UNREFERENCED_PARAMETER(Context);

	usb_completion_params = CompletionParams->Parameters.Usb.Completion;
	status = CompletionParams->IoStatus.Status;

	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: Read failed: Request status: 0x%x, UsbdStatus: 0x%x\n", __FUNCTION__, status, usb_completion_params->UsbdStatus);
	}
	WdfObjectDelete(Request);
	return;
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

void event_occurred(struct asynccom_port *port, ULONG event)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "%s: event: %d", __FUNCTION__, event);
	if (!port->current_mask_value) return;
	if (port->current_mask_value & event) port->current_mask_history |= event;
	if (port->current_wait_request && port->current_mask_history) complete_current_wait_request(port, STATUS_SUCCESS, sizeof(ULONG), port->current_mask_history);
}