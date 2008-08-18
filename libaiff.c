/*	$Id$	*/

/*-
 * Copyright (c) 2005, 2006, 2007 Marco Trillo
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
#include <string.h>
#include <stdlib.h>
#include <libaiff/libaiff.h>
#include <libaiff/endian.h>
#include "private.h"

static struct decoder* decoders[] = {
	&lpcm,
	&ulaw,
	&alaw,
	&float32,
	NULL
};

static AIFF_Ref AIFF_ReadOpen (const char *, int);
static AIFF_Ref AIFF_WriteOpen (const char *, int);
static void AIFF_ReadClose (AIFF_Ref);
static int AIFF_WriteClose (AIFF_Ref);
static void* InitBuffer (AIFF_Ref, size_t);
static void DestroyBuffer (AIFF_Ref);
static int DoWriteSamples (AIFF_Ref, void *, size_t, int);
static int Prepare (AIFF_Ref);
static void Unprepare (AIFF_Ref);
static struct decoder* FindDecoder (IFFType);

AIFF_Ref
AIFF_OpenFile(const char *file, int flags)
{
	AIFF_Ref ref = NULL;
	
	if (flags & F_RDONLY) {
		ref = AIFF_ReadOpen(file, flags);
	} else if (flags & F_WRONLY) {
		ref = AIFF_WriteOpen(file, flags);
	}

	return ref;
}

int
AIFF_CloseFile(AIFF_Ref ref)
{
	int r;
	
	if (!ref)
		return -1;
	if (ref->flags & F_RDONLY) {
		AIFF_ReadClose(ref);
		r = 1;
	} else if (ref->flags & F_WRONLY) {
		r = AIFF_WriteClose(ref);
	} else {
		r = -1;
	}
	
	return r;
}

static AIFF_Ref 
AIFF_ReadOpen(const char *file, int flags)
{
	AIFF_Ref r;
	IFFHeader hdr;

	r = malloc(kAIFFRefSize);
	if (!r) {
		return NULL;
	}
	r->fd = fopen(file, "rb");
	if (r->fd == NULL) {
		free(r);
		return NULL;
	}
	r->flags = F_RDONLY | flags;
	if (fread(&hdr, 1, 4, r->fd) < 4) {
		fclose(r->fd);
		free(r);
		return NULL;
	}
	switch (hdr.hid) {
	case AIFF_TYPE_IFF:
		/* Continue reading the IFF header */
		if (fread(&(hdr.len), 1, 8, r->fd) < 8) {
			fclose(r->fd);
			free(r);
			return NULL;
		}
		if (hdr.len == 0) {
			fclose(r->fd);
			free(r);
			return NULL;
		}
		/*
 		 * Check the format type (AIFF or AIFC)
 		 */
		r->format = hdr.fid;
		switch (r->format) {
		case AIFF_TYPE_AIFF:
		case AIFF_TYPE_AIFC:
			break;
		default:
			fclose(r->fd);
			free(r);
			return NULL;
		}

		if (init_aifx(r) < 1) {
			fclose(r->fd);
			free(r);
			return NULL;
		}
		break;
	default:
		fclose(r->fd);
		free(r);
		return NULL;
	}

	r->stat = 0;
	r->buffer = NULL;
	r->buflen = 0;

	return r;
}

char *
AIFF_GetAttribute(AIFF_Ref r, IFFType attrib)
{
	if (!r || !(r->flags & F_RDONLY))
		return NULL;
	Unprepare(r);
	
	switch (r->format) {
	case AIFF_TYPE_AIFF:
	case AIFF_TYPE_AIFC:
		return get_iff_attribute(r, attrib);
	default:
		return NULL;
	}
	return NULL;
}

int 
AIFF_ReadMarker(AIFF_Ref r, int *id, uint64_t * pos, char **name)
{
	if (!r || !(r->flags & F_RDONLY))
		return -1;
	
	switch (r->format) {
	case AIFF_TYPE_AIFF:
	case AIFF_TYPE_AIFC:
		return read_aifx_marker(r, id, pos, name);
	default:
		return 0;
	}
	return 0;
}

int 
AIFF_GetInstrumentData(AIFF_Ref r, Instrument * i)
{
	if (!r || !(r->flags & F_RDONLY))
		return (-1);
	Unprepare(r);
	
	switch (r->format) {
	case AIFF_TYPE_AIFF:
	case AIFF_TYPE_AIFC:
		return get_aifx_instrument(r, i);
	default:
		return (0);
	}
}

int 
AIFF_GetAudioFormat(AIFF_Ref r, uint64_t * nSamples, int *channels,
    double *samplingRate, int *bitsPerSample, int *segmentSize)
{
	if (!r || !(r->flags & F_RDONLY))
		return (-1);

	if (nSamples)
		*nSamples = r->nSamples;
	if (channels)
		*channels = r->nChannels;
	if (samplingRate)
		*samplingRate = r->samplingRate;
	if (bitsPerSample)
		*bitsPerSample = r->bitsPerSample;
	if (segmentSize)
		*segmentSize = r->segmentSize;

	return (1);
}

static int
Prepare (AIFF_Ref r)
{
	int res;
	struct decoder *dec;
	
	if (r->stat != 1) {
		switch (r->format) {
		case AIFF_TYPE_AIFF:
		case AIFF_TYPE_AIFC:
			res = do_aifx_prepare(r);
			break;
		default:
			res = -1;
		}
		if (res < 1)
			return res;
		if ((dec = FindDecoder(r->audioFormat)) == NULL)
			return -1;
		if (dec->construct) {
			if ((res = dec->construct(r)) < 1)
				return res;
		}

		r->decoder = dec;
		r->stat = 1;
	}

	return 1;
}

static void
Unprepare (AIFF_Ref r)
{
	struct decoder *dec;
	
	if (r->stat == 1) {
		dec = r->decoder;
		if (dec->destroy)
			dec->destroy(r);
	}
	r->stat = 0;
}

static struct decoder*
FindDecoder (IFFType fmt)
{
	struct decoder **dd, *d;
	
	for (dd = decoders; (d = *dd) != NULL; ++dd)
	  {
		if (d->fmt == fmt)
			return d;
	  }
	
	return NULL;
}

size_t 
AIFF_ReadSamples(AIFF_Ref r, void *buffer, size_t len)
{
	struct decoder *dec;
	
	if (!r || !(r->flags & F_RDONLY) || Prepare(r) < 1)
		return 0;
	dec = r->decoder;
	
	return dec->read_lpcm(r, buffer, len);
}

int
AIFF_ReadSamplesFloat(AIFF_Ref r, float *buffer, int nSamplePoints)
{
	int res;
	struct decoder *dec;
	
	if (!r || !(r->flags & F_RDONLY))
		return -1;
	if (nSamplePoints % (r->nChannels) != 0)
		return 0;
	if ((res = Prepare(r)) < 1)
		return res;
	dec = r->decoder;
	
	return dec->read_float32(r, buffer, nSamplePoints);
}

int 
AIFF_Seek(AIFF_Ref r, uint64_t framePos)
{
	int res = 0;
	struct decoder *dec;

	if (!r || !(r->flags & F_RDONLY))
		return -1;
	if (r->flags & F_NOTSEEKABLE)
		return -1;
	Unprepare(r);
	if ((res = Prepare(r)) < 1)
		return res;
	dec = r->decoder;

	return dec->seek(r, framePos);
}

int 
AIFF_ReadSamples32Bit(AIFF_Ref r, int32_t * samples, int n)
{
	int i, j;
	unsigned int h, len;

	if (!r || !(r->flags & F_RDONLY))
		return -1;
	if ((n % r->nChannels) != 0)
		return 0;
	if (n < 1 || r->segmentSize == 0)
		return -1;
	
	len = n * r->segmentSize;

	if (r->segmentSize == 4) {
		h = AIFF_ReadSamples(r, samples, len);

		return (h >> 2);
	}

	if (r->buflen < len) {
		if (r->buffer)
			free(r->buffer);
		r->buffer = malloc(len);
		if (!(r->buffer)) {
			return -1;
		}
		r->buflen = len;
	}

	h = AIFF_ReadSamples(r, r->buffer, len);
	if (h < (unsigned)r->segmentSize)
		return 0;
	if ((h % r->segmentSize) != 0)
		return -1;
	n = h / r->segmentSize;

	switch (r->segmentSize) {
	case 3:
	{
		/* XXX -- this is gross. */
		uint8_t *inbytes = (uint8_t *) r->buffer;
		uint8_t *outbytes = (uint8_t *) samples;
		j = 0;

		for (i = 0; i < n; ++i) {
#ifdef WORDS_BIGENDIAN
			outbytes[(i << 2) + 0] = inbytes[j + 0];
			outbytes[(i << 2) + 1] = inbytes[j + 1];
			outbytes[(i << 2) + 2] = inbytes[j + 2];
			outbytes[(i << 2) + 3] = 0;
#else
			outbytes[(i << 2) + 0] = 0;
			outbytes[(i << 2) + 1] = inbytes[j + 0];
			outbytes[(i << 2) + 2] = inbytes[j + 1];
			outbytes[(i << 2) + 3] = inbytes[j + 2];
#endif
			
			j += 3;
		}

		break;
	}

	case 2:
	{
		int16_t *words = (int16_t *) r->buffer;
		
		for (i = 0; i < n; ++i) {
			samples[i] = (int32_t) (words[i]) << 16;
		}
		break;
	}

	case 1:
	{
		int8_t *sbytes = (int8_t *) r->buffer;
		
		for (i = 0; i < n; ++i) {
			samples[i] = (int32_t) (sbytes[i]) << 24;
		}
		break;
	}
	}

	return n;
}


static void 
AIFF_ReadClose(AIFF_Ref r)
{
	if (r->buffer)
		free(r->buffer);
	if (r->buffer2)
		free(r->buffer2);
	Unprepare(r);
	fclose(r->fd);
	free(r);
	return;
}

static AIFF_Ref 
AIFF_WriteOpen(const char *file, int flags)
{
	AIFF_Ref w;
	IFFHeader hdr;
	ASSERT(sizeof(IFFHeader) == 12);
	
	w = malloc(kAIFFRefSize);
	if (!w) {
err0:
		return NULL;
	}

	w->fd = fopen(file, "wb");
	if (w->fd == NULL) {
err1:
		free(w);
		goto err0;
	}
	hdr.hid = ARRANGE_BE32(AIFF_FORM);
	w->len = 4;
	hdr.len = ARRANGE_BE32(4);
	if (flags & F_AIFC)
		hdr.fid = ARRANGE_BE32(AIFF_AIFC);
	else
		hdr.fid = ARRANGE_BE32(AIFF_AIFF);

	if (fwrite(&hdr, 1, 12, w->fd) < 12) {
err2:
		fclose(w->fd);
		goto err1;
	}
	w->stat = 0;
	w->segmentSize = 0;
	w->buffer = NULL;
	w->buflen = 0;
	w->tics = 0;

	/*
	 * If writing AIFF-C, write the required FVER chunk
	 */
	if (flags & F_AIFC) {
		IFFChunk chk;
		uint32_t vers;
		ASSERT(sizeof(IFFChunk) == 8);

		chk.id = ARRANGE_BE32(AIFF_FVER);
		chk.len = ARRANGE_BE32(4);
		vers = ARRANGE_BE32(AIFC_STD_DRAFT_082691);

		if (fwrite(&chk, 1, 8, w->fd) < 8 || 
		    fwrite(&vers, 1, 4, w->fd) < 4) {
			goto err2;
		}

		w->len += 12;

		/*
		 * If no endianness specified for AIFF-C,
		 * default to big endian
		 */
		if (!(flags & (LPCM_LTE_ENDIAN | LPCM_BIG_ENDIAN))) {
			flags |= LPCM_BIG_ENDIAN;
		}
	} else {
		/*
		 * If writing regular AIFF, make sure we
		 * write big-endian data
		 */
		flags &= ~LPCM_LTE_ENDIAN;
		flags |= LPCM_BIG_ENDIAN;
	}

	w->flags = F_WRONLY | flags;

	return w;
}

int 
AIFF_SetAttribute(AIFF_Ref w, IFFType attr, char *value)
{
	if (!w || !(w->flags & F_WRONLY))
		return -1;
	return set_iff_attribute(w, attr, value);
}

int
AIFF_CloneAttributes(AIFF_Ref w, AIFF_Ref r, int cloneMarkers)
{
	int rval, ret;
	int doneReadingMarkers;

	if (!w || !(w->flags & F_WRONLY))
		return -1;
	
	/*
	 * first of all, clone the IFF attributes
	 */
	rval = clone_iff_attributes(w, r);
	
	doneReadingMarkers = !cloneMarkers;
	if (!doneReadingMarkers) {
		int mMarkerId;
		uint64_t mMarkerPos;
		char *mMarkerName;
		
		if ((ret = AIFF_StartWritingMarkers(w)) < 1)
			return ret;
		
		do {
			if (AIFF_ReadMarker(r, &mMarkerId, &mMarkerPos, &mMarkerName) < 1)
				doneReadingMarkers = 1;
			else {
				ret = AIFF_WriteMarker(w, mMarkerPos, mMarkerName);
				rval = (rval > 0 ? ret : rval); /* preserve previous errors */
			}
		} while (!doneReadingMarkers);
		
		if ((ret = AIFF_EndWritingMarkers(w)) < 1)
			return ret;
	}
	
	return rval;
}

int 
AIFF_SetAudioFormat(AIFF_Ref w, int channels, double sRate, int bitsPerSample)
{
	uint8_t buffer[10];
	CommonChunk c;
	IFFChunk chk;
	IFFType enc;
	uint32_t ckLen = 18;
	char* encName = NULL;
	ASSERT(sizeof(IFFChunk) == 8);

	if (!w || !(w->flags & F_WRONLY))
		return -1;
	if (w->stat != 0)
		return 0;

	if (w->flags & F_AIFC) {
		/*
		 * On AIFF-C we need to write
		 * the encoding plus a description string
		 * in PASCAL-style format
		 */
		ckLen += 4;
		if (w->flags & LPCM_LTE_ENDIAN)
			enc = AUDIO_FORMAT_sowt;
		else
			enc = AUDIO_FORMAT_LPCM;

		encName = get_aifx_enc_name(enc);
		ckLen += PASCALOutGetLength(encName);
	}
	
	chk.id = ARRANGE_BE32(AIFF_COMM);
	chk.len = ARRANGE_BE32(ckLen);

	if (fwrite(&chk, 1, 8, w->fd) < 8) {
		return -1;
	}
	/* Fill in the chunk */
	c.numChannels = (uint16_t) channels;
	c.numChannels = ARRANGE_BE16(c.numChannels);
	c.numSampleFrames = 0;
	c.sampleSize = (uint16_t) bitsPerSample;
	c.sampleSize = ARRANGE_BE16(c.sampleSize);
	ieee754_write_extended(sRate, buffer);

	/*
	 * Write out the data.
	 * Write each independent field separately,
	 * since sizeof(CommonChunk) % 2 != 0, 
	 * so aligning may occur and insert some
	 * zeros between our fields!
	 */
	if (fwrite(&(c.numChannels), 1, 2, w->fd) < 2
	    || fwrite(&(c.numSampleFrames), 1, 4, w->fd) < 4
	    || fwrite(&(c.sampleSize), 1, 2, w->fd) < 2
	    || fwrite(buffer, 1, 10, w->fd) < 10) {
		return -1;
	}

	/*
	 * On AIFF-C, write the encoding + encstring
	 * (encstring is a PASCAL string)
	 */
	if (w->flags & F_AIFC) {
		if (fwrite(&enc, 1, 4, w->fd) != 4)
			return -1;
		if (PASCALOutWrite(w->fd, encName) < 2)
			return -1;
	}
		    
	/*
	 * We need to return here later
	 * (to update the 'numSampleFrames'),
	 * so store the current writing position
	 *
	 * ( note that w->len is total file length - 8 ,
	 * so we need to add 8 to get a valid offset ).
	 */
	w->len += 8;
	w->commonOffSet = w->len;
	w->len += ckLen;
	w->segmentSize = (bitsPerSample + 7) >> 3;
	w->nChannels = channels;
	w->stat = 1;

	return 1;
}

int 
AIFF_StartWritingSamples(AIFF_Ref w)
{
	IFFChunk chk;
	SoundChunk s;
	ASSERT(sizeof(SoundChunk) == 8);

	if (!w || !(w->flags & F_WRONLY))
		return -1;
	if (w->stat != 1)
		return 0;

	chk.id = ARRANGE_BE32(AIFF_SSND);
	chk.len = ARRANGE_BE32(8);

	if (fwrite(&chk, 1, 8, w->fd) < 8) {
		return -1;
	}
	/*
	 * We don`t use these values
	 */
	s.offset = 0;
	s.blockSize = 0;

	if (fwrite(&s, 1, 8, w->fd) < 8) {
		return -1;
	}
	w->len += 8;
	w->soundOffSet = w->len;
	w->len += 8;
	w->sampleBytes = 0;
	w->stat = 2;

	return 1;
}

static void *
InitBuffer(AIFF_Ref w, size_t len)
{
	if (w->buflen < len) {
modsize:
		w->tics = 0;

		if (w->buffer)
			free(w->buffer);
		w->buffer = malloc(len);

		if (!(w->buffer)) {
			w->buflen = 0;
			return NULL;
		}
		w->buflen = len;

	} else if (w->buflen > len) {
		if (++(w->tics) == 3)
			goto modsize;
	}
	return (w->buffer);
}

static void 
DestroyBuffer(AIFF_Ref w)
{
	if (w->buffer)
		free(w->buffer);

	w->buffer = 0;
	w->buflen = 0;
	w->tics = 0;

	return;
}

static int 
DoWriteSamples(AIFF_Ref w, void *samples, size_t len, int readOnlyBuf)
{
	int n;
	uint32_t sampleBytes;
	void *buffer;
	
	if (!w || !(w->flags & F_WRONLY))
		return -1;
	if (w->stat != 2)
		return 0;

	n = (int) len;
	if (n % (w->segmentSize) != 0)
		return 0;
	n /= w->segmentSize;

	if (readOnlyBuf) {
		buffer = InitBuffer(w, len);
		if (buffer == NULL)
			return -1;
	} else {
		buffer = samples;
	}

	lpcm_swap_samples(w->segmentSize, w->flags, samples, buffer, n);

	if (fwrite(buffer, 1, len, w->fd) < len) {
		return -1;
	}
	sampleBytes = (uint32_t) len;
	w->sampleBytes += sampleBytes;
	w->len += sampleBytes;

	return 1;
}

int
AIFF_WriteSamples(AIFF_Ref w, void *samples, size_t len)
{
	return DoWriteSamples(w, samples, len, 1);
}


int 
AIFF_WriteSamples32Bit(AIFF_Ref w, int32_t * samples, int n)
{
	int i, j;
	unsigned int len;
	void *buffer;

	if (!w || !(w->flags & F_WRONLY))
		return -1;
	if (w->stat != 2 || w->segmentSize == 0 || n < 1)
		return -1;
	len = n * w->segmentSize;

	if (w->segmentSize == 4)
		return DoWriteSamples(w, samples, len, 1) >> 2;
		
	buffer = InitBuffer(w, len);
	if (!buffer)
		return -1;

	switch (w->segmentSize) {
	case 3:
	{
		/* XXX -- this is gross. */
		uint8_t *inbytes = (uint8_t *) samples;
		uint8_t *outbytes = (uint8_t *) buffer;
		
		for (i = 0, j = 0; i < n; ++i, j += 3) {
#ifdef WORDS_BIGENDIAN
			outbytes[j + 0] = inbytes[(i << 2) + 0];
			outbytes[j + 1] = inbytes[(i << 2) + 1];
			outbytes[j + 2] = inbytes[(i << 2) + 2];
#else
			outbytes[j + 0] = inbytes[(i << 2) + 1];
			outbytes[j + 1] = inbytes[(i << 2) + 2];
			outbytes[j + 2] = inbytes[(i << 2) + 3];
#endif
		}
		break;
	}

	case 2:
	{
		int16_t *words = (int16_t *) buffer;

		for (i = 0; i < n; ++i) {
			words[i] = samples[i] >> 16;
		}
		break;
	}

	case 1:
	{
		int8_t *sbytes = (int8_t *) buffer;
		
		for (i = 0; i < n; ++i) {
			sbytes[i] = samples[i] >> 24;
		}
		break;
	}
	}

	return DoWriteSamples(w, buffer, len, 0);
}

int 
AIFF_EndWritingSamples(AIFF_Ref w)
{
	IFFChunk chk;
	long of;
	uint32_t curpos, numSampleFrames;

	if (!w || !(w->flags & F_WRONLY))
		return -1;
	if (w->stat != 2)
		return 0;
	
	DestroyBuffer(w);
	if (w->sampleBytes & 1) {
		fputc(0, w->fd);
		w->sampleBytes++;
		w->len++;
	}
	/*
	 * XXX: AIFF files are 32-bit
	 */
	curpos = (uint32_t) (w->len) + 8;

	of = w->soundOffSet;
	
	chk.id = ARRANGE_BE32(AIFF_SSND);
	chk.len = w->sampleBytes + 8;
	chk.len = ARRANGE_BE32(chk.len);

	if (fseek(w->fd, of, SEEK_SET) < 0) {
		return -1;
	}
	if (fwrite(&chk, 1, 8, w->fd) < 8) {
		return -1;
	}

	numSampleFrames = (w->sampleBytes / w->nChannels) / w->segmentSize;
	numSampleFrames = ARRANGE_BE32(numSampleFrames);

	/* Write out */
	of = w->commonOffSet + 10;
	if (fseek(w->fd, of, SEEK_SET) < 0) {
		return -1;
	}
	if (fwrite(&numSampleFrames, 1, 4, w->fd) < 4) {
		return -1;
	}
	/* Return back to current position in the file. */
	of = curpos;
	if (fseek(w->fd, of, SEEK_SET) < 0) {
		return -1;
	}
	w->stat = 3;

	return 1;
}

int 
AIFF_StartWritingMarkers(AIFF_Ref w)
{
	IFFChunk chk;
	uint16_t nMarkers = 0;

	if (!w || !(w->flags & F_WRONLY))
		return -1;
	if (w->stat != 3)
		return -1;

	chk.id = ARRANGE_BE32(AIFF_MARK);
	chk.len = ARRANGE_BE16(2);

	if (fwrite(&chk, 1, 8, w->fd) < 8)
		return -1;
	w->len += 8;
	w->markerOffSet = w->len;
	if (fwrite(&nMarkers, 1, 2, w->fd) < 2)
		return -1;
	w->len += 2;

	w->markerPos = 0;
	w->stat = 4;

	return 1;
}

int 
AIFF_WriteMarker(AIFF_Ref w, uint64_t position, char *name)
{
	Marker m;

	if (!w || !(w->flags & F_WRONLY))
		return -1;
	if (w->stat != 4)
		return -1;

	/* max. number of markers --> 0xFFFF */
	if (w->markerPos == 0xFFFF)
		return 0;

	m.id = (MarkerId) (w->markerPos + 1);
	m.id = ARRANGE_BE16(m.id);
	m.position = (uint32_t) position; /* XXX: AIFF is a 32-bit format */
	m.position = ARRANGE_BE32(m.position);

	if (fwrite(&(m.id), 1, 2, w->fd) < 2 || 
	    fwrite(&(m.position), 1, 4, w->fd) < 4)
		return -1;
	w->len += 6;

	if (name) {
		int l;

		if ((l = PASCALOutWrite(w->fd, name)) < 2)
			return -1;
		w->len += l;
	} else {
		if (fwrite("\0\0", 1, 2, w->fd) != 2)
			return -1;
		w->len += 2;
	}

	++(w->markerPos);
	return 1;
}

int 
AIFF_EndWritingMarkers(AIFF_Ref w)
{
	uint32_t cklen, curpos;
	long offSet;
	uint16_t nMarkers;

	if (!w || !(w->flags & F_WRONLY))
		return -1;
	if (w->stat != 4)
		return -1;

	curpos = w->len + 8;
	cklen = w->len - w->markerOffSet;
	cklen = ARRANGE_BE32(cklen);

	offSet = w->markerOffSet;
	
	/*
	 * Correct the chunk length
	 * and the nMarkers field
	 */
	nMarkers = w->markerPos;
	nMarkers = ARRANGE_BE16(nMarkers);

	if (fseek(w->fd, offSet + 4, SEEK_SET) < 0) {
		return -1;
	}
	if (fwrite(&cklen, 1, 4, w->fd) < 4
	    || fwrite(&nMarkers, 1, 2, w->fd) < 2) {
		return -1;
	}
	/* Return back to current writing position */
	offSet = (long) curpos;
	if (fseek(w->fd, offSet, SEEK_SET) < 0) {
		return -1;
	}
	w->stat = 3;
	return 1;
}

static int 
AIFF_WriteClose(AIFF_Ref w)
{
	int ret = 1;
	IFFHeader hdr;

	if (w->stat != 3)
		ret = 2;
	
	hdr.hid = ARRANGE_BE32(AIFF_FORM);
	hdr.len = w->len;
	hdr.len = ARRANGE_BE32(hdr.len);
	if (w->flags & F_AIFC)
		hdr.fid = ARRANGE_BE32(AIFF_AIFC);
	else
		hdr.fid = ARRANGE_BE32(AIFF_AIFF);

	if (fseek(w->fd, 0, SEEK_SET) < 0) {
		fclose(w->fd);
		free(w);
		return -1;
	}
	if (fwrite(&hdr, 1, 12, w->fd) < 12) {
		fclose(w->fd);
		free(w);
		return -1;
	}
	/* Now fclose, free & return */
	fclose(w->fd);
	free(w);
	return ret;
}


/* assertion failed */
void
AIFFAssertionFailed (const char * fil, int lin)
{
	
	fprintf(stderr, "%s: assertion at %s:%d failed\n",
			PACKAGE_STRING,
			(char *) fil,
			lin
			);
	fprintf(stderr, "%s: please report this bug at <%s>\n",
			PACKAGE_STRING,
			PACKAGE_BUGREPORT
			);
	
#ifdef HAVE_ABORT
	abort();
#else
	/* XXX */
#endif
}

