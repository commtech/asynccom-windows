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

#ifndef ASYNCCOM_ISR_H
#define ASYNCCOM_ISR_H

#include <ntddk.h>
#include <wdf.h>

#include "trace.h"
#include "utils.h"

EVT_WDF_DPC								AsyncComProcessRead;
EVT_WDF_DPC								AsyncComProcessWrite;
EVT_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE	AsyncComEvtIoCancelOnQueue;
EVT_WDF_IO_QUEUE_IO_STOP				AsyncComEvtIoStop;

void									serial_read_timeout(IN WDFTIMER Timer);
void									complete_current_request(_In_ struct asynccom_port *port, _In_ NTSTATUS status_to_use, WDFREQUEST *current_request);

NTSTATUS								data_write(struct asynccom_port *port, const unsigned char *data, unsigned byte_count);
EVT_WDF_USB_READER_COMPLETION_ROUTINE	data_received;
EVT_WDF_USB_READERS_FAILED				data_received_failed;
EVT_WDF_REQUEST_CANCEL					cancel_wait;
EVT_WDF_REQUEST_CANCEL					cancel_read;
EVT_WDF_REQUEST_CANCEL					cancel_write;

void									event_occurred(struct asynccom_port *port, ULONG event);
void									process_timeouts(struct asynccom_port *port);
void									process_read(struct asynccom_port *port);
int										get_next_request(struct asynccom_port *port, WDFQUEUE Queue, WDFREQUEST *Request, IN PFN_WDF_REQUEST_CANCEL CancelRoutine);
void									set_cancel_routine(IN WDFREQUEST Request, IN PFN_WDF_REQUEST_CANCEL CancelRoutine);
NTSTATUS								clear_cancel_routine(IN WDFREQUEST Request);

#endif