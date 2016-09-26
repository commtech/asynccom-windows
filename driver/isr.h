/*
Copyright (C) 2016  Commtech, Inc.

This file is part of synccom-windows.

synccom-windows is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published bythe Free
Software Foundation, either version 3 of the License, or (at your option)
any later version.

synccom-windows is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
more details.

You should have received a copy of the GNU General Public License along
with synccom-windows.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SYNCCOM_ISR_H
#define SYNCCOM_ISR_H

#include <ntddk.h>
#include <wdf.h>

#include "trace.h"

EVT_WDF_DPC		AsynccomProcessRead;

NTSTATUS		asynccom_port_data_write(struct asynccom_port *port, const unsigned char *data, unsigned byte_count);
void			serial_read_timeout(IN WDFTIMER Timer);
int				get_next_request(struct asynccom_port *port);
void			complete_current_request(struct asynccom_port *port);
NTSTATUS		asynccom_port_purge(_In_ struct asynccom_port *port, ULONG mask);

EVT_WDF_IO_QUEUE_IO_STOP AsyncComEvtIoStop;
EVT_WDF_USB_READER_COMPLETION_ROUTINE asynccom_port_received_data;
EVT_WDF_USB_READERS_FAILED FX3EvtReadFailed;

#endif