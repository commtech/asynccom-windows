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

#include "frame.h"
#include "utils.h" /* return_{val_}if_true */
#include <driver.h>


#include "trace.h"
//#if defined(EVENT_TRACING)
//#include "frame.tmh"
//#endif

#pragma warning(disable:4267)

struct asynccom_frame *asynccom_frame_new(struct asynccom_port *port)
{
	struct asynccom_frame *frame = 0;

	frame = (struct asynccom_frame *)ExAllocatePoolWithTag(NonPagedPool, sizeof(*frame), 'marF');

	if (frame == NULL)
		return 0;

	frame->data_length = 0;
	frame->buffer_size = 0;
	frame->buffer = 0;
	frame->port = port;
	frame->lost_bytes = 0;

	return frame;
}

void asynccom_frame_delete(struct asynccom_frame *frame)
{
	return_if_untrue(frame);

	asynccom_frame_update_buffer_size(frame, 0);

	ExFreePoolWithTag(frame, 'marF');
}

unsigned asynccom_frame_get_length(struct asynccom_frame *frame)
{
	return_val_if_untrue(frame, 0);

	return frame->data_length;
}

unsigned asynccom_frame_get_buffer_size(struct asynccom_frame *frame)
{
	return_val_if_untrue(frame, 0);

	return frame->buffer_size;
}

unsigned asynccom_frame_is_empty(struct asynccom_frame *frame)
{
	return_val_if_untrue(frame, 0);

	return frame->data_length == 0;
}

void asynccom_frame_clear(struct asynccom_frame *frame)
{
	asynccom_frame_update_buffer_size(frame, 0);
}

int asynccom_frame_update_buffer_size(struct asynccom_frame *frame, unsigned size)
{
	unsigned char *new_buffer = NULL;
	unsigned four_aligned = 0;

	return_val_if_untrue(frame, FALSE);

	if (size == 0) {
		if (frame->buffer) {
			ExFreePoolWithTag(frame->buffer, 'ataD');
			frame->buffer = 0;
		}

		frame->buffer_size = 0;
		frame->data_length = 0;

		return TRUE;
	}
	four_aligned = ((size % 4) == 0) ? size : size + (4 - (size % 4));
	new_buffer = (unsigned char *)ExAllocatePoolWithTag(NonPagedPool, four_aligned, 'ataD');

	if (new_buffer == NULL) {
		return FALSE;
	}

	memset(new_buffer, 0, four_aligned);

	if (frame->buffer) {
		if (frame->data_length) {
			/* Truncate data length if the new buffer size is less than the data length */
			frame->data_length = min(frame->data_length, size);

			/* Copy over the old buffer data to the new buffer */
			memmove(new_buffer, frame->buffer, frame->data_length);
		}

		ExFreePoolWithTag(frame->buffer, 'ataD');
	}

	frame->buffer = new_buffer;
	frame->buffer_size = four_aligned;

	return TRUE;
}

int asynccom_frame_add_data(struct asynccom_frame *frame, const unsigned char *data, unsigned length)
{
	return_val_if_untrue(frame, FALSE);
	return_val_if_untrue(length > 0, FALSE);

	/* Only update buffer size if there isn't enough space already */
	if (frame->data_length + length > frame->buffer_size) {
		if (asynccom_frame_update_buffer_size(frame, frame->data_length + length) == FALSE) {
			return FALSE;
		}
	}

	/* Copy the new data to the end of the frame */
	memmove(frame->buffer + frame->data_length, data, length);

	frame->data_length += length;

	return TRUE;
}

int asynccom_frame_remove_data(struct asynccom_frame *frame, unsigned char *destination, unsigned length)
{
	unsigned removal_length = length;
	return_val_if_untrue(frame, FALSE);

	if (removal_length == 0)
		return removal_length;

	if (frame->data_length == 0) {
		removal_length = 0;
		return removal_length;
	}

	/* Make sure we don't remove more data than we have */
	removal_length = min(removal_length, frame->data_length);

	/* Copy the data into the outside buffer */
	if (destination) memmove(destination, frame->buffer, removal_length);

	frame->data_length -= removal_length;

	/* Move the data up in the buffer (essentially removing the old data) */
	memmove(frame->buffer, frame->buffer + removal_length, frame->data_length);

	return removal_length;
}