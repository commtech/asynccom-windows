#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <ntddser.h>
#include "../../inc/asynccom.h"

int main(void)
{
	HANDLE h = 0;
	unsigned desired_clock = 1843200;
	DWORD tmp = 0;

	/* Open port 0 in a blocking IO mode */
	h = CreateFile(L"\\\\.\\ASYNCCOM0", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "CreateFile failed with %d\n", GetLastError());
		return EXIT_FAILURE;
	}

	DeviceIoControl(h, IOCTL_ASYNCCOM_SET_CLOCK_RATE, &desired_clock, sizeof(desired_clock), NULL, 0, &tmp, NULL);
	CloseHandle(h);
}