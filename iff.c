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
	uint8_t temp[8];

	if (fseek(fd, 12, SEEK_SET) == -1) {
		return 0;
	}
	for (;;) {
		if (fread(temp, 1, 8, fd) < 8)
			return 0;

		memcpy(length, temp + 4, 4);

		*length = ARRANGE_BE32(*length);

		if (rpl_memcmp(&chunk, temp, 4) != 0) {
			/*
			 * In Electronic Arts IFF files (like AIFF)
			 * chunk start positions must be even.
			 */
			if (*length & 0x1)	/* if( *length % 2 != 0 ) */
				++(*length);

			if (fseek(fd, (long) (*length), SEEK_CUR) == -1) {
				return 0;
			}
		} else {
			break;
		}

	}

	return 1;
}

char *
get_iff_attribute(AIFF_ReadRef r, IFFType attrib)
{
	int n = 4;
	IFFType *sattrib;
	char *attributes[4] = {NameID, AuthID, AnnoID, CopyID};
	char *str;
	uint32_t len;

	/*
	 * Check if 'attrib' is a valid IFF attribute
	 */
	while (n--) {
		sattrib = (IFFType *) attributes[n];
		if (attrib == *sattrib)
			break;
		if (n == 0)
			return NULL;
	}

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
	int n = 4;
	IFFType *sattrib;
	char *attributes[4] = {NameID, AuthID, AnnoID, CopyID};
	char car = 0;
	IFFChunk chk;
	uint32_t len = strlen(str);

	/*
	 * Check if 'attrib' is a valid IFF attribute
	 */
	while (n--) {
		sattrib = (IFFType *) attributes[n];
		if (attrib == *sattrib)
			break;
		if (n == 0)
			return 0;
	}

	memcpy(&(chk.id), &attrib, 4);
	chk.len = ARRANGE_BE32(len);

	if (fwrite(&chk, 1, 8, w->fd) < 8 ||
	    fwrite(str, 1, len, w->fd) < len) {
		return -1;
	}
	/*
	 * Write a pad byte if chunk length is odd,
	 * as required by the IFF specification.
	 */
	if (len & 0x1) {
		if (fwrite(&car, 1, 1, w->fd) < 1) {
			return -1;
		}
		(w->len)++;
	}
	w->len += 8 + len;

	return 1;
}
