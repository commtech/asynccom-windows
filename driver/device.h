/*
	Copyright (C) 2019  Commtech, Inc.

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

#ifndef ASYNCCOM_DEVICE_H
#define ASYNCCOM_DEVICE_H
#include <ntddk.h>
#include <wdf.h>
#include <defines.h>
#include "trace.h"

struct asynccom_port *asynccom_port_new(WDFDRIVER Driver, IN PWDFDEVICE_INIT DeviceInit);

EVT_WDF_DRIVER_DEVICE_ADD AsyncComEvtDeviceAdd;
EVT_WDF_DEVICE_D0_ENTRY AsyncComEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT AsyncComEvtDeviceD0Exit;
EVT_WDF_DEVICE_PREPARE_HARDWARE AsyncComEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE AsyncComEvtDeviceReleaseHardware;
EVT_WDF_IO_QUEUE_IO_WRITE AsyncComEvtIoWrite;
EVT_WDF_IO_QUEUE_IO_READ AsyncComEvtIoRead;

NTSTATUS setup_spinlocks(_In_ struct asynccom_port *port);
NTSTATUS setup_dpc(_In_ struct asynccom_port *port);
NTSTATUS setup_timer(_In_ struct asynccom_port *port);
NTSTATUS setup_queues(_In_ struct asynccom_port *port);

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS SelectInterfaces(_In_ struct asynccom_port *port);
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS OsrFxSetPowerPolicy(_In_ struct asynccom_port *port);
NTSTATUS SerialReadSymName(_In_ struct asynccom_port *port, __out_bcount(*SizeOfRegName) PWSTR RegName, __inout PUSHORT SizeOfRegName);
BOOLEAN SerialGetRegistryKeyValue(_In_ struct asynccom_port *port, __in PCWSTR Name, OUT PULONG Value);
NTSTATUS SerialDoExternalNaming(_In_ struct asynccom_port *port);
BOOLEAN SerialGetFdoRegistryKeyValue(IN PWDFDEVICE_INIT DeviceInit, __in PCWSTR Name, OUT PULONG Value);


#endif
