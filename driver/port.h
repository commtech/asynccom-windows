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

#ifndef SYNCCOM_PORT_H
#define SYNCCOM_PORT_H

#include <ntddk.h>
#include <wdf.h>
#include <defines.h>
#include "trace.h"
#include "fx2Events.h"

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL AsyncComEvtIoDeviceControl;

NTSTATUS			asynccom_port_set_spr_register(struct asynccom_port *port, unsigned char address, UINT32 value);
UINT32				asynccom_port_get_spr_register(struct asynccom_port *port, unsigned char address);

NTSTATUS			asynccom_port_set_650_register(struct asynccom_port *port, unsigned char address, UINT32 value);
UINT32				asynccom_port_get_650_register(struct asynccom_port *port, unsigned char address);

NTSTATUS			asynccom_port_set_register_uint32(struct asynccom_port *port, unsigned char offset, unsigned char address, UINT32 value);
UINT32				asynccom_port_get_register_uint32(struct asynccom_port *port, unsigned char offset, unsigned char address);
void				asynccom_port_set_register_rep_uint32(_In_ struct asynccom_port *port, unsigned char offset, unsigned char address, const UINT32 *buf, unsigned write_count);

NTSTATUS			asynccom_port_set_rs485(_In_ struct asynccom_port *port, BOOLEAN rs485);
BOOLEAN				asynccom_port_get_rs485(_In_ struct asynccom_port *port);

NTSTATUS			asynccom_port_set_echo_cancel(_In_ struct asynccom_port *port, BOOLEAN echocancel);
BOOLEAN				asynccom_port_get_echo_cancel(_In_ struct asynccom_port *port);

NTSTATUS			asynccom_port_set_9bit(_In_ struct asynccom_port *port, BOOLEAN ninebit);
BOOLEAN				asynccom_port_get_9bit(_In_ struct asynccom_port *port);

NTSTATUS			asynccom_port_set_frame_length(_In_ struct asynccom_port *port, unsigned length);
UINT32				asynccom_port_get_frame_length(_In_ struct asynccom_port *port);

NTSTATUS			asynccom_port_set_external_transmit(_In_ struct asynccom_port *port, unsigned frame_count);
UINT32				asynccom_port_get_external_transmit(_In_ struct asynccom_port *port);

NTSTATUS			asynccom_port_set_autodtr(_In_ struct asynccom_port *port, BOOLEAN onoff);
NTSTATUS			asynccom_port_set_autodsr(_In_ struct asynccom_port *port, BOOLEAN onoff);

NTSTATUS			asynccom_port_set_autocts(_In_ struct asynccom_port *port, BOOLEAN onoff);
NTSTATUS			asynccom_port_set_autorts(_In_ struct asynccom_port *port, BOOLEAN onoff);

NTSTATUS			asynccom_port_set_flowcontrol(_In_ struct asynccom_port *port, PSERIAL_HANDFLOW handflow);

NTSTATUS			asynccom_port_get_properties(_In_ struct asynccom_port *port, PSERIAL_COMMPROP Properties);
NTSTATUS			asynccom_port_set_divisor(_In_ struct asynccom_port *port, unsigned short divisor);

unsigned			asynccom_port_get_input_memory_cap(struct asynccom_port *port);
unsigned			asynccom_port_get_input_memory_usage(struct asynccom_port *port);

NTSTATUS			asynccom_port_set_clock_rate(_In_ struct asynccom_port *port, unsigned value);

NTSTATUS			asynccom_port_set_tx_trigger(_In_ struct asynccom_port *port, UINT32 value);
UINT32				asynccom_port_get_tx_trigger(_In_ struct asynccom_port *port);

NTSTATUS			asynccom_port_set_rx_trigger(_In_ struct asynccom_port *port, UINT32 value);
UINT32				asynccom_port_get_rx_trigger(_In_ struct asynccom_port *port);

NTSTATUS			asynccom_port_set_sample_rate(_In_ struct asynccom_port *port, unsigned rate);
NTSTATUS			asynccom_port_set_isochronous(_In_ struct asynccom_port *port, int mode);
NTSTATUS			asynccom_port_set_pretend_xonoff(_In_ struct asynccom_port *port, BOOLEAN onoff);
NTSTATUS			asynccom_port_set_break(_In_ struct asynccom_port *port, BOOLEAN onoff);
NTSTATUS			asynccom_port_program_firmware(_In_ struct asynccom_port *port, unsigned char *firmware_line, size_t data_size);
void				asynccom_port_set_clock_bits(_In_ struct asynccom_port *port, unsigned char *clock_data);

#endif