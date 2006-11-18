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
 * A-Law (A-Law)
 * 
 * Digital A-Law is a 8-bit floating point representation
 * of 13-bit PCM samples, with a 4-bit mantissa and a 3-bit
 * exponent.
 * 
 * This gives 5-bit (4 + hidden bit) mantissa precision for all samples,
 * so low samples (with zeros in the most-significant bits) get
 * more precision with this encoding than with normal 8-bit LPCM.
 * 
 * For more information on Digital A-Law:
 *   http://shannon.cm.nctu.edu.tw/comtheory/chap3.pdf
 *   http://en.wikipedia.org/wiki/G.711
 */
static int16_t
alawdec (uint8_t x)
{
	int sgn, exp, mant;
	int y;
	
	/*
	 * The bits at even positions are inversed.
	 * In addition, the sign bit is inversed too.
	 */
	x = ((~x & 0xD5) | (x & 0x2A));
	sgn = x & 0x80;
	x &= 0x7F;
	mant = (int) ((x & 0xF) << 1) | 0x21;
	exp = (int) ((x & 0x70) >> 4);
	
	/*
	 * Denormalized numbers? then remove hidden bit
	 */
	if (exp == 0)
		mant &= ~0x20;
	else
		mant <<= (exp - 1);
	
	y = (sgn ? -mant : mant);
	return (int16_t) (y << 3); /* convert 13-bit to 16-bit */
}

/*
 * XXX : this is identical to the do_ulaw() function, except the call to alawdec().
 * Maybe we should unify the u-Law & A-Law encodings in a do_ualaw() ?
 */
size_t
do_alaw (AIFF_ReadRef r, void *buffer, size_t len)
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
		if (r->buffer2 != NULL)
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
		samples[i] = alawdec(bytes[i]);
	}
	
	return (bytesRead << 1); /* bytesRead * 2 */
}


int 
alaw_seek (AIFF_ReadRef r, uint32_t pos)
{
	/*
	 * Seeking is the same in both
	 * u-Law & A-Law
	 */
	return ulaw_seek(r, pos);
}
