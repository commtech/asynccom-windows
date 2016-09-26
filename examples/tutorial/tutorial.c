#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <ntddser.h>
#include "../../inc/asynccom.h"

int main(void)
{
	HANDLE h = 0;
	DWORD tmp;
	SERIAL_BAUD_RATE baud_rate;
	unsigned char odata[] = "Hello world!";
	unsigned char idata[20];

	/* Open port 0 in a blocking IO mode */
	h = CreateFile(L"\\\\.\\ASYNCCOM0", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "CreateFile failed with %d\n", GetLastError());
		return EXIT_FAILURE;
	}
	
	baud_rate.BaudRate = 115200;
	DeviceIoControl(h, IOCTL_SERIAL_SET_BAUD_RATE, &baud_rate, sizeof(baud_rate), NULL, 0, &tmp, NULL);

	WriteFile(h, odata, sizeof(odata), &tmp, NULL);

	/* Read the data back in (with our loopback connector) */
	ReadFile(h, idata, sizeof(idata), &tmp, NULL);

	fprintf(stdout, "%s\n", idata);

	return 0;
}