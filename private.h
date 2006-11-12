/* $Id$ */

#if !defined(HAVE_MEMSET) && defined(HAVE_BZERO)
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#define memset(a, b, c) bzero((a), (c))

#endif

#ifdef ASSERT
#undef ASSERT
#endif

void AIFFAssertionFailed (const char*, int);

#define ASSERT(x) if(!(x)) AIFFAssertionFailed(__FILE__, __LINE__)

/* == Supported formats == */

/* File formats */
#define AIFF_TYPE_IFF  ARRANGE_BE32(0x464F524D)
#define AIFF_TYPE_AIFF ARRANGE_BE32(0x41494646)
#define AIFF_TYPE_AIFC ARRANGE_BE32(0x41494643)

/* Audio encoding formats */
#define AUDIO_FORMAT_LPCM  ARRANGE_BE32(0x4E4F4E45)
#define AUDIO_FORMAT_lpcm  ARRANGE_BE32(0x6C70636D)
#define AUDIO_FORMAT_twos  ARRANGE_BE32(0x74776F73)
#define AUDIO_FORMAT_sowt  ARRANGE_LE32(0x74776F73)
#define AUDIO_FORMAT_ULAW  ARRANGE_BE32(0x554C4157)
#define AUDIO_FORMAT_ulaw  ARRANGE_BE32(0x756C6177)
#define AUDIO_FORMAT_ALAW  ARRANGE_BE32(0x414C4157)
#define AUDIO_FORMAT_alaw  ARRANGE_BE32(0x616C6177)

/* OSTypes */
#define AIFF_FORM 0x464f524d
#define AIFF_AIFF 0x41494646
#define AIFF_FVER 0x46564552
#define AIFF_COMM 0x434f4d4d
#define AIFF_SSND 0x53534e44
#define AIFF_MARK 0x4d41524b
#define AIFF_INST 0x494e5354
#define AIFF_COMT 0x434f4d54

/* Flags for LPCM format */
#define LPCM_BIG_ENDIAN   (1<<0)
#define LPCM_LTE_ENDIAN   (1<<1)
#define LPCM_SYS_ENDIAN   (1<<2)


/* iff.c */
int find_iff_chunk(IFFType chunk,FILE* fd,uint32_t* length) ;
char* get_iff_attribute(AIFF_ReadRef r,IFFType attrib) ;
int set_iff_attribute(AIFF_WriteRef w,IFFType attrib,char* str) ;

/* aifx.c */
int get_aifx_format(AIFF_ReadRef r,uint32_t* nSamples,int* channels,
		int* samplingRate,int* bitsPerSample,int* segmentSize,
		IFFType* audioFormat,int* flags) ;
int read_aifx_marker(AIFF_ReadRef r,int* id,uint32_t* position,char** name) ;
int get_aifx_instrument(AIFF_ReadRef r,Instrument* inpi) ;
int do_aifx_prepare(AIFF_ReadRef r) ;

/* lpcm.c */
void lpcm_swap_samples(int,int,void*,void*,int) ;
size_t do_lpcm(AIFF_ReadRef r,void* buffer,size_t len) ;
int lpcm_seek(AIFF_ReadRef r,uint32_t pos) ;

/* extended.c */
void ieee754_write_extended (double, unsigned char*);
double ieee754_read_extended (unsigned char*);


