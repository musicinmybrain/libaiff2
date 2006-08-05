/* $Id$ */
/*-
 * Copyright (c) 2005, 2006 Marco Trillo <marcotrillo@gmail.com>
 * All rights reserved.
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
#include <libaiff/endian.h>
#include "private.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

void lpcm_swap_samples(int segmentSize,int flags,void* from,void* to,int nsamples)
{
	register int n = nsamples;
	register int i;
	/* 8 bit */
	uint8_t* fubytes = (uint8_t*)from ;
	uint8_t* ubytes = (uint8_t*)to ;
	/* 16 bit */
	int16_t* fwords = (int16_t*)from ;
	int16_t* words = (int16_t*)to ;
	/* 32 bit */
	int32_t* fdwords = (int32_t*)from ;
	int32_t* dwords = (int32_t*)to ;
	/* 24 bit */
	uint8_t x,y,z ;
	
	switch( segmentSize )
	{
		case 2:
			switch( flags )
			{
#ifndef WORDS_BIGENDIAN
				case LPCM_BIG_ENDIAN:
					for( i=0; i<n; ++i )
						words[i] = ARRANGE_BE16( fwords[i] ) ;
					break ;
#else
				case LPCM_LTE_ENDIAN:
					for( i=0; i<n; ++i )
						words[i] = ARRANGE_LE16( fwords[i] ) ;
					break ;
#endif
			}
			break ;
		case 3:
#ifdef WORDS_BIGENDIAN
			if( flags == LPCM_LTE_ENDIAN )
#else
			if( flags == LPCM_BIG_ENDIAN )
#endif
			{
				n *= 3 ;
				for( i=0; i<n; i += 3 )
				{
					x = fubytes[i] ;
					y = fubytes[i+1] ;
					z = fubytes[i+2] ;

					ubytes[i]   = z ;
					ubytes[i+1] = y ;
					ubytes[i+2] = x ;
				}
				n /= 3 ;
			}
			break ;
		case 4:
			switch( flags )
			{
#ifndef WORDS_BIGENDIAN
				case LPCM_BIG_ENDIAN:
					for( i=0; i<n; ++i )
						dwords[i] = ARRANGE_BE32( fdwords[i] ) ;
					break ;
#else
				case LPCM_LTE_ENDIAN:
					for( i=0; i<n; ++i )
						dwords[i] = ARRANGE_LE32( fdwords[i] ) ;
					break ;
#endif
			}
			break ;
	}

	return;
}

size_t do_lpcm(AIFF_ReadRef r,void* buffer,size_t len)
{
	int n;
	uint32_t clen ;
	size_t slen ;
	size_t bytes_in ;
	size_t bytesToRead ;

	n = (int)len ;
	while( n >= 0 && n % r->segmentSize != 0 )
	{
		--n ;
		--len ;
	}
	n /= r->segmentSize ;
	
	slen = (size_t)(r->soundLen - r->pos) ;
	bytesToRead = ( slen > len ? len : slen ) ;

	if( bytesToRead == 0 )
		return 0 ;
	
	bytes_in = fread( buffer, 1, bytesToRead , r->fd ) ;
	if( bytes_in > 0 )
		clen = (uint32_t)bytes_in ;
	else
		clen = 0 ;
	r->pos += clen ;

	lpcm_swap_samples( r->segmentSize, r->flags, buffer, buffer, n );

	return bytes_in ;
}

int lpcm_seek(AIFF_ReadRef r,uint32_t pos)
{
	long of ;
	uint32_t b ;

	b = pos * r->nChannels * r->segmentSize ;
	if( b >= r->soundLen )
		return 0 ;
	of = (long)b ;

	if( fseek( r->fd,of,SEEK_CUR ) < 0 )
	{
		return -1 ;
	}

	r->pos = b ;
	return 1 ;
}

