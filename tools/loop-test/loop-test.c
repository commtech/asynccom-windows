#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <ntddser.h>
#include "../../inc/asynccom.h"

int loop_test(HANDLE h, int bytes_to_read);

int main(void)
{
	HANDLE h = 0;
	DWORD tmp;
	SERIAL_BAUD_RATE baud_rate;
	int failures = 0;
	ULONG mask = 0;

	/* Open port 0 in a blocking IO mode */
	h = CreateFile(L"\\\\.\\ASYNCCOM0", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "CreateFile failed with %d\n", GetLastError());
		return EXIT_FAILURE;
	}
	// Here is an example of setup.
	// Basically the drivers calculate an 'additional divisor' to use to get a baud rate. The formula for baud rate is
	// baud_rate = clock_rate / (sample_rate * additional_divisor)
	// This divisor must be an integer, but it can be 1. In this case, it is 1.
	//
	// You do not need to know the additional divisor, but it is useful to understand how the drivers
	// achieve your desired baud rate.

	// The default clock rate is 1843200.
	// The default sample rate is 16.
	// In this case, the formula is:
	// 115200 = 1843200 / 16 / 1
	// This means the 'additional divisor' is 1.
	// You could also set the additional divisor directly by using ASYNCCOM_SET_DIVISOR.

	baud_rate.BaudRate = 115200;
	DeviceIoControl(h, IOCTL_SERIAL_SET_BAUD_RATE, &baud_rate, sizeof(baud_rate), NULL, 0, &tmp, NULL);
	failures = loop_test(h, 10000);
	mask = SERIAL_PURGE_TXABORT | SERIAL_PURGE_RXABORT | SERIAL_PURGE_RXCLEAR;
	DeviceIoControl(h, IOCTL_SERIAL_PURGE, &mask, sizeof(mask), NULL, 0, &tmp, NULL);
	printf("115.2k test: %d failures\n", failures);
	Sleep(1000);

	// In this case, the formula is:
	// 9600 = 1843200 / 16 / 12
	// This means the 'additional divisor' is 12.

	baud_rate.BaudRate = 9600;
	DeviceIoControl(h, IOCTL_SERIAL_SET_BAUD_RATE, &baud_rate, sizeof(baud_rate), NULL, 0, &tmp, NULL);
	failures = loop_test(h, 10000);
	mask = SERIAL_PURGE_TXABORT | SERIAL_PURGE_RXABORT | SERIAL_PURGE_RXCLEAR;
	DeviceIoControl(h, IOCTL_SERIAL_PURGE, &mask, sizeof(mask), NULL, 0, &tmp, NULL);
	printf("9.6k test:   %d failures\n", failures);

	CloseHandle(h);

	return EXIT_SUCCESS;
}

#define MAX_BUFFER 4096
int loop_test(HANDLE h, int bytes_to_read)	
{
	int total_errors = 0, data_size = 0;
	unsigned char odata[MAX_BUFFER], idata[MAX_BUFFER+2];
	DWORD data_written, data_read;
	int return_value = 0, total_bytes_read = 0, i = 0;

	while (total_bytes_read < bytes_to_read)
	{
		// You could do a frame larger than this - but this is the biggest FIFO size.
		data_size = rand() % 128;
		if (data_size > MAX_BUFFER) data_size = MAX_BUFFER;
		for (i = 0; i < data_size; i++) odata[i] = rand() % 255;

		return_value = WriteFile(h, odata, data_size, &data_written, NULL);
		if (return_value != 1) { printf("write_status: %d\n", return_value); total_errors++; }
		return_value = ReadFile(h, idata, data_size, &data_read, NULL);
		if (return_value != 1) { printf("read_status: %d\n", return_value); total_errors++; }
		
		if (data_written != data_read) { printf("data_read: %d   data_written: %d\n", data_read, data_written); total_errors += abs(data_written - data_read); }
		for (i = 0; i < (int)data_read; i++) if (odata[i] != idata[i]) { printf("data_mismatch: %d: 0x%2.2x != 0x%2.2x\n", i, odata[i], idata[i]); total_errors++; }
		total_bytes_read += data_read;
		if (total_errors > 5) break;
	}

	return total_errors;
}