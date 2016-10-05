/*
Copyright (C) 2016  Commtech, Inc.

This file is part of asynccom-windows.

asynccom-windows is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option)
any later version.

asynccom-windows is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
more details.

You should have received a copy of the GNU General Public License along
with asynccom-windows.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ASYNCCOM_FRAME_H
#define ASYNCCOM_FRAME_H

//#include "descriptor.h" /* struct synccom_descriptor */
#include <ntddk.h>
#include <wdf.h>
#include <defines.h>


struct asynccom_frame *asynccom_frame_new(struct asynccom_port *port);
void asynccom_frame_delete(struct asynccom_frame *frame);

int asynccom_frame_update_buffer_size(struct asynccom_frame *frame, unsigned size);
unsigned asynccom_frame_get_length(struct asynccom_frame *frame);
unsigned asynccom_frame_get_buffer_size(struct asynccom_frame *frame);
int asynccom_frame_add_data(struct asynccom_frame *frame, const unsigned char *data, unsigned length);
int asynccom_frame_remove_data(struct asynccom_frame *frame, unsigned char *destination, unsigned length);
unsigned asynccom_frame_is_empty(struct asynccom_frame *frame);
void asynccom_frame_clear(struct asynccom_frame *frame);


#endif