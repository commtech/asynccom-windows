# Sample Rate

###### Code Support
| Code | Version |
| ---- | ------- |
| asynccom-windows | 1.0.0 |

###### Operating Range
| Card Family | Range |
| ----------- | ----- |
| Fastcom: Async Com (16C950) | 4 - 16 |


## Get
```c
IOCTL_ASYNCCOM_GET_SAMPLE_RATE
```

###### Examples
```
#include <asynccom.h>
...

unsigned rate;

DeviceIoControl(h, IOCTL_ASYNCCOM_GET_SAMPLE_RATE,
				NULL, 0,
				&rate, sizeof(rate),
				&temp, NULL);
```


## Set
```c
IOCTL_ASYNCCOM_SET_SAMPLE_RATE
```

| System Error | Value | Cause |
| ------------ | -----:| ----- |
| `ERROR_INVALID_PARAMETER` | 87 (0x57) | Invalid parameter |

###### Examples
```
#include <asynccom.h>
...

unsigned rate = 16;

DeviceIoControl(h, IOCTL_ASYNCCOM_SET_SAMPLE_RATE,
				&rate, sizeof(rate),
				NULL, 0,
				&temp, NULL);
```


### Additional Resources
- Complete example: [`examples/sample-rate/sample-rate.c`](../examples/sample-rate/sample-rate.c)
