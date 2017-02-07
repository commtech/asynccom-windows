/*
	Copyright (C) 2016  Commtech, Inc.

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
#include <devpkey.h>
#include "utils.h"
#include "isr.h"
#include <port.h>
#include <frame.h>
#include <device.h>

#if defined(EVENT_TRACING)
#include "device.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AsyncComEvtDeviceAdd)
#pragma alloc_text(PAGE, AsyncComEvtDevicePrepareHardware)
#pragma alloc_text(PAGE, AsyncComEvtDeviceD0Exit)
#pragma alloc_text(PAGE, SelectInterfaces)
#pragma alloc_text(PAGE, OsrFxSetPowerPolicy)
#endif

NTSTATUS AsyncComEvtDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)
{
	struct asynccom_port *port = 0;

	PAGED_CODE();

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "%s: Entering.\n", __FUNCTION__);
	port = asynccom_port_new(Driver, DeviceInit);
	if (!port)
	{
		TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "%s: Error: no valid port created!\n", __FUNCTION__);
		return STATUS_INTERNAL_ERROR;
	}
	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "%s: Exiting.\n", __FUNCTION__);
	return STATUS_SUCCESS;
}

struct asynccom_port *asynccom_port_new(WDFDRIVER Driver, IN PWDFDEVICE_INIT DeviceInit)
{
	NTSTATUS							status = STATUS_SUCCESS;
	WDF_PNPPOWER_EVENT_CALLBACKS        pnpPowerCallbacks;
	WDF_OBJECT_ATTRIBUTES               attributes;
	WDFDEVICE                           device;
	WDF_DEVICE_PNP_CAPABILITIES         pnpCaps;
	WDF_IO_QUEUE_CONFIG					queue_config;
	WDF_DEVICE_STATE					device_state;
	struct asynccom_port				*port = 0;
	static ULONG						instance = 0;
	ULONG								port_num = 0;

	UNREFERENCED_PARAMETER(Driver);
	UNREFERENCED_PARAMETER(DeviceInit);
	PAGED_CODE();
    DECLARE_UNICODE_STRING_SIZE(device_name, DEVICE_OBJECT_NAME_LENGTH);

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "%s: Entering.\n", __FUNCTION__);
	
	port_num = instance;
	instance++;

    status = RtlUnicodeStringPrintf(&device_name, L"%ws%d", L"\\Device\\ASYNCCOM", port_num);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: RtlUnicodeStringPrintf failed %!STATUS!", __FUNCTION__, status);
		return 0;
	}
	status = WdfDeviceInitAssignName(DeviceInit, &device_name);
	if (!NT_SUCCESS(status)) {
		WdfDeviceInitFree(DeviceInit);
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: WdfDeviceInitAssignName failed %!STATUS!", __FUNCTION__, status);
		return 0;
	}
	
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, REQUEST_CONTEXT);
	WdfDeviceInitSetRequestAttributes(DeviceInit, &attributes);

	// https://msdn.microsoft.com/en-us/library/windows/hardware/ff563667(v=vs.85).aspx
	// Note: When using SDDL for device objects, your driver must link against Wdmsec.lib.
	status = WdfDeviceInitAssignSDDLString(DeviceInit, &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RWX_RES_RWX);
	if (!NT_SUCCESS(status)) {
		WdfDeviceInitFree(DeviceInit);
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "WdfDeviceInitAssignSDDLString failed %!STATUS!", status);
		return 0;
	}

	WdfDeviceInitSetExclusive(DeviceInit, TRUE);
	WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_SERIAL_PORT);

	WdfDeviceInitSetRequestAttributes(DeviceInit, &attributes);

	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
	pnpPowerCallbacks.EvtDevicePrepareHardware = AsyncComEvtDevicePrepareHardware;
	pnpPowerCallbacks.EvtDeviceReleaseHardware = AsyncComEvtDeviceReleaseHardware;
	pnpPowerCallbacks.EvtDeviceD0Entry = AsyncComEvtDeviceD0Entry;
	pnpPowerCallbacks.EvtDeviceD0Exit =  AsyncComEvtDeviceD0Exit;
	WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

	WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, ASYNCCOM_PORT);
	attributes.EvtCleanupCallback = OsrFxEvtDriverContextCleanup;
	attributes.SynchronizationScope = WdfSynchronizationScopeDevice;
	status = WdfDeviceCreate(&DeviceInit, &attributes, &device);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: WdfDeviceCreate failed with Status code %!STATUS!\n", __FUNCTION__, status);
		return 0;
	}
	
	port = GetPortContext(device);
	port->device = device;
	port->driver_object = WdfDriverWdmGetDriverObject(Driver);
	port->device_object = WdfDeviceWdmGetDeviceObject(port->device);
	port->memory_cap.input = 1000000;
	// The output cap isn't actually used, but why not.
	port->memory_cap.output = 1000000;
	port->current_acr = 0x00;
	port->istream = asynccom_frame_new(port);

	WDF_DEVICE_STATE_INIT(&device_state);
	device_state.DontDisplayInUI = WdfFalse;
	WdfDeviceSetDeviceState(port->device, &device_state);

	WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCaps);
	pnpCaps.SurpriseRemovalOK = WdfTrue;
	pnpCaps.UniqueID = WdfTrue;
	pnpCaps.UINumber = port_num; //-1 is worth trying.
	WdfDeviceSetPnpCapabilities(port->device, &pnpCaps);

	
	WDF_IO_QUEUE_CONFIG_INIT(&queue_config, WdfIoQueueDispatchSequential);
	queue_config.EvtIoDeviceControl = AsyncComEvtIoDeviceControl;

	__analysis_assume(queue_config.EvtIoStop != 0);
	status = WdfIoQueueCreate(port->device, &queue_config, WDF_NO_OBJECT_ATTRIBUTES, &port->ioctl_queue);
	__analysis_assume(queue_config.EvtIoStop == 0);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: WdfIoQueueCreate failed %!STATUS!", __FUNCTION__, status);
		return 0;
	}
	status = WdfDeviceConfigureRequestDispatching(port->device, port->ioctl_queue, WdfRequestTypeDeviceControl);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: WdfDeviceConfigureRequestDispatching failed %!STATUS!", __FUNCTION__, status);
		return 0;
	}

	WDF_IO_QUEUE_CONFIG_INIT(&queue_config, WdfIoQueueDispatchSequential);
	queue_config.EvtIoWrite = AsyncComEvtIoWrite;
	queue_config.EvtIoStop  = AsyncComEvtIoStop;

	status = WdfIoQueueCreate(port->device, &queue_config, WDF_NO_OBJECT_ATTRIBUTES, &port->write_queue);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: WdfIoQueueCreate failed %!STATUS!", __FUNCTION__, status);
		return 0;
	}
	status = WdfDeviceConfigureRequestDispatching(port->device, port->write_queue, WdfRequestTypeWrite);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: WdfDeviceConfigureRequestDispatching failed %!STATUS!", __FUNCTION__, status);
		return 0;
	}

	WDF_IO_QUEUE_CONFIG_INIT(&queue_config, WdfIoQueueDispatchSequential);
	queue_config.EvtIoRead = AsyncComEvtIoRead;
	queue_config.EvtIoStop = AsyncComEvtIoStop;

	status = WdfIoQueueCreate(port->device, &queue_config, WDF_NO_OBJECT_ATTRIBUTES, &port->read_queue);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: WdfIoQueueCreate failed %!STATUS!", __FUNCTION__, status);
		return 0;
	}
	status = WdfDeviceConfigureRequestDispatching(port->device, port->read_queue, WdfRequestTypeRead);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: WdfDeviceConfigureRequestDispatching failed %!STATUS!", __FUNCTION__, status);
		return 0;
	}
	
	WDF_IO_QUEUE_CONFIG_INIT(&queue_config, WdfIoQueueDispatchManual);
	status = WdfIoQueueCreate(port->device, &queue_config, WDF_NO_OBJECT_ATTRIBUTES, &port->read_queue2);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: WdfIoQueueCreate failed %!STATUS!", __FUNCTION__, status);
		return 0;
	}

	status = WdfDeviceCreateDeviceInterface(device, (LPGUID)&GUID_DEVINTERFACE_COMPORT, NULL);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: WdfDeviceCreateDeviceInterface failed  %!STATUS!\n", __FUNCTION__, status);
		return 0;
	}
    /*
	RtlInitEmptyUnicodeString(&dos_name, dos_name_buffer, sizeof(dos_name_buffer));
	status = RtlUnicodeStringPrintf(&dos_name, L"\\DosDevices\\ASYNCCOM%i", port_num);
	if (!NT_SUCCESS(status)) {
		WdfObjectDelete(port->device);
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: RtlUnicodeStringPrintf failed %!STATUS!", __FUNCTION__, status);
		return 0;
	}

	status = WdfDeviceCreateSymbolicLink(port->device, &dos_name);
	if (!NT_SUCCESS(status)) {
		WdfObjectDelete(port->device);
		TraceEvents(TRACE_LEVEL_WARNING, DBG_PNP, "%s: WdfDeviceCreateSymbolicLink failed %!STATUS!", __FUNCTION__, status);
		return 0;
	}
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "%s: device name %wZ!", __FUNCTION__, &dos_name);
	*/
	
	status = setup_spinlocks(port);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: Failed to set up spinlocks!  %!STATUS!", __FUNCTION__, status);
		return 0;
	}

	status = setup_dpc(port);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: Failed to set up DPCs!  %!STATUS!", __FUNCTION__, status);
		return 0;
	}
	
	status = setup_timer(port);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: Failed to set up timers! %!STATUS!", __FUNCTION__, status);
		return 0;
	}
	
	// To be added for COM compatibility. Currently errors.
	status = SerialDoExternalNaming(port);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: Failed to set up the COM port!  %!STATUS!", __FUNCTION__, status);
		return 0;
	}
	port->timeouts.ReadIntervalTimeout = 25;
	port->timeouts.ReadTotalTimeoutConstant = 50;
	port->timeouts.ReadTotalTimeoutMultiplier = 25;
	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP, "%s: Exiting.", __FUNCTION__);
	return port;
	
}

NTSTATUS setup_timer(_In_ struct asynccom_port *port)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_TIMER_CONFIG timer_config;
	WDF_OBJECT_ATTRIBUTES timer_attributes;

	WDF_TIMER_CONFIG_INIT(&timer_config, serial_read_timeout);
	timer_config.AutomaticSerialization = TRUE;
	
	WDF_OBJECT_ATTRIBUTES_INIT(&timer_attributes);
	timer_attributes.ParentObject = port->device;
	status = WdfTimerCreate(&timer_config, &timer_attributes, &port->read_request_total_timer);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: Failed to set up reaad_request_total_timer! %!STATUS!", __FUNCTION__, status);
		return status;
	}

	WDF_TIMER_CONFIG_INIT(&timer_config, serial_read_timeout);
	timer_config.AutomaticSerialization = TRUE;

	WDF_OBJECT_ATTRIBUTES_INIT(&timer_attributes);
	timer_attributes.ParentObject = port->device;
	status = WdfTimerCreate(&timer_config, &timer_attributes, &port->read_request_interval_timer);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: Failed to set up reaad_request_total_timer! %!STATUS!", __FUNCTION__, status);
		return status;
	}
	return status;
}

NTSTATUS setup_port(_In_ struct asynccom_port *port)
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT32 old_efr = 0;

	
	// This sets the port into FIFO mode. Apparently sometimes it needs to be done twice.
	// This should be fixed in the firmware.
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS, 0x02, 0x1);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "%s: Error: Can't enable the FIFO!\n", __FUNCTION__);
		return status;
	}
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS, 0x02, 0x1);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "%s: Error: Can't enable the FIFO!\n", __FUNCTION__);
		return status;
	}
	// This sets the port into asynchronous mode. 
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS, FPGA_FCR_OFFSET, 0x01000000);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "%s: Error: Can't set port to ASYNC!\n", __FUNCTION__);
		return status;
	}
	asynccom_port_set_clock_rate(port, 1843200);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "%s: Error: can't set clock!\n", __FUNCTION__);
		return status;
	}
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, FCR_OFFSET, 0x3);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "%s: Error: Can't set the 950 FCR!\n", __FUNCTION__);
		return status;
	}
	
	old_efr = asynccom_port_get_650_register(port, EFR_OFFSET);
	asynccom_port_set_650_register(port, EFR_OFFSET, old_efr | 0x10);
	status = asynccom_port_set_sample_rate(port, 16);
	status = asynccom_port_set_divisor(port, 1);
	asynccom_port_set_spr_register(port, ACR_OFFSET, 0x00);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "%s: Error: can't set sample rate!\n", __FUNCTION__);
		return status;
	}
	status = asynccom_port_set_echo_cancel(port, 0);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "%s: Error: can't set echo cancel!\n", __FUNCTION__);
		return status;
	}
	status = asynccom_port_set_isochronous(port, -1);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "%s: Error: can't set isochronous!\n", __FUNCTION__);
		return status;
	}
	
	status = asynccom_port_set_frame_length(port, 1);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "%s: Error: can't set frame length!\n", __FUNCTION__);
		return status;
	}
	
	status = asynccom_port_set_9bit(port, 0);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "%s: Error: can't set 9-bit mode!\n", __FUNCTION__);
		return status;
	}
	status = asynccom_port_set_register_uint32(port, FPGA_UPPER_ADDRESS + ASYNCCOM_UPPER_OFFSET, LCR_OFFSET, 0x3);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "%s: Error: Can't set port to use 8 bits!\n", __FUNCTION__);
		return status;
	}
	
	return status;
}

NTSTATUS setup_spinlocks(_In_ struct asynccom_port *port)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_OBJECT_ATTRIBUTES attributes;

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.ParentObject = port->device;

	status = WdfSpinLockCreate(&attributes, &port->istream_spinlock);
	if (!NT_SUCCESS(status)) {
		WdfObjectDelete(port->device);
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "WdfSpinLockCreate failed %!STATUS!", status);
		return status;
	}

	return STATUS_SUCCESS;
}

NTSTATUS setup_dpc(_In_ struct asynccom_port *port)
{
	NTSTATUS							status = STATUS_SUCCESS;
	WDF_DPC_CONFIG						dpcConfig;
	WDF_OBJECT_ATTRIBUTES				dpcAttributes;

	WDF_OBJECT_ATTRIBUTES_INIT(&dpcAttributes);
	dpcAttributes.ParentObject = port->device;

	WDF_DPC_CONFIG_INIT(&dpcConfig, &AsynccomProcessRead);
	dpcConfig.AutomaticSerialization = TRUE;

	status = WdfDpcCreate(&dpcConfig, &dpcAttributes, &port->process_read_dpc);
	if (!NT_SUCCESS(status)) {
		WdfObjectDelete(port->device);
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: process_read_dpc failed %!STATUS!", __FUNCTION__, status);
		return status;
	}

	return STATUS_SUCCESS;
}

NTSTATUS AsyncComEvtDevicePrepareHardware(WDFDEVICE Device, WDFCMRESLIST ResourceList, WDFCMRESLIST ResourceListTranslated)
{
    NTSTATUS                            status = STATUS_SUCCESS;
	
    WDF_USB_DEVICE_INFORMATION          deviceInfo;
    ULONG                               wait_wake_enable;
	struct asynccom_port					*port = 0;
	WDF_USB_CONTINUOUS_READER_CONFIG	readerConfig;

    UNREFERENCED_PARAMETER(ResourceList);
    UNREFERENCED_PARAMETER(ResourceListTranslated);

    PAGED_CODE();
	wait_wake_enable = FALSE;
    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP, "%s: Entering.\n", __FUNCTION__);

	port = GetPortContext(Device);
	return_val_if_untrue(port, STATUS_UNSUCCESSFUL);
    if (port->usb_device == NULL) {
        WDF_USB_DEVICE_CREATE_CONFIG config;

        WDF_USB_DEVICE_CREATE_CONFIG_INIT(&config, USBD_CLIENT_CONTRACT_VERSION_602);

        status = WdfUsbTargetDeviceCreateWithParameters(Device, &config, WDF_NO_OBJECT_ATTRIBUTES, &port->usb_device);
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: WdfUsbTargetDeviceCreateWithParameters failed with Status code %!STATUS!\n", __FUNCTION__, status);
            return status;
        }
    }

    WDF_USB_DEVICE_INFORMATION_INIT(&deviceInfo);

    status = WdfUsbTargetDeviceRetrieveInformation(port->usb_device, &deviceInfo);
    if (NT_SUCCESS(status)) {
		port->usb_traits = deviceInfo.Traits;
		wait_wake_enable = deviceInfo.Traits & WDF_USB_DEVICE_TRAIT_REMOTE_WAKE_CAPABLE;
        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "%s: IsDeviceHighSpeed: %s\n", __FUNCTION__, (deviceInfo.Traits & WDF_USB_DEVICE_TRAIT_AT_HIGH_SPEED) ? "TRUE" : "FALSE");
        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "%s: IsDeviceSelfPowered: %s\n", __FUNCTION__, (deviceInfo.Traits & WDF_USB_DEVICE_TRAIT_SELF_POWERED) ? "TRUE" : "FALSE");
		TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "%s: IsDeviceRemoteWakeable: %s\n", __FUNCTION__, wait_wake_enable ? "TRUE" : "FALSE");
    }
    else  {
        port->usb_traits = 0;
    }

    status = SelectInterfaces(port);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: SelectInterfaces failed 0x%x\n", __FUNCTION__, status);
        return status;
    }

	if (wait_wake_enable) {
        status = OsrFxSetPowerPolicy(port);
        if (!NT_SUCCESS (status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: OsrFxSetPowerPolicy failed  %!STATUS!\n", __FUNCTION__, status);
            return status;
        }
    }
	
	WDF_USB_CONTINUOUS_READER_CONFIG_INIT(&readerConfig, asynccom_port_received_data, port, 512);
	readerConfig.NumPendingReads = 1;
	readerConfig.EvtUsbTargetPipeReadersFailed = FX3EvtReadFailed;
	status = WdfUsbTargetPipeConfigContinuousReader(port->data_read_pipe, &readerConfig);
	
	port->pending_oframe = 0;
	status = setup_port(port);

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP, "%s: Exiting.\n", __FUNCTION__);
    return status;
}

NTSTATUS AsyncComEvtDeviceReleaseHardware(WDFDEVICE Device, WDFCMRESLIST ResourcesTranslated)
{
	NTSTATUS status = STATUS_SUCCESS;
	UNREFERENCED_PARAMETER(Device);
	UNREFERENCED_PARAMETER(ResourcesTranslated);
	
	struct asynccom_port *port = 0;
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_POWER, "%s: Entering\n", __FUNCTION__);
	UNREFERENCED_PARAMETER(ResourcesTranslated);
	port = GetPortContext(Device);
	return_val_if_untrue(port, 0);

	WdfSpinLockAcquire(port->istream_spinlock);
	asynccom_frame_delete(port->istream);
	WdfSpinLockRelease(port->istream_spinlock);
	
	return status;
}

NTSTATUS AsyncComEvtDeviceD0Entry(WDFDEVICE Device, WDF_POWER_DEVICE_STATE PreviousState)
{
	NTSTATUS                status = STATUS_SUCCESS;
	struct asynccom_port					*port = 0;

	UNREFERENCED_PARAMETER(PreviousState);
	UNREFERENCED_PARAMETER(Device);

	PAGED_CODE();
	port = GetPortContext(Device);
	return_val_if_untrue(port, STATUS_UNSUCCESSFUL);

	status = WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(port->data_read_pipe));

	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, DBG_POWER, "%!FUNC! Could not start data_read_pipe: failed 0x%x", status);
	}

	return status; 
}

NTSTATUS AsyncComEvtDeviceD0Exit(WDFDEVICE Device, WDF_POWER_DEVICE_STATE TargetState)
{
	UNREFERENCED_PARAMETER(Device);
	UNREFERENCED_PARAMETER(TargetState);
	
	struct asynccom_port *port = 0;
	UNREFERENCED_PARAMETER(TargetState);
	PAGED_CODE();
	port = GetPortContext(Device);

	return_val_if_untrue(port, STATUS_SUCCESS);
	WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(port->data_read_pipe), WdfIoTargetCancelSentIo);
    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "%s: Exiting.\n", __FUNCTION__);
	
    return STATUS_SUCCESS;
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS OsrFxSetPowerPolicy(_In_ struct asynccom_port *port)
{
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
    WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS wakeSettings;
    NTSTATUS    status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    // Init the idle policy structure.
    //
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleUsbSelectiveSuspend);
    idleSettings.IdleTimeout = 10000; // 10-sec

    status = WdfDeviceAssignS0IdleSettings(port->device, &idleSettings);
    if ( !NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: WdfDeviceSetPowerPolicyS0IdlePolicy failed %x\n", __FUNCTION__, status);
        return status;
    }

    //
    // Init wait-wake policy structure.
    //
    WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_INIT(&wakeSettings);

    status = WdfDeviceAssignSxWakeSettings(port->device, &wakeSettings);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: WdfDeviceAssignSxWakeSettings failed %x\n", __FUNCTION__, status);
        return status;
    }

    return status;
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS SelectInterfaces(_In_ struct asynccom_port *port)
{
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS configParams;
    NTSTATUS                            status = STATUS_SUCCESS;
    WDFUSBPIPE                          pipe;
    WDF_USB_PIPE_INFORMATION            pipeInfo;
    UCHAR                               index;
    UCHAR                               numberConfiguredPipes;
    PAGED_CODE();


    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE( &configParams);

    status = WdfUsbTargetDeviceSelectConfig(port->usb_device, WDF_NO_OBJECT_ATTRIBUTES, &configParams);
    if(!NT_SUCCESS(status)) {

        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: WdfUsbTargetDeviceSelectConfig failed %!STATUS! \n", __FUNCTION__, status);
        return status;
    }

    port->usb_interface = configParams.Types.SingleInterface.ConfiguredUsbInterface;
    numberConfiguredPipes = configParams.Types.SingleInterface.NumberConfiguredPipes;

    for(index=0; index < numberConfiguredPipes; index++) {
		
        WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);

        pipe = WdfUsbInterfaceGetConfiguredPipe(port->usb_interface, index, &pipeInfo);
        WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pipe);

		if (DATA_READ_ENDPOINT == pipeInfo.EndpointAddress && WdfUsbTargetPipeIsInEndpoint(pipe)) {
			TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL, "%s: data_read_pipe is 0x%p\n", __FUNCTION__, pipe);
			port->data_read_pipe = pipe;
		}

		if (DATA_WRITE_ENDPOINT == pipeInfo.EndpointAddress && WdfUsbTargetPipeIsOutEndpoint(pipe)) {
			TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL, "%s: data_write_pipe is 0x%p\n", __FUNCTION__, pipe);
			port->data_write_pipe = pipe;
		}

		if (REGISTER_READ_ENDPOINT == pipeInfo.EndpointAddress && WdfUsbTargetPipeIsInEndpoint(pipe)) {
			TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL, "%s: register_read_pipe is 0x%p\n", __FUNCTION__, pipe);
			port->register_read_pipe = pipe;
		}

		if (REGISTER_WRITE_ENDPOINT == pipeInfo.EndpointAddress && WdfUsbTargetPipeIsOutEndpoint(pipe)) {
			TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL, "%s: register_write_pipe is 0x%p\n", __FUNCTION__, pipe);
			port->register_write_pipe = pipe;
		}

    }

    if(!(port->data_read_pipe
         && port->data_write_pipe 
		 && port->register_read_pipe
		 && port->register_write_pipe )) {
        status = STATUS_INVALID_DEVICE_STATE;
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "%s: Device is not CONFIGURED properly %!STATUS!\n", __FUNCTION__,status);
        return status;
    }
	
    return status;
}

NTSTATUS SerialReadSymName(_In_ struct asynccom_port *port, __out_bcount(*SizeOfRegName) PWSTR RegName, __inout PUSHORT SizeOfRegName)
{
	NTSTATUS status;
	WDFKEY hKey;
	UNICODE_STRING value;
	UNICODE_STRING valueName;
	USHORT requiredLength;

	PAGED_CODE();

	value.Buffer = RegName;
	value.MaximumLength = *SizeOfRegName;
	value.Length = 0;

	status = WdfDeviceOpenRegistryKey(port->device,
		PLUGPLAY_REGKEY_DEVICE,
		STANDARD_RIGHTS_ALL,
		WDF_NO_OBJECT_ATTRIBUTES,
		&hKey);

	if (NT_SUCCESS(status)) {
		//
		// Fetch PortName which contains the suggested REG_SZ symbolic name.
		//


		RtlInitUnicodeString(&valueName, L"PortName");

		status = WdfRegistryQueryUnicodeString(hKey,
			&valueName,
			&requiredLength,
			&value);

		if (!NT_SUCCESS(status)) {
			//
			// This is for PCMCIA which currently puts the name under Identifier.
			//

			RtlInitUnicodeString(&valueName, L"Identifier");
			status = WdfRegistryQueryUnicodeString(hKey,
				&valueName,
				&requiredLength,
				&value);

			if (!NT_SUCCESS(status)) {
				//
				// Hmm.  Either we have to pick a name or bail...
				//
				TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "Getting PortName/Identifier failed - %x\n", status);
			}
		}

		WdfRegistryClose(hKey);
	}

	if (NT_SUCCESS(status)) {
		//
		// NULL terminate the string and return number of characters in the string.
		//
		if (value.Length > *SizeOfRegName - sizeof(WCHAR)) {
			return STATUS_UNSUCCESSFUL;
		}

		*SizeOfRegName = value.Length;
		RegName[*SizeOfRegName / sizeof(WCHAR)] = UNICODE_NULL;
	}
	return status;
}

BOOLEAN SerialGetRegistryKeyValue(_In_ struct asynccom_port *port, __in PCWSTR Name, OUT PULONG Value)
{
	WDFKEY      hKey = NULL;
	NTSTATUS    status;
	BOOLEAN     retValue = FALSE;
	UNICODE_STRING valueName;

	PAGED_CODE();

	*Value = 0;

	status = WdfDeviceOpenRegistryKey(port->device,
		PLUGPLAY_REGKEY_DEVICE,
		STANDARD_RIGHTS_ALL,
		WDF_NO_OBJECT_ATTRIBUTES,
		&hKey);

	if (NT_SUCCESS(status)) {

		RtlInitUnicodeString(&valueName, Name);

		status = WdfRegistryQueryULong(hKey,
			&valueName,
			Value);

		if (NT_SUCCESS(status)) {
			retValue = TRUE;
		}

		WdfRegistryClose(hKey);
	}

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP, "<--SerialGetRegistryKeyValue %ws %d \n", Name, *Value);

	return retValue;
}

#define SYMBOLIC_NAME_LENGTH 128
NTSTATUS SerialDoExternalNaming(_In_ struct asynccom_port *port)
{
	NTSTATUS status = STATUS_SUCCESS;
	WCHAR pRegName[SYMBOLIC_NAME_LENGTH];
	USHORT nameSize = sizeof(pRegName);
	WDFSTRING stringHandle = NULL;
	WDF_OBJECT_ATTRIBUTES attributes;
	DECLARE_UNICODE_STRING_SIZE(symbolicLinkName, SYMBOLIC_NAME_LENGTH);

	PAGED_CODE();

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.ParentObject = port->device;
	status = WdfStringCreate(NULL, &attributes, &stringHandle);
	if (!NT_SUCCESS(status)) {
		goto SerialDoExternalNamingError;
	}

	status = WdfDeviceRetrieveDeviceName(port->device, stringHandle);
	if (!NT_SUCCESS(status)) {
		goto SerialDoExternalNamingError;
	}

	WdfStringGetUnicodeString(stringHandle, &port->device_name);

	SerialGetRegistryKeyValue(port, L"SerialSkipExternalNaming", &port->skip_naming);

	if (port->skip_naming) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "Skipping external naming due to registry settings\n");
		return STATUS_SUCCESS;
	}

	status = SerialReadSymName(port, pRegName, &nameSize);
	if (!NT_SUCCESS(status)) {
		goto SerialDoExternalNamingError;
	}

	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "DosName is %ws\n", pRegName);

	status = RtlUnicodeStringPrintf(&symbolicLinkName, L"%ws%ws", L"\\DosDevices\\", pRegName);
	if (!NT_SUCCESS(status)) {
		goto SerialDoExternalNamingError;
	}
	// This fails. I have no idea why. It fails with STATUS_INVALID_DEVICE_REQUEST 0xc0000010.
    
	status = WdfDeviceCreateSymbolicLink(port->device, &symbolicLinkName);
	if (!NT_SUCCESS(status)) {

		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "Couldn't create the symbolic link for port %wZ\n", &symbolicLinkName);

		goto SerialDoExternalNamingError;

	}
    
	port->created_symbolic_link = TRUE;

	status = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP, SERIAL_DEVICE_MAP, port->device_name.Buffer, REG_SZ, pRegName, nameSize);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "Couldn't create the device map entry for port %ws\n", port->device_name.Buffer);
		goto SerialDoExternalNamingError;
	}

	port->created_serial_comm_entry = TRUE;

	status = WdfDeviceCreateDeviceInterface(port->device, (LPGUID)&GUID_DEVINTERFACE_COMPORT, NULL);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "Couldn't register class association for port %wZ\n", &port->device_name);
		goto SerialDoExternalNamingError;
	}

	return status;

SerialDoExternalNamingError:;

	port->device_name.Buffer = NULL;

	if (port->created_serial_comm_entry) {
		RtlDeleteRegistryValue(RTL_REGISTRY_DEVICEMAP, SERIAL_DEVICE_MAP, port->device_name.Buffer);
	}

	if (stringHandle) {
		WdfObjectDelete(stringHandle);
	}
    TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "SerialDoExternalNamingError!\n");
	return status;
}

BOOLEAN SerialGetFdoRegistryKeyValue(IN PWDFDEVICE_INIT DeviceInit, __in PCWSTR Name, OUT PULONG Value)
{
	WDFKEY      hKey = NULL;
	NTSTATUS    status;
	BOOLEAN     retValue = FALSE;
	UNICODE_STRING valueName;

	PAGED_CODE();

	*Value = 0;

	status = WdfFdoInitOpenRegistryKey(DeviceInit,
		PLUGPLAY_REGKEY_DEVICE,
		STANDARD_RIGHTS_ALL,
		WDF_NO_OBJECT_ATTRIBUTES,
		&hKey);

	if (NT_SUCCESS(status)) {

		RtlInitUnicodeString(&valueName, Name);

		status = WdfRegistryQueryULong(hKey, &valueName, Value);

		if (NT_SUCCESS(status)) {
			retValue = TRUE;
		}
		else TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP, "Didn't set the value??");

		WdfRegistryClose(hKey);
	}

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP,
		"<--SerialGetFdoRegistryKeyValue %ws %d \n",
		Name, *Value);

	return retValue;
}

// This will be used to implement COM compatibility. Currently C
/*
NTSTATUS create_com_port(_In_ struct asynccom_port *port)
{
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING com_name;
	WCHAR com_name_buffer[50];
	HANDLE key_handle;

	RtlInitEmptyUnicodeString(&com_name, com_name_buffer, sizeof(com_name_buffer));
	status = IoOpenDeviceRegistryKey(port->device_object, PLUGPLAY_REGKEY_DEVICE, STANDARD_RIGHTS_READ, &key_handle);
	if (!NT_SUCCESS(status)) return status;
	status = GetRegistryKeyValue(key_handle, L"PortName", sizeof(L"PortName"), com_name_buffer, 50);
	if (!NT_SUCCESS(status)) { 
		ZwClose(key_handle); 
		return status; 
	}
	ZwClose(key_handle);
	status = IoCreateSymbolicLink(&com_name, &port->device_name);

	return status;
}
*/