/* $Id$ */
/*-
 * Copyright (c) 2005, 2006 by Marco Trillo <marcotrillo@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any
 * person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the
 * Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the
 * Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice
 * shall be included in all copies or substantial portions of
 * the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 * KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/*
 * ~~~~~~~~~ Implemented AIFF version: ~~~~~~~~~~~~
 * Audio Interchange File Format (AIFF) version 1.3
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#define LIBAIFF 1
#include <libaiff/libaiff.h>
#include <libaiff/endian.h>
#include "private.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif


int 
find_iff_chunk(IFFType chunk, FILE * fd, uint32_t * length)
{
	union cio {
		uint8_t buf[8];
		IFFChunk chk;
	} d;
	
	ASSERT(sizeof(IFFChunk) == 8);
	chunk = ARRANGE_BE32(chunk);

	if (fseek(fd, 12, SEEK_SET) == -1) {
		return 0;
	}
	for (;;) {
		if (fread(d.buf, 1, 8, fd) < 8)
			return 0;
		
		d.chk.len = ARRANGE_BE32(d.chk.len);
		if (d.chk.id != chunk) {
			/*
			 * In Electronic Arts IFF files (like AIFF)
			 * chunk start positions must be even.
			 */
			if (d.chk.len & 1)	/* if( *length % 2 != 0 ) */
				++(d.chk.len);

			if (fseek(fd, (long) d.chk.len, SEEK_CUR) == -1) {
				return 0;
			}
		} else {
			*(length) = d.chk.len;
			break;
		}

	}

	return 1;
}

char *
get_iff_attribute(AIFF_ReadRef r, IFFType attrib)
{
	char *str;
	uint32_t len;
	
	if (!find_iff_chunk(attrib, r->fd, &len))
		return NULL;

	if (!len)
		return NULL;

	str = malloc(len + 1);
	if (!str)
		return NULL;

	if (fread(str, 1, len, r->fd) < len) {
		free(str);
		return NULL;
	}
	str[len] = '\0';
	r->stat = 0;

	return str;
}

int 
set_iff_attribute(AIFF_WriteRef w, IFFType attrib, char *str)
{
	uint8_t car = 0x0;
	IFFChunk chk;
	uint32_t len = strlen(str);
	
	ASSERT(sizeof(IFFChunk) == 8);
	chk.id = ARRANGE_BE32(attrib);
	chk.len = ARRANGE_BE32(len);

	if (fwrite(&chk, 1, 8, w->fd) < 8 ||
	    fwrite(str, 1, len, w->fd) < len) {
		return -1;
	}
	/*
	 * Write a pad byte if chunk length is odd,
	 * as required by the IFF specification.
	 */
	if (len & 1) {
		if (fwrite(&car, 1, 1, w->fd) < 1) {
			return -1;
		}
		(w->len)++;
	}
	w->len += 8 + len;

	return 1;
}
