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

#ifndef ASYNCCOM_DRIVER_H
#define ASYNCCOM_DRIVER_H
#include <initguid.h>
#include <ntddk.h>
#include "usbdi.h"
#include "usbdlib.h"
#include "driverspecs.h"
#include <wdf.h>
#include <wdfusb.h>
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <defines.h>
#include "trace.h"

//
// Include auto-generated ETW event functions (created by MC.EXE from 
// osrusbfx2.man)
//
#include "fx2Events.h"

#define POOL_TAG (ULONG) 'FNYS'
#define _DRIVER_NAME_ "ASYNCCOM"
extern ULONG DebugLevel;

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_OBJECT_CONTEXT_CLEANUP OsrFxEvtDriverContextCleanup;

#endif