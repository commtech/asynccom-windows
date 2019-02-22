/*
	Copyright (C) 2018  Commtech, Inc.

	This file is part of asynccom-windows.

	asynccom-windows is free software: you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	asynccom-windows is distributed in the hope that it will be useful, but WITHOUT
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
	FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
	more details.

	You should have received a copy of the GNU General Public License along
	with asynccom-windows.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <driver.h>
#include <port.h>
#include <utils.h>
#include <frame.h>
#include <isr.h>

#if defined(EVENT_TRACING)
#include "port.tmh"
#endif

VOID AsyncComEvtIoDeviceControl(_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request, _In_ size_t OutputBufferLength, _In_ size_t InputBufferLength, _In_ ULONG IoControlCode)
{
    WDFDEVICE           device;
    BOOLEAN             request_pending = FALSE;
	NTSTATUS            status = STATUS_INVALID_DEVICE_REQUEST; 
	struct asynccom_port	*port = 0;
	size_t				bytes_returned = 0;
	PVOID				buffer = 0;
	size_t				buffer_size = 0;
		
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);


	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL, "%s: Entering.\n", __FUNCTION__);
    device = WdfIoQueueGetDevice(Queue);
	port = GetPortContext(device);

    switch(IoControlCode) {
	case IOCTL_SERIAL_SET_BAUD_RATE: { 
		ULONG desired_baud = 0;
		SHORT required_divisor = 0;

		status = WdfRequestRetrieveInputBuffer(Request, sizeof(SERIAL_BAUD_RATE), &buffer, &buffer_size);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveInputBuffer failed %!STATUS!", status);
			break;
		}
		desired_baud = ((PSERIAL_BAUD_RATE)(buffer))->BaudRate;
		//if (port->fixed_baud_rate != -1) desired_baud = port->fixed_baud_rate;
		status = calculate_divisor(port->current_clock_frequency, port->current_sample_rate, desired_baud, &required_divisor);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "calculate_divisor failed %!STATUS!", status);
			break;
		}
		status = asynccom_port_set_divisor(port, required_divisor);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "set_divisor failed %!STATUS!", status);
			break;
		}
		port->current_baud = desired_baud;
		break; 
	}
	case IOCTL_SERIAL_GET_BAUD_RATE: { 
		PSERIAL_BAUD_RATE baud_rate;
		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(SERIAL_BAUD_RATE), &buffer, &buffer_size);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveOutputBuffer failed %!STATUS!", status);
			break;
		}
		baud_rate = (PSERIAL_BAUD_RATE)buffer;
		baud_rate->BaudRate = port->current_baud;
		bytes_returned = sizeof(SERIAL_BAUD_RATE);
		break; 
	}
	case IOCTL_SERIAL_GET_MODEM_CONTROL: {
		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(ULONG), &buffer, &buffer_size);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveOutputBuffer failed %!STATUS!", status);
			break;
		}
		*((PUINT32)buffer) = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, MCR_OFFSET);
		bytes_returned = sizeof(UINT32);
		break;
	}
	case IOCTL_SERIAL_SET_MODEM_CONTROL: { 
		status = WdfRequestRetrieveInputBuffer(Request, sizeof(ULONG), &buffer, &buffer_size);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveInputBuffer failed %!STATUS!", status);
			break;
		}
		status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, MCR_OFFSET, *((UCHAR *)(buffer)));
		break; 
	}
	case IOCTL_SERIAL_SET_FIFO_CONTROL: {		
		status = WdfRequestRetrieveInputBuffer(Request, sizeof(ULONG), &buffer, &buffer_size);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveInputBuffer failed %!STATUS!", status);
			break;
		}
		status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, FCR_OFFSET, *((UCHAR *)(buffer)));
		break;  
	}
	case IOCTL_SERIAL_SET_LINE_CONTROL: { 
		PSERIAL_LINE_CONTROL line_control;
        unsigned char line_data = 0, line_stop = 0, line_parity = 0;
		unsigned char mask = 0xff;
        UINT32 new_lcr = 0, current_lcr = 0;

		status = WdfRequestRetrieveInputBuffer(Request, sizeof(SERIAL_LINE_CONTROL), &buffer, &buffer_size);
		if (!NT_SUCCESS(status))
		{
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveInputBuffer failed %!STATUS!", status);
			break;
		}
		line_control = (PSERIAL_LINE_CONTROL)buffer;
		switch (line_control->WordLength) {
		case 5: {
			line_data = SERIAL_5_DATA;
            new_lcr |= 0x00;
			mask = 0x1f;
			break;
		}
		case 6: {
			line_data = SERIAL_6_DATA;
            new_lcr |= 0x01;
			mask = 0x3f;
			break;
		}
		case 7: {
			line_data = SERIAL_7_DATA;
            new_lcr |= 0x02;
			mask = 0x7f;
			break;
		}
		case 8: {
			line_data = SERIAL_8_DATA;
            new_lcr |= 0x03;
			break;
		}
		default: {
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		}
		if (status == STATUS_INVALID_PARAMETER) break;

		switch (line_control->Parity)
		{
		case NO_PARITY: {
			line_parity = SERIAL_NONE_PARITY;
            new_lcr |= 0x00;
			break;
		}
		case EVEN_PARITY: {
			line_parity = SERIAL_EVEN_PARITY;
            new_lcr |= 0x18;
			break;
		}
		case ODD_PARITY: {
			line_parity = SERIAL_ODD_PARITY;
            new_lcr |= 0x08;
			break;
		}
		case MARK_PARITY: {
			line_parity = SERIAL_MARK_PARITY;
            new_lcr |= 0x28;
			break;
		}
        case SPACE_PARITY: {
            line_parity = SERIAL_SPACE_PARITY;
            new_lcr |= 0x38;
            break;
        }
		default: {
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		}
		if (status == STATUS_INVALID_PARAMETER) break;

		switch (line_control->StopBits) {
		case STOP_BIT_1: {
			line_stop = SERIAL_1_STOP;
            new_lcr |= 0x00;
			break;
		}
		case STOP_BITS_1_5: {
			if (line_data != SERIAL_5_DATA) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			line_stop = SERIAL_1_5_STOP;
            new_lcr |= 0x04;
			break;
		}
		case STOP_BITS_2: {
			if (line_data == SERIAL_5_DATA) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			line_stop = SERIAL_2_STOP;
            new_lcr |= 0x04;
			break;
		}
		default: {
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		}
		port->line_control = (unsigned char)((port->line_control & SERIAL_LCR_BREAK) | (line_data | line_parity | line_stop));
		port->valid_data_mask = mask;
        current_lcr = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET);
        current_lcr &= 0xc0;
        new_lcr |= current_lcr;
        status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET, new_lcr);
		break; 
	}
	case IOCTL_SERIAL_GET_LINE_CONTROL: {
		PSERIAL_LINE_CONTROL line_control;
        UINT32 current_lcr = 0;

		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(SERIAL_LINE_CONTROL), &buffer, &buffer_size);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveOutputBuffer failed %!STATUS!", status);
			break;
		}
		line_control = (PSERIAL_LINE_CONTROL)buffer;
		RtlZeroMemory(buffer, OutputBufferLength);

        current_lcr = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET);

        if ((current_lcr & 0x03) == 0x03) line_control->WordLength = 8;
        else if ((current_lcr & 0x03) == 0x02) line_control->WordLength = 7;
        else if ((current_lcr & 0x03) == 0x01) line_control->WordLength = 6;
        else line_control->WordLength = 5;

        if ((current_lcr & 0x38) == 0x38) line_control->Parity = SPACE_PARITY;
        else if ((current_lcr & 0x38) == 0x28) line_control->Parity = MARK_PARITY;
        else if ((current_lcr & 0x38) == 0x18) line_control->Parity = EVEN_PARITY;
        else if ((current_lcr & 0x38) == 0x08) line_control->Parity = ODD_PARITY;
        else line_control->Parity = NO_PARITY;

        if ((current_lcr & 0x04) == 0x04) {
            if (line_control->WordLength == 5) line_control->StopBits = STOP_BITS_1_5;
            else line_control->StopBits = STOP_BITS_2;
        }
        else line_control->StopBits = STOP_BIT_1;

		bytes_returned = sizeof(SERIAL_LINE_CONTROL); // SERIAL_LINE_CONTROL
		break; 
	}
	case IOCTL_SERIAL_SET_TIMEOUTS: { 
		PSERIAL_TIMEOUTS new_timeouts;

		status = WdfRequestRetrieveInputBuffer(Request, sizeof(SERIAL_TIMEOUTS), &buffer, &buffer_size);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "IOCTL_SERIAL_SET_TIMEOUTS: WdfRequestRetrieveInputBuffer failed %!STATUS!", status);
			break;
		}
		new_timeouts = (PSERIAL_TIMEOUTS)buffer;

		if ((new_timeouts->ReadIntervalTimeout == MAXULONG) &&
			(new_timeouts->ReadTotalTimeoutMultiplier == MAXULONG) &&
			(new_timeouts->ReadTotalTimeoutConstant == MAXULONG)) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		port->timeouts.ReadIntervalTimeout = new_timeouts->ReadIntervalTimeout;
		port->timeouts.ReadTotalTimeoutMultiplier = new_timeouts->ReadTotalTimeoutMultiplier;
		port->timeouts.ReadTotalTimeoutConstant = new_timeouts->ReadTotalTimeoutConstant;
		port->timeouts.WriteTotalTimeoutConstant = new_timeouts->WriteTotalTimeoutConstant;
		port->timeouts.WriteTotalTimeoutMultiplier = new_timeouts->WriteTotalTimeoutMultiplier;
		break;
	}
	case IOCTL_SERIAL_GET_TIMEOUTS: { 
		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(SERIAL_TIMEOUTS), &buffer, &buffer_size);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "IOCTL_SERIAL_GET_TIMEOUTS: WdfRequestRetrieveOutputBuffer failed %!STATUS!", status);
			break;
		}
		*((PSERIAL_TIMEOUTS)buffer) = port->timeouts;
		bytes_returned = sizeof(SERIAL_TIMEOUTS);
		break; 
	}
	case IOCTL_SERIAL_SET_CHARS: {
		PSERIAL_CHARS NewChars;

		status = WdfRequestRetrieveInputBuffer(Request, sizeof(ULONG), &buffer, &buffer_size);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "IOCTL_SERIAL_SET_CHARS: WdfRequestRetrieveInputBuffer failed %!STATUS!", status);
			break;
		}
		NewChars = (PSERIAL_CHARS)buffer;
		if (port->escape_char) {
			if ((port->escape_char == NewChars->XonChar) || (port->escape_char == NewChars->XoffChar)) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
		}
		port->special_chars = *NewChars;
		// TODO: Add 'chars' using SerialSetChars?
		break;
	}
	case IOCTL_SERIAL_GET_CHARS: {

		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(SERIAL_CHARS), &buffer, &buffer_size);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "IOCTL_SERIAL_GET_CHARS: Could not get request memory buffer %X\n", status);
			break;
		}

		*((PSERIAL_CHARS)buffer) = port->special_chars;
		bytes_returned = sizeof(SERIAL_CHARS);

		break;
	}
	case IOCTL_SERIAL_SET_DTR:
	case IOCTL_SERIAL_CLR_DTR: {
		UINT32 new_mcr = 0x00;
		
		if ((port->HandFlow.ControlHandShake & SERIAL_DTR_MASK) == SERIAL_DTR_HANDSHAKE) status = STATUS_INVALID_PARAMETER; 
		else {
			new_mcr = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, MCR_OFFSET);
			if (IoControlCode == IOCTL_SERIAL_SET_DTR) new_mcr |= 0x01;
			else new_mcr &= ~0x01;
			status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, MCR_OFFSET, new_mcr);
			if (NT_SUCCESS(status)) { port->current_mcr = new_mcr; }
		}
		break;
	}
	case IOCTL_SERIAL_RESET_DEVICE: { break; }
	case IOCTL_SERIAL_SET_RTS:
	case IOCTL_SERIAL_CLR_RTS: {
		UINT32 new_mcr = 0x00;

		if ((port->HandFlow.ControlHandShake & SERIAL_DTR_MASK) == SERIAL_DTR_HANDSHAKE) status = STATUS_INVALID_PARAMETER;
		else {
			new_mcr = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, MCR_OFFSET);
			if (IoControlCode == IOCTL_SERIAL_SET_DTR) new_mcr |= 0x02;
			else new_mcr &= ~0x02;
			status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, MCR_OFFSET, new_mcr);
			if (NT_SUCCESS(status)) { port->current_mcr = new_mcr; }
		}
		break;
	}
	case IOCTL_SERIAL_SET_XOFF: {
		status = asynccom_port_set_xonoff(port, FALSE);
		break;
	}
	case IOCTL_SERIAL_SET_XON: {
		status = asynccom_port_set_xonoff(port, TRUE);
		break;
	}
	case IOCTL_SERIAL_SET_BREAK_ON: {
		status = asynccom_port_set_break(port, TRUE);
		break;
	}
	case IOCTL_SERIAL_SET_BREAK_OFF: {
		status = asynccom_port_set_break(port, FALSE);
		break;
	}								
	case IOCTL_SERIAL_SET_QUEUE_SIZE: { status = STATUS_NOT_SUPPORTED; break; } // TODO: This appears to be basically just a memory cap value.
	case IOCTL_SERIAL_GET_WAIT_MASK: { status = STATUS_NOT_SUPPORTED; break; } // We don't use the ISR, so this has no value.
	case IOCTL_SERIAL_SET_WAIT_MASK: { status = STATUS_NOT_SUPPORTED; break; } // We don't use the ISR, so this has no value.
	case IOCTL_SERIAL_WAIT_ON_MASK: { status = STATUS_NOT_SUPPORTED; break; } // We don't use the ISR, so this has no value.
	case IOCTL_SERIAL_IMMEDIATE_CHAR: { status = STATUS_NOT_SUPPORTED; break; } // We may be able to implement this - but currently we send every request directly to the Asynccom. This has no current valuable implementation.
    case IOCTL_SERIAL_PURGE: {
        ULONG mask;
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(ULONG), &buffer, &buffer_size);
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveInputBuffer failed %!STATUS!", status);
            break;
        }
        mask = *((ULONG *)(buffer));
        TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "IOCTL: Attempting to purge.. %lu", mask);
        asynccom_port_purge(port, mask);	
        break; 
    }
	case IOCTL_SERIAL_GET_HANDFLOW: { 
		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(ULONG), &buffer, &buffer_size);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "IOCTL_SERIAL_GET_HANDFLOW: WdfRequestRetrieveOutputBuffer failed %!STATUS!", status);
			break;
		}
		*((PSERIAL_HANDFLOW)buffer) = port->HandFlow;
		bytes_returned = sizeof(SERIAL_HANDFLOW);
		break; 
	}
	case IOCTL_SERIAL_SET_HANDFLOW: {
		PSERIAL_HANDFLOW handflow;
		
		status = WdfRequestRetrieveInputBuffer(Request, sizeof(ULONG), &buffer, &buffer_size);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "IOCTL_SERIAL_SET_HANDFLOW :WdfRequestRetrieveInputBuffer failed %!STATUS!", status);
			break;
		}
		handflow = (PSERIAL_HANDFLOW)buffer;
		status = asynccom_port_set_flowcontrol(port, handflow);

		break;
	}
    case IOCTL_SERIAL_GET_MODEMSTATUS: {
        
        status = WdfRequestRetrieveOutputBuffer(Request, sizeof(ULONG), &buffer, &buffer_size);
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "IOCTL_SERIAL_GET_MODEMSTATUS: WdfRequestRetrieveOutputBuffer failed %!STATUS!", status);
            break;
        }
        *(PULONG)buffer = asynccom_port_modem_status(port);
        bytes_returned = sizeof(ULONG);
        
        break;
    } 
	case IOCTL_SERIAL_GET_DTRRTS: {
		ULONG modem_control = 0;
	
		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(ULONG), &buffer, &buffer_size);
		if (!NT_SUCCESS(status)) {
			break;
		}
		modem_control = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, MCR_OFFSET);
		modem_control &= (SERIAL_DTR_STATE | SERIAL_RTS_STATE);
		*(PULONG)buffer = modem_control;
		bytes_returned = sizeof(ULONG);
		break;
	}
	case IOCTL_SERIAL_GET_COMMSTATUS: { 
        status = WdfRequestRetrieveOutputBuffer(Request, sizeof(SERIAL_STATUS), &buffer, &buffer_size);
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "IOCTL_SERIAL_GET_COMMSTATUS: WdfRequestRetrieveOutputBuffer failed %!STATUS!", status);
            break;
        }
        status = asynccom_port_get_status(port, buffer);
        bytes_returned = sizeof(SERIAL_STATUS);
        break; 
    } 
	case IOCTL_SERIAL_GET_PROPERTIES: { 
		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(SERIAL_COMMPROP), &buffer, &buffer_size);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "IOCTL_SERIAL_GET_PROPERTIES: WdfRequestRetrieveOutputBuffer failed %!STATUS!", status);
			break;
		}
		status = asynccom_port_get_properties(port, buffer);
		bytes_returned = sizeof(SERIAL_COMMPROP);
		break; 
	}
	case IOCTL_SERIAL_XOFF_COUNTER: { status = STATUS_NOT_SUPPORTED; break; }
	case IOCTL_SERIAL_LSRMST_INSERT: { status = STATUS_NOT_SUPPORTED; break; }
	case IOCTL_SERIAL_CONFIG_SIZE: { 
		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(ULONG), &buffer, &buffer_size);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "IOCTL_SERIAL_CONFIG_SIZE: WdfRequestRetrieveOutputBuffer failed %!STATUS!", status);
			break;
		} 
		
		bytes_returned = sizeof(ULONG);
		*(PULONG)buffer = 0;
		status = STATUS_SUCCESS;
		break; 
	}
	case IOCTL_SERIAL_GET_STATS: { 
		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(SERIALPERF_STATS), &buffer, &buffer_size);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "IOCTL_SERIAL_GET_STATS: WdfRequestRetrieveOutputBuffer failed %!STATUS!", status);
			break;
		}
		bytes_returned = sizeof(SERIALPERF_STATS);
		RtlZeroMemory(buffer, sizeof(SERIALPERF_STATS));
		break; 
	}
	case IOCTL_SERIAL_CLEAR_STATS: { 
		status = STATUS_SUCCESS; 	
		break; 
	}
	case IOCTL_ASYNCCOM_SET_CLOCK_RATE: {
		unsigned *desired_clock = 0;

		status = WdfRequestRetrieveInputBuffer(Request, sizeof(*desired_clock), (PVOID *)&desired_clock, NULL);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveOutputBuffer failed %!STATUS!", status);
			break;
		}
		status = asynccom_port_set_clock_rate(port, *desired_clock);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "set_clock_rate failed %!STATUS!", status);
			break;
		}
		break;
	}
	case IOCTL_ASYNCCOM_SET_SAMPLE_RATE: {
		unsigned *desired_sample = 0;

		status = WdfRequestRetrieveInputBuffer(Request, sizeof(*desired_sample), (PVOID *)&desired_sample, NULL);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveOutputBuffer failed %!STATUS!", status);
			break;
		}
		status = asynccom_port_set_sample_rate(port, *desired_sample);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "set_clock_rate failed %!STATUS!", status);
			break;
		}
		break;
	}
	case IOCTL_ASYNCCOM_GET_SAMPLE_RATE: {
		unsigned *sample_rate = 0;

		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(*sample_rate), (PVOID *)&sample_rate, NULL);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveOutputBuffer failed %!STATUS!", status);
			break;
		}

		*sample_rate = port->current_sample_rate;

		bytes_returned = sizeof(*sample_rate);
		break;
	}
	case IOCTL_ASYNCCOM_REPROGRAM: {
		unsigned char *firmware_line = 0;
		size_t data_size = 0;

		status = WdfRequestRetrieveInputBuffer(Request, 1, (PVOID *)&firmware_line, &buffer_size);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveInputMemory failed %!STATUS!", status);
			break;
		}
		for (data_size = 0;; data_size++)
		{
			if (firmware_line[data_size] == '\n') break; // Should pass everything before the newline.
			if (data_size > 50) break; // Best safety check I can think of - highest should be ~47, so this will see a runaway line.
		}
		if (data_size < 51) status = asynccom_port_program_firmware(port, firmware_line, data_size);
		// Currently I get, store, get, modify, send the buffer - there's a better way, I'm just rushed.
		break;
	}
	case IOCTL_ASYNCCOM_SET_DIVISOR: {
		unsigned short *new_divisor = 0;

		status = WdfRequestRetrieveInputBuffer(Request, sizeof(*new_divisor), (PVOID *)&new_divisor, NULL);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveInputBuffer failed %!STATUS!", status);
			break;
		}
		status = asynccom_port_set_divisor(port, *new_divisor);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "set_divisor failed %!STATUS!", status);
			break;
		}
		break;
	}
	case IOCTL_ASYNCCOM_SET_TX_TRIGGER: {
		
		UINT32 *new_tx_trigger = 0;

		status = WdfRequestRetrieveInputBuffer(Request, sizeof(*new_tx_trigger), (PVOID *)&new_tx_trigger, NULL);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveInputBuffer failed %!STATUS!", status);
			break;
		}
		status = asynccom_port_set_tx_trigger(port, *new_tx_trigger);
		break;
	}
	case IOCTL_ASYNCCOM_GET_TX_TRIGGER: {
		UINT32 *tx_trigger = 0;

		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(*tx_trigger), (PVOID *)&tx_trigger, NULL);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveOutputBuffer failed %!STATUS!", status);
			break;
		}
		*tx_trigger = asynccom_port_get_tx_trigger(port);
		bytes_returned = sizeof(unsigned);
		break;
	}
	case IOCTL_ASYNCCOM_SET_RX_TRIGGER: {
		UINT32 *new_rx_trigger = 0;

		status = WdfRequestRetrieveInputBuffer(Request, sizeof(*new_rx_trigger), (PVOID *)&new_rx_trigger, NULL);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "Could not get request memory buffer %X\n", status);
			break;
		}
		status = asynccom_port_set_rx_trigger(port, *new_rx_trigger);
		break;
	}
	case IOCTL_ASYNCCOM_GET_RX_TRIGGER: {
		UINT32 *rx_trigger = 0;

		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(*rx_trigger), (PVOID *)&rx_trigger, NULL);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveOutputBuffer failed %!STATUS!", status);
			break;
		}
		*rx_trigger = asynccom_port_get_rx_trigger(port);
		bytes_returned = sizeof(UINT32);
		break;
	}
	case IOCTL_ASYNCCOM_SET_EXTERNAL_TRANSMIT: {
		unsigned *external_transmit = 0;

		status = WdfRequestRetrieveInputBuffer(Request, sizeof(*external_transmit), (PVOID *)&external_transmit, NULL);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "Could not get request memory buffer %X\n", status);
			break;
		}
		status = asynccom_port_set_external_transmit(port, *external_transmit);
		break;
	}
	case IOCTL_ASYNCCOM_GET_EXTERNAL_TRANSMIT: {
		UINT32 *external_transmit = 0;

		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(*external_transmit), (PVOID *)&external_transmit, NULL);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveOutputBuffer failed %!STATUS!", status);
			break;
		}
		*external_transmit = asynccom_port_get_external_transmit(port);
		bytes_returned = sizeof(UINT32);
		break;
	}
	case IOCTL_ASYNCCOM_SET_FRAME_LENGTH: {
		unsigned *frame_length = 0;

		status = WdfRequestRetrieveInputBuffer(Request, sizeof(*frame_length), (PVOID *)&frame_length, NULL);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "Could not get request memory buffer %X\n", status);
			break;
		}
		status = asynccom_port_set_frame_length(port, *frame_length);
		break;
	}
	case IOCTL_ASYNCCOM_GET_FRAME_LENGTH: {
		UINT32 *frame_length = 0;

		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(*frame_length), (PVOID *)&frame_length, NULL);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveOutputBuffer failed %!STATUS!", status);
			break;
		}
		*frame_length = asynccom_port_get_frame_length(port);
		bytes_returned = sizeof(UINT32);
		break;
	}
	case IOCTL_ASYNCCOM_ENABLE_ECHO_CANCEL: {
		status = asynccom_port_set_echo_cancel(port, TRUE);
		break;
	}
	case IOCTL_ASYNCCOM_DISABLE_ECHO_CANCEL: {
		status = asynccom_port_set_echo_cancel(port, FALSE);
		break;
	}
	case IOCTL_ASYNCCOM_GET_ECHO_CANCEL: {
		BOOLEAN *echo_cancel = 0;

		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(*echo_cancel), (PVOID *)&echo_cancel, NULL);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveOutputBuffer failed %!STATUS!", status);
			break;
		}
		*echo_cancel = asynccom_port_get_echo_cancel(port);
		bytes_returned = sizeof(BOOLEAN);
		break;
	}
	case IOCTL_ASYNCCOM_ENABLE_RS485: {
		status = asynccom_port_set_rs485(port, TRUE);
		break;
	}
	case IOCTL_ASYNCCOM_DISABLE_RS485: {
		status = asynccom_port_set_rs485(port, FALSE);
		break;
	}
	case IOCTL_ASYNCCOM_GET_RS485: {
		BOOLEAN *rs485 = 0;

		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(*rs485), (PVOID *)&rs485, NULL);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveOutputBuffer failed %!STATUS!", status);
			break;
		}
		*rs485 = asynccom_port_get_rs485(port);
		bytes_returned = sizeof(BOOLEAN);
		break;
	}
	case IOCTL_ASYNCCOM_ENABLE_9BIT: {
		status = asynccom_port_set_9bit(port, TRUE);
		break;
	}
	case IOCTL_ASYNCCOM_DISABLE_9BIT: {
		status = asynccom_port_set_9bit(port, FALSE);
		break;
	}
	case IOCTL_ASYNCCOM_GET_9BIT: {
		BOOLEAN *nine_bit = 0;

		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(*nine_bit), (PVOID *)&nine_bit, NULL);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveOutputBuffer failed %!STATUS!", status);
			break;
		}
		*nine_bit = asynccom_port_get_9bit(port);
		bytes_returned = sizeof(BOOLEAN);
		break;
	}
	case IOCTL_ASYNCCOM_ENABLE_ISOCHRONOUS: {
		int *isochronous = 0;
		
		status = WdfRequestRetrieveInputBuffer(Request, sizeof(*isochronous), (PVOID *)&isochronous, NULL);
		if( !NT_SUCCESS(status) ) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveInputBuffer failed %!STATUS!", status);
			break;
		}
		status = asynccom_port_set_isochronous(port, *isochronous);
		break;
	}
	case IOCTL_ASYNCCOM_DISABLE_ISOCHRONOUS: {
		status = asynccom_port_set_isochronous(port, -1);
		break;
	}
	case IOCTL_ASYNCCOM_GET_ISOCHRONOUS: {
		int *isochronous = 0;
	
		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(*isochronous), (PVOID *)&isochronous, NULL);
		 if( !NT_SUCCESS(status) ) {
			TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "WdfRequestRetrieveOutputBuffer failed %!STATUS!", status);
			break;
		 }

		*isochronous = asynccom_port_get_isochronous(port);

		bytes_returned = sizeof(int);
		break;
	}


	default :
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    if (request_pending == FALSE) {
		WdfRequestCompleteWithInformation(Request, status, bytes_returned);
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL, "%s: Exiting.", __FUNCTION__);

    return;
}

NTSTATUS asynccom_port_get_status(_In_ struct asynccom_port *port, PSERIAL_STATUS settings)
{
    RtlZeroMemory(settings, sizeof(SERIAL_STATUS));
    settings->Errors = 0;
    settings->EofReceived = 0;
    settings->AmountInInQueue = asynccom_port_get_input_memory_usage(port);
    settings->AmountInOutQueue = 0;
    settings->HoldReasons = 0;
    if (port->TXHolding) {
        if (port->TXHolding & SERIAL_TX_CTS) settings->HoldReasons |= SERIAL_TX_WAITING_FOR_CTS;
        if (port->TXHolding & SERIAL_TX_DSR) settings->HoldReasons |= SERIAL_TX_WAITING_FOR_DSR;
        if (port->TXHolding & SERIAL_TX_DCD) settings->HoldReasons |= SERIAL_TX_WAITING_FOR_DCD;
        if (port->TXHolding & SERIAL_TX_XOFF) settings->HoldReasons |= SERIAL_TX_WAITING_FOR_XON;
        if (port->TXHolding & SERIAL_TX_BREAK) settings->HoldReasons |= SERIAL_TX_WAITING_ON_BREAK;
    }

    if (port->RXHolding & SERIAL_RX_DSR) settings->HoldReasons |= SERIAL_RX_WAITING_FOR_DSR;
    if (port->RXHolding & SERIAL_RX_XOFF) settings->HoldReasons |= SERIAL_TX_WAITING_XOFF_SENT;

    return STATUS_SUCCESS;
}

NTSTATUS asynccom_port_set_divisor(_In_ struct asynccom_port *port, unsigned short divisor)
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT32 new_lcr = 0, dll = 0, dlm = 0, old_lcr = 0;

	old_lcr = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET);
	new_lcr = old_lcr | 0x80;
	dll = (unsigned char)divisor & 0xff;
	dlm = (unsigned char)((divisor & 0xff00) >> 8);
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET, new_lcr);
	if (!NT_SUCCESS(status)) return status;
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, DLL_OFFSET, dll);
	if (!NT_SUCCESS(status)) return status;
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, DLM_OFFSET, dlm);
	if (NT_SUCCESS(status)) port->current_divisor = divisor;
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL, "%s: New divisor: %d.", __FUNCTION__, divisor);
	return status;
}

#define STRB_BASE 0x00000008
#define DTA_BASE 0x00000001
#define CLK_BASE 0x00000002
void asynccom_port_set_clock_bits(struct asynccom_port *port, unsigned char *clock_data)
{
	UINT32 orig_fcr_value = 0, new_fcr_value = 0;
	int j = 0; // Must be signed because we are going backwards through the array
	int i = 0; // Must be signed because we are going backwards through the array
	unsigned strb_value = STRB_BASE;
	unsigned dta_value = DTA_BASE;
	unsigned clk_value = CLK_BASE;
	UINT32 *data = 0;
	unsigned data_index = 0;

	return_if_untrue(port);
#ifdef DISABLE_XTAL
	clock_data[15] &= 0xfb;
#else
	clock_data[15] |= 0x04;
#endif

	data = (UINT32 *)ExAllocatePoolWithTag(PagedPool, sizeof(UINT32) * 323, 'stiB');

	if (data == NULL) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "ExAllocatePoolWithTag failed");
		return;
	}

	orig_fcr_value = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS, FPGA_FCR_OFFSET);
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "%s: Original FCR: 0x%8.8x\n", __FUNCTION__, orig_fcr_value);
	data[data_index++] = new_fcr_value = orig_fcr_value & 0xfffff0f0;

	for (i = 19; i >= 0; i--) {
		for (j = 7; j >= 0; j--) {
			int bit = ((clock_data[i] >> j) & 1);

			if (bit)
				new_fcr_value |= dta_value; /* Set data bit */
			else
				new_fcr_value &= ~dta_value; /* Clear clock bit */

			data[data_index++] = new_fcr_value |= clk_value; /* Set clock bit */
			data[data_index++] = new_fcr_value &= ~clk_value; /* Clear clock bit */
		}
	}

	new_fcr_value = orig_fcr_value & 0xfffff0f0;

	new_fcr_value |= strb_value; /* Set strobe bit */
	new_fcr_value &= ~clk_value; /* Clear clock bit */

	data[data_index++] = new_fcr_value;
	data[data_index++] = orig_fcr_value;

	asynccom_port_set_register_rep_uint32(port, FPGA_UPPER_ADDRESS, FPGA_FCR_OFFSET, data, data_index);

	ExFreePoolWithTag(data, 'stiB');
}

NTSTATUS asynccom_port_program_firmware(_In_ struct asynccom_port *port, unsigned char *firmware_line, size_t data_size)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDFUSBPIPE write_pipe;
	WDF_MEMORY_DESCRIPTOR write_descriptor;
	unsigned char firmware_buffer[75];
	PULONG bytes_written = 0;
	
	write_pipe = port->register_write_pipe;
	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&write_descriptor, (PVOID)&firmware_buffer, (ULONG)data_size+1);
	RtlCopyMemory(&firmware_buffer[1], firmware_line, data_size);
	firmware_buffer[0] = 0x06;
	status = WdfUsbTargetPipeWriteSynchronously(write_pipe, WDF_NO_HANDLE, NULL, &write_descriptor, bytes_written);
	return status;
}
	
NTSTATUS asynccom_port_set_spr_register(struct asynccom_port *port, unsigned char address, UINT32 value)
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT32 old_lcr = 0;

	old_lcr = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET);
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET, 0x0);
	if (!NT_SUCCESS(status)) return status;
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, SPR_OFFSET, address);
	if(NT_SUCCESS(status)) status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, ICR_OFFSET, value);
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET, old_lcr);
	if (address == ACR_OFFSET) port->current_acr = value;

	return status;
}

NTSTATUS asynccom_port_set_650_register(struct asynccom_port *port, unsigned char address, UINT32 value)
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT32 old_lcr = 0;

	old_lcr = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET);
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET, 0xBF);
	if (!NT_SUCCESS(status)) return status;
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, address, value);
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET, old_lcr);

	return status;
}

UINT32 asynccom_port_get_650_register(struct asynccom_port *port, unsigned char address)
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT32 old_lcr = 0, old_value = 0;

	old_lcr = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET);
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET, 0xBF);
	old_value = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, address);
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET, old_lcr);

	return old_value;
}

UINT32 asynccom_port_get_spr_register(struct asynccom_port *port, unsigned char address)
{
	NTSTATUS status = STATUS_SUCCESS;
	int failures = 0;
	UINT32 value = 0;

	
	status = asynccom_port_set_spr_register(port, ACR_OFFSET, port->current_acr | 0x40);
	if (!NT_SUCCESS(status)) return 0x00;
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, SPR_OFFSET, address);
	if (!NT_SUCCESS(status)) failures++;
	else value = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, ICR_OFFSET);
	if (!NT_SUCCESS(status)) failures++;
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, SPR_OFFSET, 0x00);
	status = asynccom_port_set_spr_register(port, ACR_OFFSET, port->current_acr & ~0x40);

	return value;
}

NTSTATUS asynccom_port_set_register_uint32(struct asynccom_port *port, unsigned char offset, unsigned char address, UINT32 value)
{
	WDF_MEMORY_DESCRIPTOR write_descriptor;	
	unsigned char write_buffer[7];
	NTSTATUS status;
	WDFUSBPIPE write_pipe;
	PULONG bytes_written = 0;

	write_pipe = port->register_write_pipe;

	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&write_descriptor, (PVOID)&write_buffer, sizeof(write_buffer));

	write_buffer[0] = ASYNCCOM_WRITE_REGISTER;
	write_buffer[1] = offset;
	write_buffer[2] = address << 1;
#ifdef __BIG_ENDIAN
#else
	value = _BYTESWAP_UINT32(value);
#endif
	memcpy(&write_buffer[3], &value, sizeof(UINT32));

	status = WdfUsbTargetPipeWriteSynchronously(write_pipe, WDF_NO_HANDLE, NULL, &write_descriptor, bytes_written);
	if(!NT_SUCCESS(status)) TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "%s: Error status: 0x%x.", __FUNCTION__, status);
	return status;
}

UINT32 asynccom_port_get_register_uint32(struct asynccom_port *port, unsigned char offset, unsigned char address)
{
	WDF_MEMORY_DESCRIPTOR  write_descriptor, read_descriptor;
	NTSTATUS status;
	WDFUSBPIPE write_pipe, read_pipe;
	unsigned char write_buffer[3];
	PULONG bytes_written = 0, bytes_read = 0;
	UINT32 original_value = 0;

	write_pipe = port->register_write_pipe;
	read_pipe = port->register_read_pipe;

	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&write_descriptor, (PVOID)&write_buffer, sizeof(write_buffer));
	write_buffer[0] = ASYNCCOM_READ_REGISTER;
	write_buffer[1] = offset;
	write_buffer[2] = address << 1;

	status = WdfUsbTargetPipeWriteSynchronously(write_pipe, WDF_NO_HANDLE, NULL, &write_descriptor, bytes_written);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTL, "%s: Error status: 0x%x.", __FUNCTION__, status);
		return 0;
	}

	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&read_descriptor, (PVOID)&original_value, sizeof(UINT32));
	status = WdfUsbTargetPipeReadSynchronously(read_pipe, WDF_NO_HANDLE, NULL, &read_descriptor, bytes_read);
	if (NT_SUCCESS(status)) {
#ifdef __BIG_ENDIAN
#else
		original_value = _BYTESWAP_UINT32(original_value);
#endif
	}
	return original_value;
}

void asynccom_port_set_register_rep_uint32(_In_ struct asynccom_port *port, unsigned char offset, unsigned char address, const UINT32 *buf, unsigned write_count)
{
	unsigned i = 0;
	const UINT32 *outgoing_data;
	NTSTATUS status = STATUS_SUCCESS;

	outgoing_data = buf;
	for (i = 0; i < write_count; i++)
	{
		status = asynccom_port_set_register_uint32(port, offset, address, outgoing_data[i]);
		if (!NT_SUCCESS(status))
			TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL, "%s: failed iteration: %d, status: %!STATUS!\n", __FUNCTION__, i, status);
	}

}

unsigned asynccom_port_get_input_memory_usage(struct asynccom_port *port)
{
	unsigned value = 0;

	return_val_if_untrue(port, 0);

	WdfSpinLockAcquire(port->istream_spinlock);
	value += asynccom_frame_get_length(port->istream);
	WdfSpinLockRelease(port->istream_spinlock);

	return value;
}

unsigned asynccom_port_get_input_memory_cap(struct asynccom_port *port)
{
	return_val_if_untrue(port, 0);

	return port->memory_cap.input;
}

NTSTATUS asynccom_port_set_sample_rate(_In_ struct asynccom_port *port, unsigned rate)
{
	NTSTATUS status = STATUS_SUCCESS;
	if (rate < 4 || rate > 16) return STATUS_INVALID_PARAMETER;
	status = asynccom_port_set_spr_register(port, TCR_OFFSET, rate);
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL, "%s: New sample rate: %d.", __FUNCTION__, rate);
	port->current_sample_rate = rate;
	return status;
}

NTSTATUS asynccom_port_set_echo_cancel(_In_ struct asynccom_port *port, BOOLEAN echocancel)
{
	UINT32 original_fcr, new_fcr, bit_mask = 0x00010000;
	NTSTATUS status = STATUS_SUCCESS;

	original_fcr = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS, FPGA_FCR_OFFSET);
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT, "%s: Original FCR value: 0x%8.8x\n", __FUNCTION__, original_fcr);
	if (echocancel) new_fcr = original_fcr | bit_mask;
	else new_fcr = original_fcr & ~bit_mask;

	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS, FPGA_FCR_OFFSET, (UINT32)new_fcr);
	TraceEvents(TRACE_LEVEL_WARNING, DBG_INIT, "%s: New FCR value: 0x%8.8x\n", __FUNCTION__, new_fcr);
	return status;
}

BOOLEAN asynccom_port_get_echo_cancel(_In_ struct asynccom_port *port)
{
	UINT32 fcr_val, bit_mask = 0x00010000;

	fcr_val = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS, FPGA_FCR_OFFSET);

	return (fcr_val & bit_mask) ? TRUE : FALSE;
}

NTSTATUS asynccom_port_set_isochronous(_In_ struct asynccom_port *port, int mode)
{
	unsigned char new_cks = 0, new_mdm = 0;
	NTSTATUS status = STATUS_SUCCESS;

	switch (mode)
	{
	case 2:
	case 3:
	case 4:
	case 10:
		new_cks |= 0x09;
		new_mdm |= 0x02;
		break;
	case 5:
	case 6:
	case 7:
		new_cks |= 0x0A;
		break;
	case 8:
		new_cks |= 0x0B;
		break;
	}
	switch (mode)
	{
	case 0:
	case 3:
	case 6:
	case 8:
		new_cks |= 0xD0;
		new_mdm |= 0x04;
		break;
	case 1:
	case 4:
	case 7:
		new_cks |= 0x90;
		break;
	case 9:
	case 10:
		new_cks |= 0x10;
		break;
	}
	status = asynccom_port_set_spr_register(port, MDM_OFFSET, new_mdm);
	if (!NT_SUCCESS(status)) { return status; }
	status = asynccom_port_set_spr_register(port, CKS_OFFSET, new_cks);
	return status;
}

int asynccom_port_get_isochronous(_In_ struct asynccom_port *port)
{
	UINT32 current_cks = 0;
	int mode = 0;
	
	current_cks = asynccom_port_get_spr_register(port, CKS_OFFSET);
	
	switch (current_cks) {
    case 0x00:
        mode = -1;
        break;

    case 0xD0:
        mode = 0;
        break;

    case 0x90:
        mode = 1;
        break;

    case 0x09:
        mode = 2;
        break;

    case 0xD9:
        mode = 3;
        break;

    case 0x99:
        mode = 4;
        break;

    case 0x0A:
        mode = 5;
        break;

    case 0xDA:
        mode = 6;
        break;

    case 0x9A:
        mode = 7;
        break;

    case 0xDB:
        mode = 8;
        break;

    case 0x10:
        mode = 9;
        break;

    case 0x19:
        mode = 10;
        break;
    }
	
	return mode;
}

NTSTATUS asynccom_port_set_9bit(_In_ struct asynccom_port *port, BOOLEAN ninebit)
{
	NTSTATUS status = STATUS_SUCCESS;
	unsigned char nbm = 0x00;

	if (ninebit) nbm = 0x01;
	status = asynccom_port_set_spr_register(port, NMR_OFFSET, nbm);
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT, "%s: Setting nine bit mode: %s\n", __FUNCTION__, nbm ? "ON":"OFF");
	return status;
}

BOOLEAN asynccom_port_get_9bit(_In_ struct asynccom_port *port)
{
	return (asynccom_port_get_spr_register(port, NMR_OFFSET)) ? TRUE : FALSE;
}

NTSTATUS asynccom_port_set_xonoff(_In_ struct asynccom_port *port, BOOLEAN onoff)
{
	NTSTATUS status = STATUS_SUCCESS, second_status = STATUS_SUCCESS;
	UINT32 new_efr = 0x00, old_lcr = 0x00;

	old_lcr = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET);
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET, 0xBF);
	if (!NT_SUCCESS(status)) return status;
	new_efr = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, EFR_OFFSET);
	if (onoff) new_efr |= 0x0a;
	else new_efr &= ~0x0a;
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, EFR_OFFSET, new_efr);
	second_status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET, old_lcr);
	if (!NT_SUCCESS(second_status)) return second_status;

	return status;
}

NTSTATUS asynccom_port_set_break(_In_ struct asynccom_port *port, BOOLEAN onoff) {
	NTSTATUS status = STATUS_SUCCESS;
	UINT32 new_lcr = 0x00, old_lcr = 0;
	
	old_lcr = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET);
	if (onoff)
	{
		if ((port->HandFlow.FlowReplace & SERIAL_RTS_MASK) == SERIAL_TRANSMIT_TOGGLE) {
			status = asynccom_port_set_rs485(port, TRUE);
			new_lcr = old_lcr | SERIAL_LCR_BREAK;
			status = asynccom_port_set_register_uint32(port, ASYNCCOM_UPPER_OFFSET + FPGA_UPPER_ADDRESS, LCR_OFFSET, new_lcr);
			if (NT_SUCCESS(status)) {
				port->TXHolding |= SERIAL_TX_BREAK;
			}

		}
	}
	else
	{
		if (port->TXHolding & SERIAL_TX_BREAK) {
			new_lcr = old_lcr & ~SERIAL_LCR_BREAK;
			port->TXHolding &= ~SERIAL_TX_BREAK;
			status = asynccom_port_set_register_uint32(port, ASYNCCOM_UPPER_OFFSET + FPGA_UPPER_ADDRESS, LCR_OFFSET, new_lcr);
			// Not really sure - some special condition needs to happen here to raise rts.
		}
	}

	return status;
}

ULONG asynccom_port_modem_status(_In_ struct asynccom_port *port)
{
	UINT32 modem_status = 0x00;
	unsigned long current_tx_holding = port->TXHolding;


	modem_status = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, MSR_OFFSET);
	
    // Because we don't get the modem status passively (no interrupts) the next
    // section is ultimately useless. It would only happen when it's actively
    // read by the user, which means the user knowns.
    /*
	if (port->escape_char) {
		if (modem_status & (SERIAL_MSR_DCTS |
			SERIAL_MSR_DDSR |
			SERIAL_MSR_TERI |
			SERIAL_MSR_DDCD)) {
            // This puts the modem_status character into the data stream.
			// serialputchar escape char
			// serialputchar serial_lsrmst_mst
			// serialputchar modem_status
		}
	}
    */
	if (port->HandFlow.ControlHandShake & SERIAL_DSR_SENSITIVITY) {
		if (modem_status & SERIAL_MSR_DSR)  port->RXHolding &= ~SERIAL_RX_DSR;
		else port->RXHolding |= SERIAL_RX_DSR;
	}
	else port->RXHolding &= ~SERIAL_RX_DSR;

	// ISR wait mask stuff

	if (port->HandFlow.ControlHandShake & SERIAL_OUT_HANDSHAKEMASK) {
		if (port->HandFlow.ControlHandShake & SERIAL_CTS_HANDSHAKE) {		
			if (modem_status & SERIAL_MSR_CTS) port->TXHolding &= ~SERIAL_TX_CTS;
			else port->TXHolding |= SERIAL_TX_CTS;
		}
		else port->TXHolding &= ~SERIAL_TX_CTS;
		if (port->HandFlow.ControlHandShake & SERIAL_DSR_HANDSHAKE) {
			if (modem_status & SERIAL_MSR_DSR) port->TXHolding &= ~SERIAL_TX_DSR;
			else port->TXHolding |= SERIAL_TX_DSR;
		}
		else port->TXHolding &= ~SERIAL_TX_DSR;
		if (port->HandFlow.ControlHandShake & SERIAL_DCD_HANDSHAKE) {
			if (modem_status & SERIAL_MSR_DCD) port->TXHolding &= ~SERIAL_TX_DCD;
			else port->TXHolding |= SERIAL_TX_DCD;
		}
		else port->TXHolding &= ~SERIAL_TX_DCD;

		if (!current_tx_holding && port->TXHolding && ((port->HandFlow.FlowReplace & SERIAL_RTS_MASK) == SERIAL_TRANSMIT_TOGGLE)) {
			// StartTimerToLowerRTSDPC
		}
		/*
			if (!doingTX && current_tx_holding && !port->TXHolding) {
			if(port->TXHolding && (port->TransmiteImmediate || port->WriteLength) && port->HoldingEmpty) {
			}
			}
		*/
		
	}
	return (ULONG)modem_status;
}

NTSTATUS asynccom_port_set_autorts(_In_ struct asynccom_port *port, BOOLEAN onoff)
{
	NTSTATUS status = STATUS_SUCCESS, second_status = STATUS_SUCCESS;
	UINT32 new_efr = 0x00, old_lcr = 0x00;

	old_lcr = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET);
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET, 0xBF);
	if (!NT_SUCCESS(status)) return status;
	new_efr = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, EFR_OFFSET);
	if (onoff) new_efr |= 0x40;
	else new_efr &= ~0x40;
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, EFR_OFFSET, new_efr);
	second_status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET, old_lcr);
	if (!NT_SUCCESS(second_status)) return second_status;
	return status;
}

NTSTATUS asynccom_port_set_autocts(_In_ struct asynccom_port *port, BOOLEAN onoff)
{
	NTSTATUS status = STATUS_SUCCESS, second_status = STATUS_SUCCESS;
	UINT32 new_efr = 0x00, old_lcr = 0;

	old_lcr = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET);
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET, 0xBF);
	if (!NT_SUCCESS(status)) return status;
	new_efr = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, EFR_OFFSET);
	if (onoff) new_efr |= 0x80;
	else new_efr &= ~0x80;
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, EFR_OFFSET, new_efr);
	second_status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET, old_lcr);
	if (!NT_SUCCESS(second_status)) return second_status;
	return status;
}

NTSTATUS asynccom_port_set_autodtr(_In_ struct asynccom_port *port, BOOLEAN onoff)
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT32 new_acr = 0x00;

	if (onoff) new_acr = port->current_acr | 0x08;
	else new_acr = port->current_acr & ~0x08;

	status = asynccom_port_set_spr_register(port, ACR_OFFSET, new_acr);
	return status;
}

NTSTATUS asynccom_port_set_autodsr(_In_ struct asynccom_port *port, BOOLEAN onoff)
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT32 new_acr = 0x00;

	if (onoff) new_acr = port->current_acr | 0x04;
	else new_acr = port->current_acr & ~0x04;

	status = asynccom_port_set_spr_register(port, ACR_OFFSET, new_acr);
	return status;
}

NTSTATUS asynccom_port_set_flowcontrol(_In_ struct asynccom_port *port, PSERIAL_HANDFLOW handflow)
{
	NTSTATUS status = STATUS_SUCCESS;

	if (handflow->ControlHandShake & SERIAL_CONTROL_INVALID) return STATUS_INVALID_PARAMETER;
	if (handflow->FlowReplace & SERIAL_FLOW_INVALID) return STATUS_INVALID_PARAMETER;
	if (port->escape_char) {
		if (handflow->FlowReplace & SERIAL_ERROR_CHAR) return STATUS_INVALID_PARAMETER;
	}
	if (handflow->ControlHandShake & SERIAL_DTR_HANDSHAKE) status = asynccom_port_set_autodtr(port, TRUE);
	else status = asynccom_port_set_autodtr(port, FALSE);
	if (!NT_SUCCESS(status)) return status;
	if (handflow->ControlHandShake & SERIAL_CTS_HANDSHAKE) status = asynccom_port_set_autocts(port, TRUE);
	else status = asynccom_port_set_autocts(port, FALSE);
	if (!NT_SUCCESS(status)) return status;
	if (handflow->ControlHandShake & SERIAL_DSR_HANDSHAKE) status = asynccom_port_set_autodsr(port, TRUE);
	else status = asynccom_port_set_autodsr(port, FALSE);
	if (!NT_SUCCESS(status)) return status;
	if (handflow->FlowReplace & SERIAL_RTS_HANDSHAKE) status = asynccom_port_set_autorts(port, TRUE);
	else status = asynccom_port_set_autorts(port, FALSE);


	return status;
}

NTSTATUS asynccom_port_get_properties(_In_ struct asynccom_port *port, PSERIAL_COMMPROP Properties)
{
	RtlZeroMemory(
		Properties,
		sizeof(SERIAL_COMMPROP)
	);

	Properties->PacketLength = sizeof(SERIAL_COMMPROP);
	Properties->PacketVersion = 2;
	Properties->ServiceMask = SERIAL_SP_SERIALCOMM;
	Properties->MaxTxQueue = 0;
	Properties->MaxRxQueue = 0;

	Properties->MaxBaud = SERIAL_BAUD_USER;
	//TODO
	//Properties->SettableBaud = port->supported_bauds;

	Properties->ProvSubType = SERIAL_SP_RS232;
	Properties->ProvCapabilities = SERIAL_PCF_DTRDSR |
		SERIAL_PCF_RTSCTS |
		SERIAL_PCF_CD |
		SERIAL_PCF_PARITY_CHECK |
		SERIAL_PCF_XONXOFF |
		SERIAL_PCF_SETXCHAR |
		SERIAL_PCF_TOTALTIMEOUTS |
		SERIAL_PCF_INTTIMEOUTS;
	Properties->SettableParams = SERIAL_SP_PARITY |
		SERIAL_SP_BAUD |
		SERIAL_SP_DATABITS |
		SERIAL_SP_STOPBITS |
		SERIAL_SP_HANDSHAKING |
		SERIAL_SP_PARITY_CHECK |
		SERIAL_SP_CARRIER_DETECT;


	Properties->SettableData = SERIAL_DATABITS_5 |
		SERIAL_DATABITS_6 |
		SERIAL_DATABITS_7 |
		SERIAL_DATABITS_8;
	Properties->SettableStopParity = SERIAL_STOPBITS_10 |
		SERIAL_STOPBITS_15 |
		SERIAL_STOPBITS_20 |
		SERIAL_PARITY_NONE |
		SERIAL_PARITY_ODD |
		SERIAL_PARITY_EVEN |
		SERIAL_PARITY_MARK |
		SERIAL_PARITY_SPACE;
	Properties->CurrentTxQueue = 0;
	Properties->CurrentRxQueue = port->buffer_size;

	return STATUS_SUCCESS;
}

#define STRB_BASE 0x00000008
#define DTA_BASE 0x00000001
#define CLK_BASE 0x00000002
NTSTATUS asynccom_port_set_clock_rate(_In_ struct asynccom_port *port, unsigned value)
{
	UINT32 orig_fcr_value = 0;
	UINT32 new_fcr_value = 0;
	int j = 0; // Must be signed because we are going backwards through the array
	int i = 0; // Must be signed because we are going backwards through the array
	unsigned strb_value = STRB_BASE;
	unsigned dta_value = DTA_BASE;
	unsigned clk_value = CLK_BASE;
	UINT32 *data = 0;
	unsigned data_index = 0;
	unsigned char clock_data[20];
	struct ResultStruct solutiona;  //final results for ResultStruct data calculations
	struct IcpRsStruct solutionb;   //final results for IcpRsStruct data calculations
	int return_value = 0;

	if (value < 15000 || value > 270000000) return STATUS_INVALID_PARAMETER;
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL, "%s: Trying clock rate: %d...", __FUNCTION__, value);
	memset(&solutiona, 0, sizeof(struct ResultStruct));
	memset(&solutionb, 0, sizeof(struct IcpRsStruct));

	return_value = GetICS30703Data(value, 2, &solutiona, &solutionb, clock_data);
	if (return_value != 0) {
		return STATUS_UNSUCCESSFUL;
	}
	clock_data[15] |= 0x04;

	data = (UINT32 *)ExAllocatePoolWithTag(NonPagedPool, sizeof(UINT32) * 323, 'stiB');

	if (data == NULL) return STATUS_UNSUCCESSFUL;

	orig_fcr_value = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS, FPGA_FCR_OFFSET);

	data[data_index++] = new_fcr_value = orig_fcr_value & 0xfffff0f0;

	for (i = 19; i >= 0; i--) {
		for (j = 7; j >= 0; j--) {
			int bit = ((clock_data[i] >> j) & 1);

			if (bit)
				new_fcr_value |= dta_value; /* Set data bit */
			else
				new_fcr_value &= ~dta_value; /* Clear clock bit */

			data[data_index++] = new_fcr_value |= clk_value; /* Set clock bit */
			data[data_index++] = new_fcr_value &= ~clk_value; /* Clear clock bit */
		}
	}

	new_fcr_value = orig_fcr_value & 0xfffff0f0;

	new_fcr_value |= strb_value; /* Set strobe bit */
	new_fcr_value &= ~clk_value; /* Clear clock bit */

	data[data_index++] = new_fcr_value;
	data[data_index++] = orig_fcr_value;

	asynccom_port_set_register_rep_uint32(port, FPGA_UPPER_ADDRESS, FPGA_FCR_OFFSET, data, data_index);

	ExFreePoolWithTag(data, 'stiB');
	port->current_clock_frequency = value;
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL, "%s: New clock rate: %d.", __FUNCTION__, value);
	return STATUS_SUCCESS;
}

NTSTATUS asynccom_port_set_tx_trigger(_In_ struct asynccom_port *port, UINT32 value) {
	NTSTATUS status = STATUS_SUCCESS;

	if (value > 127) return STATUS_INVALID_PARAMETER;
	status = asynccom_port_set_spr_register(port, TTL_OFFSET, value);
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT, "%s: Setting tx trigger: %d\n", __FUNCTION__, value);

	return status;
}

NTSTATUS asynccom_port_set_rx_trigger(_In_ struct asynccom_port *port, UINT32 value) {
	NTSTATUS status = STATUS_SUCCESS;

	if (value > 127) return STATUS_INVALID_PARAMETER;
	status = asynccom_port_set_spr_register(port, RTL_OFFSET, value);
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT, "%s: Setting rx trigger: %d\n", __FUNCTION__, value);

	return status;
}

UINT32 asynccom_port_get_tx_trigger(_In_ struct asynccom_port *port) {
	return asynccom_port_get_spr_register(port, TTL_OFFSET);
}

UINT32 asynccom_port_get_rx_trigger(_In_ struct asynccom_port *port) {
	return asynccom_port_get_spr_register(port, RTL_OFFSET);
}

NTSTATUS asynccom_port_set_chars(_In_ struct asynccom_port *port, PSERIAL_CHARS new_characters)
{
	NTSTATUS status = STATUS_SUCCESS;

	status = asynccom_port_set_650_register(port, XON1_OFFSET, new_characters->XonChar);
	status |= asynccom_port_set_650_register(port, XOFF1_OFFSET, new_characters->XoffChar);

	return status;
}

NTSTATUS asynccom_port_set_frame_length(_In_ struct asynccom_port *port, unsigned frame_length)
{
	NTSTATUS status = STATUS_SUCCESS;

	if (frame_length > 256 || frame_length < 1) return STATUS_INVALID_PARAMETER;
	status = asynccom_port_set_spr_register(port, FLR_OFFSET, frame_length - 1);
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT, "%s: Setting frame_length: %d\n", __FUNCTION__, frame_length);

	return status;
}

UINT32 asynccom_port_get_frame_length(_In_ struct asynccom_port *port)
{
	return asynccom_port_get_spr_register(port, FLR_OFFSET);
}

NTSTATUS asynccom_port_set_external_transmit(_In_ struct asynccom_port *port, unsigned frame_count)
{
	NTSTATUS status = STATUS_SUCCESS;

	if (frame_count > 256) return STATUS_INVALID_PARAMETER;

	status = asynccom_port_set_spr_register(port, EXTH_OFFSET, frame_count >> 8);
	status |= asynccom_port_set_spr_register(port, EXT_OFFSET, frame_count);
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT, "%s: Setting external transmit: %d\n", __FUNCTION__, frame_count);

	return status;
}

UINT32 asynccom_port_get_external_transmit(_In_ struct asynccom_port *port)
{
	UINT32 exth, ext;

	exth = asynccom_port_get_spr_register(port, EXTH_OFFSET);
	ext = asynccom_port_get_spr_register(port, EXT_OFFSET);
	return ((exth & 0x1f) << 8) + ext;
}

NTSTATUS asynccom_port_set_rs485(_In_ struct asynccom_port *port, BOOLEAN enable)
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT32 fcr_mask = 0x00040000, acr_mask = 0x00000010;
	UINT32 current_fcr = 0;

	current_fcr = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS, FPGA_FCR_OFFSET);

	if (enable) {
		status = asynccom_port_set_spr_register(port, ACR_OFFSET, port->current_acr | acr_mask);
		status |= asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS, FPGA_FCR_OFFSET, current_fcr | fcr_mask);
	}
	else {
		status = asynccom_port_set_spr_register(port, ACR_OFFSET, port->current_acr & ~acr_mask);
		status |= asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS, FPGA_FCR_OFFSET, current_fcr & ~fcr_mask);
	}

	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT, "%s: Setting rs485: %s\n", __FUNCTION__, enable ? "ON" : "OFF");
	return status;
}

BOOLEAN asynccom_port_get_rs485(_In_ struct asynccom_port *port)
{
	UINT32 fcr_val = 0x0, acr_val = 0x0;

	fcr_val = asynccom_port_get_register_uint32(port, FPGA_UPPER_ADDRESS, FPGA_FCR_OFFSET);
	acr_val = asynccom_port_get_spr_register(port, ACR_OFFSET);

	return ((fcr_val & 0x00040000) && (acr_val & 0x10)) ? TRUE : FALSE;
}