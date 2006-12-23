/*	$Id$ */
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

#define LIBAIFF 1
#include <stdio.h>
#include <stdlib.h>
#include <libaiff/libaiff.h>
#include <libaiff/endian.h>
#include "private.h"


/*
 * IEEE-754 32-bit single-precision floating point
 *
 * This is a conversor to linear 32-bit integer PCM
 * using only integer arithmetic.
 * 
 * Some IEEE-754 documentation and references on the WWW:
 * -------------------------------------------------------------
 * http://stevehollasch.com/cgindex/coding/ieeefloat.html
 * http://www.answers.com/topic/ieee-floating-point-standard
 * http://www.cs.berkeley.edu/~wkahan/ieee754status/IEEE754.PDF
 * http://www.psc.edu/general/software/packages/ieee/ieee.html
 * http://www.duke.edu/~twf/cps104/floating.html
 * -------------------------------------------------------------
 */

static int32_t
float32dec(uint32_t in)
{
	int sgn, exp;
	uint32_t mantissa;

	if (in == 0 || in == 0x80000000)
		return (0);

	sgn = (int) (in >> 31);
	exp = (int) ((in >> 23) & 0xFF);
	mantissa = in & 0x7FFFFF;

	if (exp == 255) {
		/*
		 * Infinity and NaN.
		 * XXX: what we should do with these?
		 */
		if (mantissa == 0) /* infinity */
			return (int32_t) (0x7FFFFFFF);
		else /* NaN */
			return (0);
	} else if (exp == 0) {
		exp = -126; /* denormalized number */
	} else {
		/* Normalized numbers */
		mantissa |= 0x800000; /* hidden bit */
		exp -= 127; /* unbias exponent */
	}
	
	/*
	 * Quantization to linear PCM 31 bits.
	 * 
	 * Multiply the mantissa by 2^(-23+exp) to get the floating
	 * point value, and then multiply by 2^31 to quantize
	 * the floating point to 31 bits. So:
	 * 
	 * 2^(-23+exp) * 2^31 = 2^(8 + exp)
	 */
	exp += 8;
	if (exp < 0) {
		exp = -exp;
		mantissa >>= exp;
	} else {
		mantissa <<= exp;
	}
	
	if (sgn) {
		int32_t val;
		
		if (mantissa == 0x80000000)
			return (-2147483647 - 1); /* -(2^31) */
		else {
			val = (int32_t) mantissa;
			return (-val);
		}
	} else {
		/*
		 * if mantissa is 0x80000000 (for a 1.0 float),
		 * we should return +2147483648 (2^31), but in
		 * two's complement arithmetic the max. positive value
		 * is +2147483647, so clip the result.
		 */
		if (mantissa == 0x80000000)
			return (2147483647);
		else
			return ((int32_t) mantissa);
	}
}

static void
float32_decode(void *dst, void *src, int n)
{
	uint32_t *srcSamples = (uint32_t *) src;
	int32_t *dstSamples = (int32_t *) dst;

	while (n-- > 0)
		dstSamples[n] = float32dec(srcSamples[n]);
}

static void
float32_swap_samples(void *buffer, int n)
{
	uint32_t *streams = (uint32_t *) buffer;

	while (n-- > 0)
		streams[n] = ARRANGE_BE32(streams[n]);
}

size_t 
do_float32(AIFF_ReadRef r, void *buffer, size_t len)
{
	int n;
	uint32_t clen;
	size_t slen;
	size_t bytes_in;
	size_t bytesToRead;

	n = (int) len;
	/* 'n' should be divisible by 4 (32 bits) */
	while (n >= 0 && ((n & 3) != 0)) {
		--n;
		--len;
	}
	n >>= 2;

	slen = (size_t) (r->soundLen - r->pos);
	bytesToRead = MIN(len, slen);

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

	bytes_in = fread(r->buffer2, 1, bytesToRead, r->fd);
	if (bytes_in > 0)
		clen = (uint32_t) bytes_in;
	else
		clen = 0;
	r->pos += clen;

	float32_swap_samples(r->buffer2, n);
	float32_decode(buffer, r->buffer2, n);

	return bytes_in;
}

int 
float32_seek(AIFF_ReadRef r, uint32_t pos)
{
	long of;
	uint32_t b;

	b = pos * r->nChannels * 4;
	if (b >= r->soundLen)
		return 0;
	of = (long) b;

	if (fseek(r->fd, of, SEEK_CUR) < 0) {
		return -1;
	}
	r->pos = b;
	return 1;
}


