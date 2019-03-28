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

#ifndef ASYNCCOM_DEFINES_H
#define ASYNCCOM_DEFINES_H

#include <ntddk.h>
#include <wdf.h>
#include <asynccom.h>
#include <wmidata.h>
#include <ntddser.h>

#define DEBUG

// {74A3CD54-4696-4B6D-9863-5A67936ED5FA}
DEFINE_GUID(GUID_DEV_ASYNCCOM, 0x74a3cd54, 0x4696, 0x4b6d, 0x98, 0x63, 0x5a, 0x67, 0x93, 0x6e, 0xd5, 0xfa);
static const GUID ASYNCCOM_DEV_GUID = { 0x74a3cd54, 0x4696, 0x4b6d,{ 0x98, 0x63, 0x5a, 0x67, 0x93, 0x6e, 0xd5, 0xfa } };

// {E85C65CC-B33B-40D4-815E-3657C2D05017}
DEFINE_GUID(GUID_DEVCLASS_COMMTECH, 0xe85c65cc, 0xb33b, 0x40d4, 0x81, 0x5e, 0x36, 0x57, 0xc2, 0xd0, 0x50, 0x17);
static const GUID COMMTECH_CLASS_GUID = { 0xe85c65cc, 0xb33b, 0x40d4,{ 0x81, 0x5e, 0x36, 0x57, 0xc2, 0xd0, 0x50, 0x17 } };


#define DEVICE_NAME "asynccom"

#define REGISTER_WRITE_ENDPOINT 0x01
#define REGISTER_READ_ENDPOINT	0x81
#define DATA_WRITE_ENDPOINT		0x06
#define DATA_READ_ENDPOINT		0x82

// These are defined in the firmware, so shouldn't be changed unless you are 100% sure
// you know what you are doing.
#define ASYNCCOM_WRITE_REGISTER				0x6A // write: 0x6A (address - 2B) (value - 4B)
#define ASYNCCOM_READ_REGISTER				0x6B // write: 0x6B (address - 2B)
                                                 // read : (value - 4B)
#define ASYNCCOM_READ_WITH_ADDRESS			0x6C // write: 0x6C (address - 2B)
                                                 // read : (address - 2B) (value - 4B)
#define ASYNCCOM_READ_WAIT_HIGH_VAL  		0x6D // write: 0x6D (address - 2B) (timeout - 1B) (mask - 4B)
                                                 // read : (address - 2B) (value - 4B)

#define SYNCCOM_GET_STATUS					0xFE
#define DEVICE_OBJECT_NAME_LENGTH       128

#define TX_FIFO_SIZE 128

#define SERIAL_DEVICE_MAP					L"SERIALCOMM"

#define UNUSED(x) (void)(x)

#define warn_if_untrue(expr) if (expr) {} else  {  KdPrint((DEVICE_NAME " %s %s\n", #expr, "is untrue."));  }
#define return_if_untrue(expr)  if (expr) {} else {  KdPrint((DEVICE_NAME " %s %s\n", #expr, "is untrue."));  return;  }
#define return_val_if_untrue(expr, val)  if (expr) {} else  {  KdPrint((DEVICE_NAME " %s %s\n", #expr, "is untrue."));  return val;  }
#define _BYTESWAP_UINT32(value) ((value << 24) | ((value << 8) & 0x00ff0000) | ((value >> 8) & 0x0000ff00) | (value >> 24))

#define SYNCCOM_UPPER_OFFSET	0x80
#define ASYNCCOM_UPPER_OFFSET	0x40
#define FPGA_UPPER_ADDRESS		0x00

#define IER_OFFSET 0x1
#define FCR_OFFSET 0x2 //w
#define ISR_OFFSET 0x2 //r
#define LCR_OFFSET 0x3
#define MCR_OFFSET 0x4
#define LSR_OFFSET 0x5 //r
#define MSR_OFFSET 0x6 //r
#define SPR_OFFSET 0x7

// These require the LCR[7] bit to be set to 1.
#define DLL_OFFSET 0x0
#define DLM_OFFSET 0x1

// These require the LCR register set to 0xBF
#define EFR_OFFSET 0x2
#define XON1_OFFSET 0x4
#define XON2_OFFSET 0x5
#define XOFF1_OFFSET 0x6
#define XOFF2_OFFSET 0x7

#define ICR_OFFSET 0x5
// To use these, ensure that 0xBF was not the last value written to LCR.
// Then write the offset to SPR (0x7)
// Then write the value to ICR (0x5)

// To read, ensure that 0xBF was not the last value written to LCR.
// Write 0x0 to SPR (0x7)
// Write x1xxxxxx to ACR (keep the other bits the same)
// Write the desired offset to SPR (0x7)
// Read the desired value from ICR (0x5)

#define ACR_OFFSET  0x00
#define CPR_OFFSET  0x01
#define TCR_OFFSET  0x02
#define CKS_OFFSET  0x03
#define TTL_OFFSET  0x04
#define RTL_OFFSET  0x05
#define FCL_OFFSET  0x06
#define FCH_OFFSET  0x07
#define ID1_OFFSET  0x08 //r
#define ID2_OFFSET  0x09 //r
#define ID3_OFFSET  0x0A //r
#define REV_OFFSET  0x0B //r
#define CSR_OFFSET  0x0C //w
#define NMR_OFFSET  0x0D
#define MDM_OFFSET  0x0E
#define RFC_OFFSET  0x0F //r
#define GDS_OFFSET  0x10 //r
#define DMS_OFFSET  0x11
#define PIDX_OFFSET 0x12 //w
#define CKA_OFFSET  0x13
#define EXT_OFFSET  0x16
#define EXTH_OFFSET 0x17
#define FLR_OFFSET	0x20

// Should only use these with the upper address of 0x00
#define FPGA_FCR_OFFSET 0x20 
#define CONTROL_OFFSET	0x02

#define FLUSH_COMMAND 0x00000010


typedef struct _SERIAL_FIRMWARE_DATA {
    PDRIVER_OBJECT  DriverObject;
    ULONG           ControllersFound;
    ULONG           ForceFifoEnableDefault;
    ULONG           DebugLevel;
    ULONG           ShouldBreakOnEntry;
    ULONG           RS485Default;
    ULONG           SampleRateDefault;
    ULONG           RxTriggerDefault;
    ULONG           TxTriggerDefault;
    ULONG           TerminationDefault;
    ULONG           EchoCancelDefault;
    ULONG           IsochronousDefault;
    ULONG           FrameLengthDefault;
    ULONG           NineBitDefault;
    ULONG           FixedBaudRateDefault;
    ULONG           PermitShareDefault;
    ULONG           PermitSystemWideShare;
    ULONG           LogFifoDefault;
    ULONG           UartRemovalDetect;
    UNICODE_STRING  Directory;
    UNICODE_STRING  NtNameSuffix;
    UNICODE_STRING  DirectorySymbolicName;
    LIST_ENTRY      ConfigList;
} SERIAL_FIRMWARE_DATA, *PSERIAL_FIRMWARE_DATA;


//
// These defines are used to set the line control register.
//
#define SERIAL_5_DATA       ((UCHAR)0x00)
#define SERIAL_6_DATA       ((UCHAR)0x01)
#define SERIAL_7_DATA       ((UCHAR)0x02)
#define SERIAL_8_DATA       ((UCHAR)0x03)
#define SERIAL_DATA_MASK    ((UCHAR)0x03)

#define SERIAL_1_STOP       ((UCHAR)0x00)
#define SERIAL_1_5_STOP     ((UCHAR)0x04) // Only valid for 5 data bits
#define SERIAL_2_STOP       ((UCHAR)0x04) // Not valid for 5 data bits
#define SERIAL_STOP_MASK    ((UCHAR)0x04)

#define SERIAL_NONE_PARITY  ((UCHAR)0x00)
#define SERIAL_ODD_PARITY   ((UCHAR)0x08)
#define SERIAL_EVEN_PARITY  ((UCHAR)0x18)
#define SERIAL_MARK_PARITY  ((UCHAR)0x28)
#define SERIAL_SPACE_PARITY ((UCHAR)0x38)
#define SERIAL_PARITY_MASK  ((UCHAR)0x38)

//
// Reasons that recption may be held up.
//
#define SERIAL_RX_DTR       ((ULONG)0x01)
#define SERIAL_RX_XOFF      ((ULONG)0x02)
#define SERIAL_RX_RTS       ((ULONG)0x04)
#define SERIAL_RX_DSR       ((ULONG)0x08)

//
// Reasons that transmission may be held up.
//
#define SERIAL_TX_CTS       ((ULONG)0x01)
#define SERIAL_TX_DSR       ((ULONG)0x02)
#define SERIAL_TX_DCD       ((ULONG)0x04)
#define SERIAL_TX_XOFF      ((ULONG)0x08)
#define SERIAL_TX_BREAK     ((ULONG)0x10)

// This defines the bit used to control whether the device is sending
// a break.  When this bit is set the device is sending a space (logic 0).
//
// Most protocols will assume that this is a hangup.
//
#define SERIAL_LCR_BREAK    0x40


// These masks are used to access the modem status register.
// Whenever one of the first four bits in the modem status
// register changes state a modem status interrupt is generated.
//
#define SERIAL_MSR_DCTS     0x01
#define SERIAL_MSR_DDSR     0x02
#define SERIAL_MSR_TERI     0x04
#define SERIAL_MSR_DDCD     0x08
#define SERIAL_MSR_CTS      0x10
#define SERIAL_MSR_DSR      0x20
#define SERIAL_MSR_RI       0x40
#define SERIAL_MSR_DCD      0x80

#define SERIAL_UNINITIALIZED_DEFAULT    1234567
#define SERIAL_FORCE_FIFO_DEFAULT       1
#define SERIAL_RS485_DEFAULT            0
#define SERIAL_SAMPLE_RATE_DEFAULT      16
#define SERIAL_RX_TRIGGER_DEFAULT       32
#define SERIAL_TX_TRIGGER_DEFAULT       32
#define SERIAL_TERMINATION_DEFAULT      1
#define SERIAL_ECHO_CANCEL_DEFAULT      0
#define SERIAL_ISOCHRONOUS_DEFAULT      -1
#define SERIAL_FRAME_LENGTH_DEFAULT     1
#define SERIAL_9BIT_DEFAULT             0
#define SERIAL_FIXED_BAUD_RATE_DEFAULT -1
#define SERIAL_PERMIT_SHARE_DEFAULT     0
#define SERIAL_LOG_FIFO_DEFAULT         0

#define RFE		0x00000004
#define RFT		0x00000002
#define RFS		0x00000001
#define RFO		0x00000008
#define RDO		0x00000010
#define RFL		0x00000020
#define TIN		0x00000100
#define TDU		0x00040000
#define TFT		0x00010000
#define ALLS	0x00020000
#define CTSS	0x01000000
#define DSRC	0x02000000
#define CDC		0x04000000
#define CTSA	0x08000000
#define DR_STOP 0x00004000
#define DT_STOP 0x00008000
#define DT_FE	0x00002000
#define DR_FE	0x00001000
#define DT_HI	0x00000800
#define DR_HI	0x00000400

#define CE_BIT	0x00040000

typedef struct asynccom_frame {
	unsigned char *buffer;
	unsigned buffer_size;
	unsigned data_length;
	unsigned lost_bytes;
	struct asynccom_port *port;
} ASYNCCOM_FRAME;

typedef struct asynccom_port {
	WDFUSBDEVICE                    usb_device;
	ULONG							usb_traits;
	PDRIVER_OBJECT					driver_object;
	PDEVICE_OBJECT					device_object;
	WDFDEVICE						device;
	WDFUSBINTERFACE                 usb_interface;
	UNICODE_STRING					device_name;
	ULONG							skip_naming;
	BOOLEAN							created_symbolic_link;
	BOOLEAN							created_serial_comm_entry;
	WDFUSBPIPE                      data_read_pipe;
	WDFUSBPIPE                      data_write_pipe;
	WDFUSBPIPE						register_write_pipe;
	WDFUSBPIPE						register_read_pipe;
	WDFTIMER						read_request_total_timer;
	WDFTIMER						read_request_interval_timer;
	WDFREQUEST						current_read_request;

	unsigned port_number;

	WDFQUEUE write_queue;
	WDFQUEUE read_queue;
	WDFQUEUE read_queue2; // TODO: Change name to be more descriptive. 
	WDFQUEUE ioctl_queue;

	WDFSPINLOCK istream_spinlock;
	struct asynccom_frame *istream; // Transparent stream 
	struct asynccom_frame *pending_oframe; // Frame being put in the FIFO 

	WDFDPC oframe_dpc;
	WDFDPC istream_dpc;
	WDFDPC process_read_dpc;

	struct asynccom_memory_cap memory_cap;

	//
	// New async stuff added to port structure
	//
	UINT32							current_acr;
	UINT32							current_mcr;
	UINT32							current_clock_frequency;
	unsigned short					current_divisor;
	ULONG							current_baud;
	UINT32							current_sample_rate;
	unsigned char					line_control;
	unsigned char					valid_data_mask;
	unsigned char					escape_char;
	SERIAL_CHARS					special_chars;
	SERIAL_TIMEOUTS					timeouts;

	ULONG							TXHolding;
	ULONG							RXHolding;
	ULONG							buffer_size; //Is this necessary?

	SERIAL_HANDFLOW HandFlow;

} ASYNCCOM_PORT, *PASYNCCOM_PORT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(ASYNCCOM_PORT, GetPortContext)

typedef struct _REQUEST_CONTEXT {
	ULONG_PTR information;
	NTSTATUS status;
	ULONG length;
	unsigned char *data_buffer;
} REQUEST_CONTEXT, *PREQUEST_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(REQUEST_CONTEXT, GetRequestContext)

#define DEFAULT_INPUT_MEMORY_CAP_VALUE 1000000
#define DEFAULT_OUTPUT_MEMORY_CAP_VALUE 1000000

#define DEFAULT_HOT_PLUG_VALUE 0
#define DEFAULT_FORCE_FIFO_VALUE 1
#define DEFAULT_APPEND_STATUS_VALUE 0
#define DEFAULT_APPEND_TIMESTAMP_VALUE 0
#define DEFAULT_IGNORE_TIMEOUT_VALUE 0
#define DEFAULT_TX_MODIFIERS_VALUE XF
#define DEFAULT_RX_MULTIPLE_VALUE 0
#define DEFAULT_WAIT_ON_WRITE_VALUE 0

// These defines are taken from the microsoft serialp.h sample file.
//
// The following three macros are used to initialize, set
// and clear references in IRPs that are used by
// this driver.  The reference is stored in the fourth
// argument of the request, which is never used by any operation
// accepted by this driver.
//

#define SERIAL_REF_ISR         (0x00000001)
#define SERIAL_REF_CANCEL      (0x00000002)
#define SERIAL_REF_TOTAL_TIMER (0x00000004)
#define SERIAL_REF_INT_TIMER   (0x00000008)
#define SERIAL_REF_XOFF_REF    (0x00000010)


//
// Prototypes and defines to handle processor groups.
//
typedef
USHORT
(*PFN_KE_GET_ACTIVE_GROUP_COUNT)(
	VOID
	);

typedef
KAFFINITY
(*PFN_KE_QUERY_GROUP_AFFINITY) (
	_In_ USHORT GroupNumber
	);

#endif