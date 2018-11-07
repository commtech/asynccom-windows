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

#ifndef ASYNCCOM_UTILS_H
#define ASYNCCOM_UTILS_H
#include <ntddk.h>
#include <wdf.h>
#include <defines.h>
#include "trace.h"

struct ResultStruct {
	double target;
	double freq;
	double errorPPM;
	int VCO_Div;
	int refDiv;
	int outDiv;
	int failed;
};

struct IcpRsStruct {
	double pdf;
	double nbw;
	double ratio;
	double df;
	ULONG Rs;
	double icp;
	ULONG icpnum;   //I have to use this in the switch statement because 8.75e-6 becomes 874
};


UINT32 chars_to_u32(const unsigned char *data);
NTSTATUS calculate_divisor(unsigned long clock_rate, unsigned long sample_rate, long desired_baud, short *divisor);
int GetICS30703Data(unsigned long desired, unsigned long ppm, struct ResultStruct *theOne, struct IcpRsStruct *theOther, unsigned char *progdata);
#endif
