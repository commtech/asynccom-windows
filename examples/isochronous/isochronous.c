#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include "../../inc/asynccom.h"

int main(void)
{
	HANDLE h = 0;
	DWORD tmp;
	unsigned mode;

	h = CreateFile(L"\\\\.\\COM7", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "CreateFile failed with %d\n", GetLastError());
		return EXIT_FAILURE;
	}

	DeviceIoControl(h, IOCTL_ASYNCCOM_GET_ISOCHRONOUS, NULL, 0, &mode, sizeof(mode), &tmp, (LPOVERLAPPED)NULL);
	printf("Mode: %d\n", mode);
	mode = 9;
	DeviceIoControl(h, IOCTL_ASYNCCOM_ENABLE_ISOCHRONOUS, &mode, sizeof(mode), NULL, 0, &tmp, (LPOVERLAPPED)NULL);
	printf("Mode: %d\n", mode);

	DeviceIoControl(h, IOCTL_ASYNCCOM_GET_ISOCHRONOUS, NULL, 0, &mode, sizeof(mode), &tmp, (LPOVERLAPPED)NULL);
	printf("Mode: %d\n", mode);

	DeviceIoControl(h, IOCTL_ASYNCCOM_DISABLE_ISOCHRONOUS, NULL, 0, NULL, 0, &tmp, (LPOVERLAPPED)NULL);

	CloseHandle(h);

	return 0;
}