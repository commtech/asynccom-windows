#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <ntddser.h>
#include "../../inc/asynccom.h"

int main(void)
{
	HANDLE h = 0;
	DWORD tmp;
	unsigned char odata[] = "Hello world!";
	unsigned char idata[20];
    DCB mdcb;
    BOOL success;

	/* Open port 0 in a blocking IO mode */
	h = CreateFile(L"\\\\.\\COM8", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "CreateFile failed with %d\n", GetLastError());
		return EXIT_FAILURE;
	}
	
    memset(&mdcb, 0, sizeof(mdcb));
    mdcb.DCBlength = sizeof(mdcb);
    success = GetCommState(h, &mdcb);
    if (!success)
    {
        printf("GetCommState failed! %d\n", GetLastError());
        return EXIT_FAILURE;
    }

    mdcb.BaudRate = 115200;
    mdcb.ByteSize = 7;
    mdcb.Parity = NOPARITY;
    mdcb.StopBits = ONESTOPBIT;
    if (SetCommState(h, &mdcb) == FALSE) {
        fprintf(stderr, "SetCommState failed with %d\n", GetLastError());
        return EXIT_FAILURE;
    }
    PurgeComm(h, PURGE_TXCLEAR | PURGE_RXCLEAR);


	WriteFile(h, odata, sizeof(odata), &tmp, NULL);

	/* Read the data back in (with our loopback connector) */
	ReadFile(h, idata, sizeof(idata), &tmp, NULL);

	fprintf(stdout, "%s\n", idata);

	return 0;
}