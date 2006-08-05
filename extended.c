/* $Id$ */
/*-
 * Copyright (c) 2005, 2006 Marco Trillo <marcotrillo@gmail.com>
 * All rights reserceed.
 *
 * This source code is licensed under the GNU Lesser General
 * Public License, as described on the file <LGPL.txt>
 * which should be included in the distribution.
 *
 */
/*
 * ~~~~~~~~~ Implemented AIFF version: ~~~~~~~~~~~~
 * Audio Interchange File Format (AIFF) version 1.3
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#define LIBAIFF 1
#include <libaiff/libaiff.h>
#include "private.h"
#include <math.h>
#include <string.h>


/* Infinite & NAN values
* for non-IEEE systems
*/
#ifndef HUGE_VAL
#  ifdef HUGE
#    define INFINITE_VALUE	HUGE
#    define NAN_VALUE	HUGE
#  endif
#else
#  define INFINITE_VALUE	HUGE_VAL
#  define NAN_VALUE	HUGE_VAL
#endif


/*
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * IEEE Extended Precision
 *
 * Implementation here is Motorola 680x0 / Intel 80x87 80-bit
 * "extended" data type.
 *
 * Exponent range: [-16383,16383]
 * Precision for mantissa: 64-bits with no hidden bit
 * Bias: 16383
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

/*
 * Write IEEE Extended Precision Numbers
 */
void
ieee754_write_extended (double in, unsigned char *out)
{
    int sgn;
    int exp;
    int shift;
    double fraction;
    double t;
    unsigned int lexp, hexp;
    uint32_t low, high;
    
    if (in == 0.0)
     {
        memset (out, 0, 10);
        return;
     }
    
    if (in < 0.0)
     {
        in = fabs (in);
        sgn = 1;
     }
    else
        sgn = 0;
    
    fraction = frexp (in, &exp);
    
    if (exp == 0 || exp > 16384)
     {
        if (exp > 16384 || fraction == INFINITE_VALUE)
            low = high = 0;
        else
         {
            low = 0x80000000;
            high = 0;
         }
        exp = 32767;
        goto done;
     }
    
    fraction = ldexp (fraction, 32);
    t = floor (fraction);
    low = (uint32_t) t;
    fraction -= t;
    t = floor (ldexp (fraction, 32));
    high = (uint32_t) t;
    
    /* Convert exponents lesser than -16382
     * to -16382
     * (then they will be stored as -16383)
     */
    if (exp < -16382)
     {
        shift = 0 - exp - 16382;
        high >>= shift;
        high |= (low << (32 - shift));
        low >>= shift;
        exp = -16382;
     }
    exp += 16383 - 1;		/* bias */
    
done:
    lexp = ((unsigned int) exp) >> 8;
    hexp = ((unsigned int) exp) & 0xFF;

#if 0 
    if (flags & IEEE754_ENDIAN_XINU)
     {
        out[9] = ((unsigned char) sgn) << 7;
        out[9] |= (unsigned char) lexp;
        out[8] = hexp;
        out[7] = low >> 24;
        out[6] = (low >> 16) & 0xFF;
        out[5] = (low >> 8) & 0xFF;
        out[4] = low & 0xFF;
        out[3] = high >> 24;
        out[2] = (high >> 16) & 0xFF;
        out[1] = (high >> 8) & 0xFF;
        out[0] = high & 0xFF;
     }
    else
     {
#endif

    out[0] = ((unsigned char) sgn) << 7;
    out[0] |= (unsigned char) lexp;
    out[1] = hexp;
    out[2] = low >> 24;
    out[3] = (low >> 16) & 0xFF;
    out[4] = (low >> 8) & 0xFF;
    out[5] = low & 0xFF;
    out[6] = high >> 24;
    out[7] = (high >> 16) & 0xFF;
    out[8] = (high >> 8) & 0xFF;
    out[9] = high & 0xFF;
     
#if 0
     }
#endif
    
    return;
}

/*
 * Read IEEE Extended Precision Numbers
 */
double
ieee754_read_extended (unsigned char *in)
{
    int sgn;
    int exp;
    uint32_t low, high;
    double out;
    
    /* Extract the components from the buffer */
#if 0
    if (flags & IEEE754_ENDIAN_XINU)
     {
        sgn = (int) (in[9] >> 7);
        exp = ((int) (in[9] & 0x7F) << 8) | ((int) in[8]);
        low = (((uint32_t) in[7]) << 24)
            | (((uint32_t) in[6]) << 16)
            | (((uint32_t) in[5]) << 8) | (uint32_t) in[4];
        high = (((uint32_t) in[3]) << 24)
            | (((uint32_t) in[2]) << 16)
            | (((uint32_t) in[1]) << 8) | (uint32_t) in[0];
     }
    else
     {
#endif

    sgn = (int) (in[0] >> 7);
    exp = ((int) (in[0] & 0x7F) << 8) | ((int) in[1]);
    low = (((uint32_t) in[2]) << 24)
        | (((uint32_t) in[3]) << 16)
        | (((uint32_t) in[4]) << 8) | (uint32_t) in[5];
    high = (((uint32_t) in[6]) << 24)
        | (((uint32_t) in[7]) << 16)
        | (((uint32_t) in[8]) << 8) | (uint32_t) in[9];
#if 0
     }
#endif

    if (exp == 0 && low == 0 && high == 0)
        return (sgn ? -0.0 : 0.0);
    
    switch (exp)
     {
        case 32767:
            if (low == 0 && high == 0)
                return (sgn ? -INFINITE_VALUE : INFINITE_VALUE);
            else
                return (sgn ? -NAN_VALUE : NAN_VALUE);
        default:
            exp -= 16383;		/* unbias exponent */
            
     }
    
    out = ldexp ((double) low, -31 + exp);
    out += ldexp ((double) high, -63 + exp);
    
    return (sgn ? -out : out);
}



