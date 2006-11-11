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

#ifndef _LIBAIFF_H_
#define _LIBAIFF_H_ 1

#include <stdio.h>
#include <stdlib.h>
#include <libaiff/config.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif


/* == Typedefs == */
typedef uint32_t IFFType ;
typedef uint8_t iext ;
typedef uint16_t MarkerId ;

/* == Struct for AIFF I/O functions == */
struct s_AIFF_WriteRef {
	int segmentSize ;
	int markerPos ;
	FILE* fd ;
	uint32_t len ;
	uint32_t sampleBytes ;
	uint32_t commonOffSet ;
	uint32_t soundOffSet ;
	uint32_t markerOffSet ;
	int stat ;
	void* buffer ;
	size_t buflen ;
	int tics ;
} ;
struct s_AIFF_ReadRef {
	int segmentSize ;
	int flags ;
	int nMarkers ;
	int markerPos ;
	int nChannels ;
	IFFType format ;
	IFFType audioFormat ;
	FILE* fd ;
	uint32_t soundLen ;
	uint32_t pos ;
	int stat ;
	void* buffer ;
	size_t buflen ;
} ;
static const size_t kAiffWriteRefSize = sizeof(struct s_AIFF_WriteRef) ;
static const size_t kAiffReadRefSize = sizeof(struct s_AIFF_ReadRef) ;
typedef struct s_AIFF_WriteRef* AIFF_WriteRef ;
typedef struct s_AIFF_ReadRef* AIFF_ReadRef ;

/* 
 * == Interchange File Format (IFF) ==
 */
#define AIFF_FORM		0x464f524d
#define AIFF_AIFF		0x41494646
#define AIFF_NAME		0x4e414d45
#define AIFF_AUTH		0x41555448
#define AIFF_COPY		0x28632920
#define AIFF_ANNO		0x414e4e4f

struct s_IFFHeader
{
	IFFType hid ;
	uint32_t len ;
	IFFType fid ;
} ;
typedef struct s_IFFHeader IFFHeader ;

struct s_IFFChunk
{
	IFFType id ;
	uint32_t len ;
} ;
typedef struct s_IFFChunk IFFChunk ;

struct s_AIFFCommon
{
	uint16_t numChannels ;
	uint32_t numSampleFrames ;
	uint16_t sampleSize ;
	iext sampleRate ;
} ;
typedef struct s_AIFFCommon CommonChunk ;

struct s_AIFFSound
{
	uint32_t offset ;
	uint32_t blockSize ;
	char samples ;
} ;
typedef struct s_AIFFSound SoundChunk ;

struct s_Marker
{
	MarkerId id ;
	uint32_t position ;
	uint8_t markerNameLen ;
	char markerName ;
} ;
typedef struct s_Marker Marker ;

struct s_AIFFMarker
{
	uint16_t numMarkers ;
	char markers ;
} ;
typedef struct s_AIFFMarker MarkerChunk ;

struct s_AIFFLoop
{
	int16_t playMode ;
	MarkerId beginLoop ;
	MarkerId endLoop ;
	uint16_t garbage ; /* not read (size=6 bytes) */
} ;
typedef struct s_AIFFLoop AIFFLoop ;

/* Play modes */
#define kModeNoLooping			0
#define kModeForwardLooping		1
#define kModeForwardBackwardLooping	2

struct s_Loop
{
	int16_t playMode ;
	uint32_t beginLoop ;
	uint32_t endLoop ;
} ;

struct s_Instrument
{
	int8_t baseNote ;
	int8_t detune ;
	int8_t lowNote ;
	int8_t highNote ;
	int8_t lowVelocity ;
	int8_t highVelocity ;
	int16_t gain ;
	struct s_Loop sustainLoop ;
	struct s_Loop releaseLoop ;
} ;
typedef struct s_Instrument Instrument ;

struct s_Comment
{
	uint32_t timeStamp ;
	MarkerId marker ;
	uint16_t count ;
	char text ;
} ;
typedef struct s_Comment Comment ;

struct s_AIFFComment
{
	uint16_t numComments ;
	char comments ;
} ;
typedef struct s_AIFFComment CommentChunk ;

/* == Function prototypes == */
int IFF_FindChunk(IFFType,int,uint32_t*) ;
AIFF_ReadRef AIFF_Open(const char*) ;
char* AIFF_GetAttribute(AIFF_ReadRef,IFFType) ;
int AIFF_GetInstrumentData(AIFF_ReadRef,Instrument*) ;
size_t AIFF_ReadSamples(AIFF_ReadRef,void*,size_t) ;
int AIFF_Seek(AIFF_ReadRef,uint32_t) ;
int AIFF_ReadSamples32Bit(AIFF_ReadRef,int32_t*,int) ;
int AIFF_ReadMarker(AIFF_ReadRef,int*,uint32_t*,char**) ;
void AIFF_Close(AIFF_ReadRef) ;
int AIFF_GetSoundFormat(AIFF_ReadRef,uint32_t*,int*,int*,int*,int*) ;
AIFF_WriteRef AIFF_WriteOpen(const char*) ;
int AIFF_SetAttribute(AIFF_WriteRef,IFFType,char*) ;
int AIFF_SetSoundFormat(AIFF_WriteRef,int,int,int ) ;
int AIFF_StartWritingSamples(AIFF_WriteRef) ;
int AIFF_WriteSamples(AIFF_WriteRef,void*,size_t) ;
int AIFF_WriteSamples32Bit(AIFF_WriteRef,int32_t*,int) ;
int AIFF_EndWritingSamples(AIFF_WriteRef) ;
int AIFF_StartWritingMarkers(AIFF_WriteRef) ;
int AIFF_WriteMarker(AIFF_WriteRef,uint32_t,char*) ;
int AIFF_EndWritingMarkers(AIFF_WriteRef) ;
int AIFF_WriteClose(AIFF_WriteRef) ;

#ifndef LIBAIFF

#ifdef HAVE_BZERO
#undef HAVE_BZERO
#endif
#ifdef HAVE_INTTYPES_H
#undef HAVE_INTTYPES_H
#endif
#ifdef HAVE_MEMORY_H
#undef HAVE_MEMORY_H
#endif
#ifdef HAVE_MEMSET
#undef HAVE_MEMSET
#endif
#ifdef HAVE_STDINT_H
#undef HAVE_STDINT_H
#endif
#ifdef HAVE_STDLIB_H
#undef HAVE_STDLIB_H
#endif
#ifdef HAVE_STRINGS_H
#undef HAVE_STRINGS_H
#endif
#ifdef HAVE_STRING_H
#undef HAVE_STRING_H
#endif
#ifdef HAVE_SYS_STAT_H
#undef HAVE_SYS_STAT_H
#endif
#ifdef HAVE_SYS_TYPES_H
#undef HAVE_SYS_TYPES_H
#endif
#ifdef HAVE_UNISTD_H
#undef HAVE_UNISTD_H
#endif

#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#ifdef STDC_HEADERS
#undef STDC_HEADERS
#endif
#ifdef WORDS_BIGENDIAN
#undef WORDS_BIGENDIAN
#endif

#endif


#endif

