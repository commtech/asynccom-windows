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

PCHAR get_ioctl_name(ULONG ioctl_code);
UINT32 chars_to_u32(const unsigned char *data);
NTSTATUS calculate_divisor(unsigned long clock_rate, unsigned long sample_rate, long desired_baud, short *divisor);
int GetICS30703Data(unsigned long desired, unsigned long ppm, struct ResultStruct *theOne, struct IcpRsStruct *theOther, unsigned char *progdata);
#endif
