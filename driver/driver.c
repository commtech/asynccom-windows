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
#include <device.h>

#if defined(EVENT_TRACING)
#include "driver.tmh"
#else
ULONG DebugLevel = TRACE_LEVEL_INFORMATION;
ULONG DebugFlag = 0xff;
#endif


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, OsrFxEvtDriverContextCleanup)
#endif

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	WDF_DRIVER_CONFIG       config;
    NTSTATUS                status = STATUS_SUCCESS;
    WDF_OBJECT_ATTRIBUTES   attributes;

    WPP_INIT_TRACING( DriverObject, RegistryPath );
    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT, "Async Com Driver\n");
    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT, "Built %s %s\n", __DATE__, __TIME__);
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT, "Copyright (c) 2016, Fastcom.");
  
	ExInitializeDriverRuntime(DrvRtPoolNxOptIn);
	WDF_DRIVER_CONFIG_INIT(&config, AsyncComEvtDeviceAdd);

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = OsrFxEvtDriverContextCleanup;

    status = WdfDriverCreate(DriverObject, RegistryPath, &attributes, &config, WDF_NO_HANDLE);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "WdfDriverCreate failed with status 0x%x\n", status);
        WPP_CLEANUP(NULL);
    }
	
    return status;
}

VOID OsrFxEvtDriverContextCleanup(WDFOBJECT Driver)
{
    PAGED_CODE ();

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT,  "%s: Entering.\n", __FUNCTION__);

	WPP_CLEANUP(NULL);
    UNREFERENCED_PARAMETER(Driver);
	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "%s: Exiting.\n", __FUNCTION__);
}

#if !defined(EVENT_TRACING)

VOID
TraceEvents (
    _In_ ULONG DebugPrintLevel,
    _In_ ULONG DebugPrintFlag,
    _Printf_format_string_
    _In_ PCSTR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print for the sample driver.

Arguments:

    DebugPrintLevel - print level between 0 and 3, with 3 the most verbose
    DebugPrintFlag - message mask
    DebugMessage - format string of the message to print
    ... - values used by the format string

Return Value:

    None.

 --*/
 {
#if DBG
#define     TEMP_BUFFER_SIZE        1024
    va_list    list;
    CHAR       debugMessageBuffer[TEMP_BUFFER_SIZE];
    NTSTATUS   status;

    va_start(list, DebugMessage);

    if (DebugMessage) {

        //
        // Using new safe string functions instead of _vsnprintf.
        // This function takes care of NULL terminating if the message
        // is longer than the buffer.
        //
        status = RtlStringCbVPrintfA( debugMessageBuffer,
                                      sizeof(debugMessageBuffer),
                                      DebugMessage,
                                      list );
        if(!NT_SUCCESS(status)) {

            DbgPrint (_DRIVER_NAME_": RtlStringCbVPrintfA failed 0x%x\n", status);
            return;
        }
        if (DebugPrintLevel <= TRACE_LEVEL_ERROR ||
            (DebugPrintLevel <= DebugLevel &&
             ((DebugPrintFlag & DebugFlag) == DebugPrintFlag))) {
            DbgPrint("%s %s", _DRIVER_NAME_, debugMessageBuffer);
        }
    }
    va_end(list);

    return;
#else
    UNREFERENCED_PARAMETER(DebugPrintLevel);
    UNREFERENCED_PARAMETER(DebugPrintFlag);
    UNREFERENCED_PARAMETER(DebugMessage);
#endif
}

#endif




