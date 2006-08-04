/* $ libaiff -- endian.h $ */
/*-
 * Copyright (c) 2005 Marco Trillo <marcotrillo@gmail.com>
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


/* === Endian-related stuff === */

#ifndef WORDS_BIGENDIAN

#define ARRANGE_BE16(dat) ( (((dat) & 0xff00 ) >> 8 ) | (((dat) & 0x00ff ) << 8 ) )
#define ARRANGE_BE32(dat) ( (((dat) & 0xff000000 ) >> 24 ) | (((dat) & 0x00ff0000 ) >> 8 ) | (((dat) & 0x0000ff00 ) << 8 ) | (((dat) & 0x000000ff ) << 24 ) )
#define ARRANGE_LE16(dat) (dat)
#define ARRANGE_LE32(dat) (dat)


#else

#define ARRANGE_BE16(dat) (dat)
#define ARRANGE_BE32(dat) (dat)
#define ARRANGE_LE16(dat) ( (((dat) & 0xff00 ) >> 8 ) | (((dat) & 0x00ff ) << 8 ) )
#define ARRANGE_LE32(dat) ( (((dat) & 0xff000000 ) >> 24 ) | (((dat) & 0x00ff0000 ) >> 8 ) | (((dat) & 0x0000ff00 ) << 8 ) | (((dat) & 0x000000ff ) << 24 ) )

#endif


