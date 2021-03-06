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
