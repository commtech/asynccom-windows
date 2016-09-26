# External Transmit

###### Code Support
| Code | Version |
| ---- | ------- |
| asynccom-windows | 1.0.0 |


###### Operating Range
| Card Family | Range |
| ----------- | ----- |
| Fastcom: Async Com (16C950) | 0 - 8191 |


## Get
```c
IOCTL_ASYNCCOM_GET_EXTERNAL_TRANSMIT
```

| System Error | Value | Cause |
| ------------ | -----:| ----- |
| `ERROR_NOT_SUPPORTED` | 50 (0x32) | Not supported on this family of cards |

###### Examples
```c
#include <asynccom.h>
...

unsigned num_chars;

DeviceIoControl(h, IOCTL_ASYNCCOM_GET_EXTERNAL_TRANSMIT,
                NULL, 0,
                &num_chars, sizeof(num_chars),
                &temp, NULL);
```


## Enable
```c
IOCTL_ASYNCCOM_ENABLE_EXTERNAL_TRANSMIT
```

| System Error | Value | Cause |
| ------------ | -----:| ----- |
| `ERROR_NOT_SUPPORTED` | 50 (0x32) | Not supported on this family of cards |
| `ERROR_INVALID_PARAMETER` | 87 (0x57) | Invalid parameter |

###### Examples
```c
#include <asynccom.h>
...

unsigned num_chars = 4;

DeviceIoControl(h, IOCTL_ASYNCCOM_ENABLE_EXTERNAL_TRANSMIT,
                &num_chars, sizeof(num_chars),
                NULL, 0,
                &temp, NULL);
```


## Disable
```c
IOCTL_ASYNCCOM_DISABLE_EXTERNAL_TRANSMIT
```

| System Error | Value | Cause |
| ------------ | -----:| ----- |
| `ERROR_NOT_SUPPORTED` | 50 (0x32) | Not supported on this family of cards |

###### Examples
```c
#include <asynccom.h>
...

DeviceIoControl(h, IOCTL_ASYNCCOM_DISABLE_EXTERNAL_TRANSMIT,
                NULL, 0,
                NULL, 0,
                &temp, NULL);
```


### Additional Resources
- Complete example: [`examples/external-transmit/external-transmit.c`](../examples/external-transmit/external-transmit.c)
