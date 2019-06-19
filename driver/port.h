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

#ifndef ASYNCCOM_PORT_H
#define ASYNCCOM_PORT_H

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

NTSTATUS			asynccom_port_set_isochronous(_In_ struct asynccom_port *port, int mode);
int					asynccom_port_get_isochronous(_In_ struct asynccom_port *port);

NTSTATUS			asynccom_port_set_xonoff(_In_ struct asynccom_port *port, BOOLEAN onoff);
NTSTATUS			asynccom_port_set_sample_rate(_In_ struct asynccom_port *port, unsigned rate);
NTSTATUS			asynccom_port_set_isochronous(_In_ struct asynccom_port *port, int mode);
NTSTATUS			asynccom_port_set_break(_In_ struct asynccom_port *port, BOOLEAN onoff);
NTSTATUS			asynccom_port_program_firmware(_In_ struct asynccom_port *port, unsigned char *firmware_line, size_t data_size);
void				asynccom_port_set_clock_bits(_In_ struct asynccom_port *port, unsigned char *clock_data);
NTSTATUS            asynccom_port_get_status(_In_ struct asynccom_port *port, PSERIAL_STATUS settings);
ULONG               asynccom_port_modem_status(_In_ struct asynccom_port *port);
void				asynccom_port_set_memory_cap(struct asynccom_port *port, struct asynccom_memory_cap *value);

#endif