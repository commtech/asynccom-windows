# Connect

###### Code Support
| Code | Version |
| ---- | ------- |
| asynccom-windows | 1.0.0 |


## Connect
The Windows [`CreateFile`](http://msdn.microsoft.com/en-us/library/windows/desktop/aa363858.aspx) is used to connect to the port.

###### Examples
```c
#include <Windows.h>
...

HANDLE h;

h = CreateFile("\\\\.\\ASYNCCOM0", GENERIC_READ | GENERIC_WRITE, 0, NULL,
               OPEN_EXISTING, 0, NULL);
```


### Additional Resources
- Complete example: [`examples/tutorial/tutorial.c`](../examples/tutorial/tutorial.c)
