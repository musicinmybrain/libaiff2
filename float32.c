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
 * IEEE-754 single precision floating point
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

	/* quantization */
	mantissa <<= (8 + exp); /* 31 - 23 */

	if (sgn) {
		int32_t val;

		if (mantissa == 0x80000000) {
#if 0
			return (-2147483648); /* -(2^31) */
#else
			/*
			 * ISO C90 does not have negative constants,
			 * so we can't simply return -2147483648
			 * (since 2147483648 can't be represented
			 *  by two's complement positive numbers.)
			 * This is a workaround.
			 */
			val = -2147483647;
			return (val - 1); /* -(2^31) */
#endif
		} else {
			val = (int32_t)mantissa;
			return (-val);
		}
	} else {
		if (mantissa == 0x80000000) /* 1.0 ? */
			--mantissa; /* clip to +(2^31 - 1) */
		return (int32_t)mantissa;
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


