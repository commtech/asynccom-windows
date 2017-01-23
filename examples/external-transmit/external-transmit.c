#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <ntddser.h>
#include "../../inc/asynccom.h"

int main(void)
{
	HANDLE h = 0;
	DWORD tmp = 0;
	unsigned num_chars = 0;

	/* Open port 0 in a blocking IO mode */
	h = CreateFile(L"\\\\.\\COM8", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "CreateFile failed with %d\n", GetLastError());
		return EXIT_FAILURE;
	}

	DeviceIoControl(h, IOCTL_ASYNCCOM_GET_EXTERNAL_TRANSMIT, NULL, 0, &num_chars, sizeof(num_chars), &tmp, NULL);
	num_chars = 4;
	DeviceIoControl(h, IOCTL_ASYNCCOM_SET_EXTERNAL_TRANSMIT, &num_chars, sizeof(num_chars), NULL, 0, &tmp, NULL);
	num_chars = 0;
	DeviceIoControl(h, IOCTL_ASYNCCOM_SET_EXTERNAL_TRANSMIT, &num_chars, sizeof(num_chars), NULL, 0, &tmp, NULL);

	CloseHandle(h);
	return 0;
}