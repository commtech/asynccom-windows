/*
Copyright 2019 Commtech, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
copies of the Software, and to permit persons to whom the Software is 
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in 
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
THE SOFTWARE.
*/

#ifndef ASYNCCOM_FRAME_H
#define ASYNCCOM_FRAME_H

//#include "descriptor.h" /* struct synccom_descriptor */
#include <ntddk.h>
#include <wdf.h>
#include <defines.h>


struct asynccom_frame	*asynccom_frame_new(struct asynccom_port *port);
void					asynccom_frame_delete(struct asynccom_frame *frame);
int						asynccom_frame_update_buffer_size(struct asynccom_frame *frame, unsigned size);
unsigned				asynccom_frame_get_length(struct asynccom_frame *frame);
unsigned				asynccom_frame_get_buffer_size(struct asynccom_frame *frame);
int						asynccom_frame_add_data(struct asynccom_frame *frame, const unsigned char *data, unsigned length);
int						asynccom_frame_remove_data(struct asynccom_frame *frame, unsigned char *destination, unsigned length);
unsigned				asynccom_frame_is_empty(struct asynccom_frame *frame);
void					asynccom_frame_clear(struct asynccom_frame *frame);


#endif