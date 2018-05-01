#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include "asynccom.h"

int main(void)
{
	HANDLE h = 0;
	DWORD tmp = 0;
	unsigned rs485 = 0;

	/* Open port 0 in a blocking IO mode */
	h = CreateFile(L"\\\\.\\COM3", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "CreateFile failed with %d\n", GetLastError());
		return EXIT_FAILURE;
	}

	DeviceIoControl(h, IOCTL_ASYNCCOM_GET_RS485, NULL, 0, &rs485, sizeof(rs485), &tmp, NULL);
	DeviceIoControl(h, IOCTL_ASYNCCOM_ENABLE_RS485, NULL, 0, NULL, 0, &tmp, NULL);
	DeviceIoControl(h, IOCTL_ASYNCCOM_DISABLE_RS485, NULL, 0, NULL, 0, &tmp, NULL);

	CloseHandle(h);
	return 0;
}