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

#ifndef ASYNCCOM_PUBLIC_H
#define ASYNCCOM_PUBLIC_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <string.h>

struct asynccom_memory_cap {
	int input;
	int output;
};

#define ASYNCCOM_IOCTL_MAGIC 0x8020
#define ASYNCCOM_IOCTL_INDEX 0x800

#define IOCTL_ASYNCCOM_ENABLE_RS485 CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ASYNCCOM_DISABLE_RS485 CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ASYNCCOM_GET_RS485 CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+2, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ASYNCCOM_ENABLE_ECHO_CANCEL CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+3, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ASYNCCOM_DISABLE_ECHO_CANCEL CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+4, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ASYNCCOM_GET_ECHO_CANCEL CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+5, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ASYNCCOM_SET_SAMPLE_RATE CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+6, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ASYNCCOM_GET_SAMPLE_RATE CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+7, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ASYNCCOM_SET_TX_TRIGGER CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ASYNCCOM_GET_TX_TRIGGER CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+9, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ASYNCCOM_SET_RX_TRIGGER CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+10, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ASYNCCOM_GET_RX_TRIGGER CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+11, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ASYNCCOM_SET_CLOCK_RATE CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+12, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ASYNCCOM_ENABLE_ISOCHRONOUS CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+13, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ASYNCCOM_DISABLE_ISOCHRONOUS CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+14, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ASYNCCOM_GET_ISOCHRONOUS CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+15, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ASYNCCOM_SET_EXTERNAL_TRANSMIT CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+16, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ASYNCCOM_GET_EXTERNAL_TRANSMIT CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+17, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ASYNCCOM_SET_FRAME_LENGTH CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+18, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ASYNCCOM_GET_FRAME_LENGTH CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+19, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ASYNCCOM_ENABLE_9BIT CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+20, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ASYNCCOM_DISABLE_9BIT CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+21, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ASYNCCOM_GET_9BIT CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+22, METHOD_BUFFERED, FILE_ANY_ACCESS)

// fixed baud rate

#define IOCTL_ASYNCCOM_REPROGRAM CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+26, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ASYNCCOM_SET_DIVISOR CTL_CODE(ASYNCCOM_IOCTL_MAGIC, ASYNCCOM_IOCTL_INDEX+27, METHOD_BUFFERED, FILE_ANY_ACCESS)









#ifdef __cplusplus
}
#endif
#endif
