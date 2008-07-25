/*	$Id$ */

/*-
 * Copyright (c) 2005, 2006, 2008 by Marco Trillo <marcotrillo@gmail.com>
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
 * These routines read/write 80-bit extended-precision floating point
 * numbers in the format used by the Motorola 68881, Motorola 68882,
 * Motorola 68040 and Intel 80x87 FPUs.
 */

#define LIBAIFF 1
#include <libaiff/libaiff.h>
#include "private.h"

#ifdef HAVE_INTEL_80x87

double x87_read_extended(const unsigned char *);
void x87_write_extended(double, unsigned char *);

static void
byteswap(const uint8_t *buffer, uint8_t *data)
{
        uint16_t p;

        /* XXX - big endian */
        p = *((const uint16_t *) &buffer[8]);
        *((uint16_t *) &data[0]) = ARRANGE_ENDIAN_16(p);
        p = *((const uint16_t *) &buffer[6]);
        *((uint16_t *) &data[2]) = ARRANGE_ENDIAN_16(p);
        p = *((const uint16_t *) &buffer[4]);
        *((uint16_t *) &data[4]) = ARRANGE_ENDIAN_16(p);
        p = *((const uint16_t *) &buffer[2]);
        *((uint16_t *) &data[6]) = ARRANGE_ENDIAN_16(p);
        p = *((const uint16_t *) &buffer[0]);
        *((uint16_t *) &data[8]) = ARRANGE_ENDIAN_16(p);
}

double
ieee754_read_extended(const uint8_t *buffer)
{
        uint8_t data[10];

        byteswap(buffer, data);
        return x87_read_extended(data);
}

void
ieee754_write_extended(double d, uint8_t *out)
{
        uint8_t data[10];

        x87_write_extended(d, data);
        byteswap(data, out);
}

#else /* HAVE_INTEL_80x87 */

#include <math.h>
#include <string.h>


/*
 * Infinite & NAN values
 * for non-IEEE systems
 */
#ifndef HUGE_VAL
#ifdef HUGE
#define INFINITE_VALUE	HUGE
#define NAN_VALUE	HUGE
#endif
#else
#define INFINITE_VALUE	HUGE_VAL
#define NAN_VALUE	HUGE_VAL
#endif


/*
 * IEEE 754 Extended Precision
 * 
 * Implementation here is the 80-bit extended precision
 * format of Motorola 68881, Motorola 68882 and Motorola
 * 68040 FPUs, as well as Intel 80x87 FPUs.
 * 
 * See:
 *    http://www.freescale.com/files/32bit/doc/fact_sheet/BR509.pdf
 */
/*
 * Exponent range: [-16383,16383]
 * Precision for mantissa: 64 bits with no hidden bit
 * Bias: 16383
 */

/*
 * Write IEEE Extended Precision Numbers
 */
void
ieee754_write_extended(double in, uint8_t* out)
{
	int sgn, exp, shift;
	double fraction, t;
	unsigned int lexp, hexp;
	uint32_t low, high;

	if (in == 0.0) {
		memset(out, 0, 10);
		return;
	}
	if (in < 0.0) {
		in = fabs(in);
		sgn = 1;
	} else
		sgn = 0;

	fraction = frexp(in, &exp);

	if (exp == 0 || exp > 16384) {
		if (exp > 16384) /* infinite value */
			low = high = 0;
		else {
			low = 0x80000000;
			high = 0;
		}
		exp = 32767;
		goto done;
	}
	fraction = ldexp(fraction, 32);
	t = floor(fraction);
	low = (uint32_t) t;
	fraction -= t;
	t = floor(ldexp(fraction, 32));
	high = (uint32_t) t;

	/* Convert exponents < -16382 to -16382 (then they will be
	 * stored as -16383) */
	if (exp < -16382) {
		shift = 0 - exp - 16382;
		high >>= shift;
		high |= (low << (32 - shift));
		low >>= shift;
		exp = -16382;
	}
	exp += 16383 - 1;	/* bias */

done:
	lexp = ((unsigned int) exp) >> 8;
	hexp = ((unsigned int) exp) & 0xFF;
	
	/* big endian */
	out[0] = ((uint8_t) sgn) << 7;
	out[0] |= (uint8_t) lexp;
	out[1] = (uint8_t) hexp;
	out[2] = (uint8_t) (low >> 24);
	out[3] = (uint8_t) ((low >> 16) & 0xFF);
	out[4] = (uint8_t) ((low >> 8) & 0xFF);
	out[5] = (uint8_t) (low & 0xFF);
	out[6] = (uint8_t) (high >> 24);
	out[7] = (uint8_t) ((high >> 16) & 0xFF);
	out[8] = (uint8_t) ((high >> 8) & 0xFF);
	out[9] = (uint8_t) (high & 0xFF);

	return;
}


/*
 * Read IEEE Extended Precision Numbers
 */
double
ieee754_read_extended(const uint8_t* in)
{
	int sgn, exp;
	uint32_t low, high;
	double out;

	/* Extract the components from the big endian buffer */
	sgn = (int) (in[0] >> 7);
	exp = ((int) (in[0] & 0x7F) << 8) | ((int) in[1]);
	low = (((uint32_t) in[2]) << 24)
		| (((uint32_t) in[3]) << 16)
		| (((uint32_t) in[4]) << 8) | (uint32_t) in[5];
	high = (((uint32_t) in[6]) << 24)
		| (((uint32_t) in[7]) << 16)
		| (((uint32_t) in[8]) << 8) | (uint32_t) in[9];

	if (exp == 0 && low == 0 && high == 0)
		return (sgn ? -0.0 : 0.0);

	switch (exp) {
	case 32767:
		if (low == 0 && high == 0)
			return (sgn ? -INFINITE_VALUE : INFINITE_VALUE);
		else
			return (sgn ? -NAN_VALUE : NAN_VALUE);
	default:
		exp -= 16383;	/* unbias exponent */

	}

	out = ldexp((double) low, -31 + exp);
	out += ldexp((double) high, -63 + exp);

	return (sgn ? -out : out);
}

#endif /* ! HAVE_INTEL_80x87 */


