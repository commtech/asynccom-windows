# Frame Length

The frame length specifies the number of bytes that get transmitted between the start and stop bits. The standard asynchronous serial communication protocol uses a frame length of one.

###### Code Support
| Code | Version |
| ---- | ------- |
| asynccom-windows | 1.0.0 |

## Get
```c
IOCTL_ASYNCCOM_GET_FRAME_LENGTH
```

| System Error | Value | Cause |
| ------------ | -----:| ----- |
| `ERROR_NOT_SUPPORTED` | 50 (0x32) | Not supported on this family of cards |

###### Examples
```
#include <asynccom.h>
...

unsigned length;

DeviceIoControl(h, IOCTL_ASYNCCOM_GET_FRAME_LENGTH,
				NULL, 0,
				&length, sizeof(length),
				&temp, NULL);
```


## Set
```c
IOCTL_ASYNCCOM_SET_FRAME_LENGTH
```

| System Error | Value | Cause |
| ------------ | -----:| ----- |
| `ERROR_NOT_SUPPORTED` | 50 (0x32) | Not supported on this family of cards |
| `ERROR_INVALID_PARAMETER` | 87 (0x57) | Invalid parameter |

###### Examples
```
#include <asynccom.h>
...

unsigned length = 4;

DeviceIoControl(h, IOCTL_ASYNCCOM_SET_FRAME_LENGTH,
				&length, sizeof(length),
				NULL, 0,
				&temp, NULL);
```


### Additional Resources
- Complete example: [`examples/frame-length/frame-length.c`](../examples/frame-length/frame-length.c)
