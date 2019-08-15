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

#ifndef ASYNCCOM_DEVICE_H
#define ASYNCCOM_DEVICE_H
#include <ntddk.h>
#include <wdf.h>
#include <defines.h>
#include "trace.h"

struct asynccom_port			*asynccom_port_new(WDFDRIVER Driver, IN PWDFDEVICE_INIT DeviceInit);

EVT_WDF_DRIVER_DEVICE_ADD		AsyncComEvtDeviceAdd;
EVT_WDF_DEVICE_D0_ENTRY			AsyncComEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT			AsyncComEvtDeviceD0Exit;
EVT_WDF_DEVICE_PREPARE_HARDWARE AsyncComEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE AsyncComEvtDeviceReleaseHardware;
EVT_WDF_IO_QUEUE_IO_WRITE		AsyncComEvtIoWrite;
EVT_WDF_IO_QUEUE_IO_READ		AsyncComEvtIoRead;
EVT_WDF_DEVICE_CONTEXT_CLEANUP	AsyncComEvtDeviceContextCleanup;
EVT_WDF_DEVICE_FILE_CREATE		AsyncComEvtDeviceFileCreate;
EVT_WDF_FILE_CLOSE				AsyncComEvtFileClose;

NTSTATUS						setup_spinlocks(_In_ struct asynccom_port *port);
NTSTATUS						setup_dpc(_In_ struct asynccom_port *port);
NTSTATUS						setup_timer(_In_ struct asynccom_port *port);
NTSTATUS						setup_queues(_In_ struct asynccom_port *port);

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS						SelectInterfaces(_In_ struct asynccom_port *port);
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS						OsrFxSetPowerPolicy(_In_ struct asynccom_port *port);
NTSTATUS						SerialReadSymName(_In_ struct asynccom_port *port, __out_bcount(*SizeOfRegName) PWSTR RegName, __inout PUSHORT SizeOfRegName);
BOOLEAN							SerialGetRegistryKeyValue(_In_ struct asynccom_port *port, __in PCWSTR Name, OUT PULONG Value);
NTSTATUS						SerialDoExternalNaming(_In_ struct asynccom_port *port);
BOOLEAN							SerialGetFdoRegistryKeyValue(IN PWDFDEVICE_INIT DeviceInit, __in PCWSTR Name, OUT PULONG Value);

#endif
