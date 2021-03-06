#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include "asynccom.h"

int main(void)
{
	HANDLE h = 0;
	DWORD tmp = 0;
	unsigned level = 0;

	/* Open port 0 in a blocking IO mode */
	h = CreateFile(L"\\\\.\\COM3", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "CreateFile failed with %d\n", GetLastError());
		return EXIT_FAILURE;
	}

	DeviceIoControl(h, IOCTL_ASYNCCOM_GET_TX_TRIGGER, NULL, 0, &level, sizeof(level), &tmp, NULL);
	level = 32;
	DeviceIoControl(h, IOCTL_ASYNCCOM_SET_TX_TRIGGER, &level, sizeof(level), NULL, 0, &tmp, NULL);

	CloseHandle(h);
	return 0;
}