# Echo Cancel

The echo cancel feature disables the receiver while transmitting. This is mainly used in RS485 mode when the transmit and receive lines are tied together. 

###### Code Support
| Code | Version |
| ---- | ------- |
| asynccom-windows | 1.0.0 |


## Get
```c
IOCTL_ASYNCCOM_GET_ECHO_CANCEL
```

###### Examples
```c
#include <asynccom.h>
...

unsigned status;

DeviceIoControl(h, IOCTL_ASYNCCOM_GET_ECHO_CANCEL,
                NULL, 0,
                &status, sizeof(status),
                &temp, NULL);
```


## Enable
```c
IOCTL_ASYNCCOM_ENABLE_ECHO_CANCEL
```

###### Examples
```c
#include <asynccom.h>
...

DeviceIoControl(h, IOCTL_ASYNCCOM_ENABLE_ECHO_CANCEL,
                NULL, 0,
                NULL, 0,
                &temp, NULL);
```


## Disable
```c
IOCTL_ASYNCCOM_DISABLE_ECHO_CANCEL
```

###### Examples
```c
#include <asynccom.h>
...

DeviceIoControl(h, IOCTL_ASYNCCOM_DISABLE_ECHO_CANCEL,
                NULL, 0,
                NULL, 0,
                &temp, NULL);
```


### Additional Resources
- Complete example: [`examples/echo-cancel/echo-cancel.c`](../examples/echo-cancel/echo-cancel.c)
