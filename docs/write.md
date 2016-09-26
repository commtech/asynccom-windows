# Write

###### Support
| Code | Version |
| ---- | ------- |
| asynccom-windows | 1.0.0 |


## Write
The Windows [`WriteFile`](http://msdn.microsoft.com/en-us/library/windows/desktop/aa365747.aspx) is used to write data to the port.

###### Examples
```c
char odata[] = "Hello world!";
unsigned bytes_written;

WriteFile(h, odata, sizeof(odata), (DWORD*)bytes_written, NULL);
```


### Additional Resources
- Complete example: [`examples/tutorial/tutorial.c`](../examples/tutorial/tutorial.c)
