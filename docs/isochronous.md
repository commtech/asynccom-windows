# Isochronous

If you apply an external clock to the card before turning on isochronous mode your system will freeze due to too many serial interrupts. Make sure and apply the clock after you are the  isochronous mode (so the interrupts are disabled).

###### Code Support
| Code | Version |
| ---- | ------- |
| asynccom-windows | 1.0.0 |


| Mode | Description |
| ----:| ----------- |
| 0 | Transmit using external RI# |
| 1 | Transmit using internal BRG |
| 2 | Receive using external DSR# |
| 3 | Transmit using external RI#, receive using external DSR# |
| 4 | Transmit using internal BRG, receive using external DSR# |
| 5 | Receive using internal BRG |
| 6 | Transmit using external RI#, receive using internal BRG |
| 7 | Transmit using internal BRG, receive using internal BRG |
| 8 | Transmit and receive using external RI# |
| 9 | Transmit clock is output on DTR# |
| 10 | Transmit clock is output on DTR#, receive using external DSR# |


## Get
```c
IOCTL_FASTCOM_GET_ISOCHRONOUS
```

| System Error | Value | Cause |
| ------------ | -----:| ----- |
| `ERROR_NOT_SUPPORTED` | 50 (0x32) | Not supported on this family of cards |

###### Examples
```c
#include <asynccom.h>
...

unsigned mode;

DeviceIoControl(h, IOCTL_ASYNCCOM_GET_ISOCHRONOUS,
                NULL, 0,
                &mode, sizeof(mode),
                &temp, NULL);
```


## Enable
```c
IOCTL_ASYNCCOM_ENABLE_ISOCHRONOUS
```

| System Error | Value | Cause |
| ------------ | -----:| ----- |
| `ERROR_NOT_SUPPORTED` | 50 (0x32) | Not supported on this family of cards |
| `ERROR_INVALID_PARAMETER` | 87 (0x57) | Invalid parameter |

###### Examples
```c
#include <asynccom.h>
...

unsigned mode = 7;

DeviceIoControl(h, IOCTL_ASYNCCOM_ENABLE_ISOCHRONOUS,
                &mode, sizeof(mode),
                NULL, 0,
                &temp, NULL);
```


## Disable
```c
IOCTL_ASYNCCOM_DISABLE_ISOCHRONOUS
```

| System Error | Value | Cause |
| ------------ | -----:| ----- |
| `ERROR_NOT_SUPPORTED` | 50 (0x32) | Not supported on this family of cards |

###### Examples
```c
#include <asynccom.h>
...

DeviceIoControl(h, IOCTL_ASYNCCOM_DISABLE_ISOCHRONOUS,
                NULL, 0,
                NULL, 0,
                &temp, NULL);
```


### Additional Resources
- Complete example: [`examples/isochronous/isochronous.c`](../examples/isochronous/isochronous.c)
