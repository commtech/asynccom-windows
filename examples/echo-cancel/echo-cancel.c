#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <ntddser.h>
#include "../../inc/asynccom.h"

int main(void)
{
	HANDLE h = 0;
	DWORD tmp = 0;
	unsigned echo_cancel = 0;

	/* Open port 0 in a blocking IO mode */
	h = CreateFile(L"\\\\.\\COM8", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "CreateFile failed with %d\n", GetLastError());
		return EXIT_FAILURE;
	}

	DeviceIoControl(h, IOCTL_ASYNCCOM_GET_ECHO_CANCEL, NULL, 0, &echo_cancel, sizeof(echo_cancel), &tmp, NULL);
	DeviceIoControl(h, IOCTL_ASYNCCOM_ENABLE_ECHO_CANCEL, NULL, 0 , NULL, 0, &tmp, NULL);
	DeviceIoControl(h, IOCTL_ASYNCCOM_DISABLE_ECHO_CANCEL, NULL, 0, NULL, 0, &tmp, NULL);

	CloseHandle(h);
	return 0;
}