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

EVT_WDF_DPC		AsynccomProcessRead;
EVT_WDF_DPC		AsynccomProcessWrite;

NTSTATUS		asynccom_port_data_write(struct asynccom_port *port, const unsigned char *data, unsigned byte_count);
void			serial_read_timeout(IN WDFTIMER Timer);
int				get_next_read_request(struct asynccom_port *port);
void			complete_current_read_request(struct asynccom_port *port);
void			complete_current_write_request(struct asynccom_port *port);
NTSTATUS		asynccom_port_purge(_In_ struct asynccom_port *port, ULONG mask);

EVT_WDF_IO_QUEUE_IO_STOP AsyncComEvtIoStop;
EVT_WDF_USB_READER_COMPLETION_ROUTINE asynccom_port_received_data;
EVT_WDF_USB_READERS_FAILED FX3EvtReadFailed;
EVT_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE AsyncComEvtIoCancelOnQueue;

void basic_completion(_In_ WDFREQUEST Request, _In_ WDFIOTARGET Target, _In_ PWDF_REQUEST_COMPLETION_PARAMS CompletionParams, _In_ WDFCONTEXT Context);
void complete_current_wait_request(struct asynccom_port *port, NTSTATUS status, ULONG info, ULONG matches);
void event_occurred(struct asynccom_port *port, ULONG event);

#endif