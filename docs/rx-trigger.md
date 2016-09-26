# RX Trigger Level

The RX FIFO trigger level generates an interrupt whenever the receive FIFO level rises to this preset trigger level.

###### Code Support
| Code | Version |
| ---- | ------- |
| serialfc-windows | 1.0.0 |

###### Operating Range
| Card Family | Range |
| ----------- | ----- |
| Fastcom: Async Com (16C950) | 0 - 127 |

## Get
```c
IOCTL_ASYNCCOM_GET_RX_TRIGGER
```

###### Examples
```
#include <asynccom.h>
...

unsigned level;

DeviceIoControl(h, IOCTL_ASYNCCOM_GET_RX_TRIGGER,
				NULL, 0,
				&level, sizeof(level),
				&temp, NULL);
```


## Set
```c
IOCTL_ASYNCCOM_SET_RX_TRIGGER
```

| System Error | Value | Cause |
| ------------ | -----:| ----- |
| `ERROR_INVALID_PARAMETER` | 87 (0x57) | Invalid parameter |

###### Examples
```
#include <asynccom.h>
...

unsigned level = 32;

DeviceIoControl(h, IOCTL_ASYNCCOM_SET_RX_TRIGGER,
				&level, sizeof(level),
				NULL, 0,
				&temp, NULL);
```


### Additional Resources
- Complete example: [`examples/rx-trigger/rx-trigger.c`](../examples/rx-trigger/rx-trigger.c)
