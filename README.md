# asynccom-windows
This README file is best viewed [online](http://github.com/commtech/asynccom-windows/).

## Installing Driver

##### Downloading Driver Package
You can download a pre-built driver package directly from our [website](https://fastcomproducts.com/software/).


## Quick Start Guide
There is documentation for each specific function listed below, but lets get started with a quick programming example for fun. _This tutorial has already been set up for you at_ [`asynccom-windows/examples/tutorial/tutorial.c`](https://github.com/commtech/asynccom-windows/blob/master/examples/tutorial/tutorial.c).

Create a new C file (named tutorial.c) with the following code.

```c
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <ntddser.h>
#include "asynccom.h"

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
```

For this example I will use the Visual Studio command line compiler, but you can use your compiler of choice.

```
# cl tutorial.c
```

Now attach the included loopback connector.

```
# tutorial.exe
Hello world!
```

You have now transmitted and received data!


## Using The Serial Port
### Setting Baud Rate
##### Max Supported Speeds
The Fastcom: Async Com has a maximum baud rate 40 Mhz, but throughput may be lower than that.


The Fastcom: Async Com cards have their baud rate configured using the standard Windows API, such as IOCTL_SERIAL_SET_BAUD_RATE.

To get a non-standard baud rate there are a couple variables you need to setup before you can achieve them.

First is the variable clock generator frequency and second is the variable sampling rate. The formula for determining a baud rate is as follows.

```
Baud Rate = Clock Rate / Sampling Rate / Integer Divisor.
```

The 'Integer Divisor' value is determined in the driver and as long as the rest of the formula allows for an integer divisor it can be ignored.

Here is an example of some values that will work. We would like a baud rate of 1 MHz so we find a combination of a clock rate of 16 MHz and a sampling rate of 16 that can be divided by an integer to end up with 1 MHz. Now if we configure these two values we will be able to achieve any supported rate we want.

```
1,000,000 = 16,000,000 / 16 / 1
```

Once you understand how to achieve a desired baud rate, you can use our examples to achieve whatever baud rates you need.


## API Reference

There are likely other configuration options you will need to set up for your  own program. All of these options are described on their respective documentation page.

- [Connect](docs/connect.md)
- [Clock Rate](docs/clock-rate.md)
- [Echo Cancel](docs/echo-cancel.md)
- [External Transmit](docs/external-transmit.md)
- [Frame Length](docs/frame-length.md)
- [Isochronous](docs/isochronous.md)
- [9-Bit Protocol](docs/nine-bit.md)
- [Read](docs/read.md)
- [RS485](docs/rs485.md)
- [RX Trigger](docs/rx-trigger.md)
- [Sample Rate](docs/sample-rate.md)
- [TX Trigger](docs/tx-trigger.md)
- [Write](docs/write.md)
- [Disconnect](docs/disconnect.md)



### FAQ

## Build Dependencies
- Windows Driver Kit (7.1.0 used internally to support XP)


## Run-time Dependencies
- OS: Windows 7+


## API Compatibility
We follow [Semantic Versioning](http://semver.org/) when creating releases.


## License

Copyright (C) 2016 [Commtech, Inc.](http://www.fastcomproducts.com)

Licensed under the [GNU General Public License v3](http://www.gnu.org/licenses/gpl.txt).
