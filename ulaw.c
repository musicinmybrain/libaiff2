/* $Id$ */
/*-
 * Copyright (c) 2006 by Marco Trillo <marcotrillo@gmail.com>
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
#include "private.h"

/*
 * mu-Law (u-Law)
 * 
 * Digital mu-Law is a 8-bit floating point representation
 * of 14-bit PCM samples, with a 4-bit mantissa and a 3-bit
 * exponent.
 * 
 * This gives a full 4-bit binary fraction for all samples,
 * so low samples (with zeros in the most-significant bits) get
 * more precision with this encoding than with normal 8-bit LPCM.
 */
static int16_t
ulawdec (uint8_t x)
{
	int sgn, exp, mant;
	int y;
	
	x = ~x; /* bits are sent reversed */
	sgn = x & 0x80; /* sign */
	x &= 0x7F;
	mant = (int) ((x & 0xF) << 1) | 0x21; /* mantissa plus hidden bits */
	exp = (int) ((x & 0x70) >> 4); /* exponent */
	mant <<= exp; /* raise mantissa to 2^exponent */
	
	/*
	 * Subtract 33 from the 'raised output'
	 * to get the normal output (14-bit),
	 * and then convert the 14-bit output
	 * to 16-bit
	 */
	y = (sgn ? (-mant + 33) : (mant - 33));
	return (int16_t) (y << 2);
}


size_t
do_ulaw (AIFF_ReadRef r, void *buffer, size_t len)
{
	size_t n, i, rem, bytesToRead, bytesRead;
	uint8_t* bytes;
	int16_t* samples;
	
	/* length must be even (16-bit samples) */
	if (len & 1)
		--len;
	
	n = len >> 1;
	rem = r->soundLen - r->pos;
	bytesToRead = MIN(n, rem);
	if (bytesToRead == 0)
		return 0;
	
	if (r->buffer2 == NULL || r->buflen2 < bytesToRead) {
		free(r->buffer2);
		r->buffer2 = malloc(bytesToRead);
		if (r->buffer2 == NULL) {
			r->buflen2 = 0;
			return 0;
		}
		r->buflen2 = bytesToRead;
	}
	
	bytesRead = fread(r->buffer2, 1, bytesToRead, r->fd);
	if (bytesRead > 0) {
		r->pos += bytesRead;
	} else {
		return 0;
	}
	
	bytes = (uint8_t *) (r->buffer2);
	samples = (int16_t *) buffer;
	for (i = 0; i < bytesRead; ++i) {
		samples[i] = ulawdec(bytes[i]);
	}
	
	return (bytesRead << 1); /* bytesRead * 2 */
}

	
int 
ulaw_seek (AIFF_ReadRef r, uint32_t pos)
{
	long of;
	uint32_t b;
	
	b = pos * r->nChannels;
	if (b >= r->soundLen)
		return 0;
	of = (long) b;
	
	if (fseek(r->fd, of, SEEK_CUR) < 0) {
		return -1;
	}
	r->pos = b;
	return 1;
}
