# Clock Rate

###### Code Support
| Code | Version |
| ---- | ------- |
| asynccom-windows | 1.0.0 |



###### Operating Range
| Card Family | Range |
| ----------- | ----- |
| Fastcom: Async Com (16C950) | 200 Hz - 270 MHz |


## Set
```c
IOCTL_ASYNCCOM_SET_CLOCK_RATE
```

| System Error | Value | Cause |
| ------------ | -----:| ----- |
| `ERROR_INVALID_PARAMETER` | 87 (0x57) | Invalid parameter |

###### Examples
```
#include <asynccom.h>
...

unsigned rate = 18432000; /* 18.432 MHz */

DeviceIoControl(h, IOCTL_ASYNCCOM_SET_CLOCK_RATE,
				&rate, sizeof(rate),
				NULL, 0,
				&temp, NULL);
```


### Additional Resources
- Complete example: [`examples/clock-rate/clock-rate.c`](../examples/clock-rate/clock-rate.c)
