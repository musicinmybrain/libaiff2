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

#include "defs.h"

/* == Public functions == */

AIFF_ReadRef 
AIFF_Open(const char *file)
{
	AIFF_ReadRef r;
	IFFHeader hdr;

	r = malloc(kAiffReadRefSize);
	if (!r) {
		return NULL;
	}
	r->fd = fopen(file, "rb");
	if (r->fd == NULL) {
		free(r);
		return NULL;
	}
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

		/*
		 * Get basic sound format
		 */
		if (get_aifx_format(r, NULL, &(r->nChannels), NULL,
			NULL, &(r->segmentSize),
			&(r->audioFormat),
			&(r->flags)) < 1) {
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
AIFF_GetAttribute(AIFF_ReadRef r, IFFType attrib)
{
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
AIFF_ReadMarker(AIFF_ReadRef r, int *id, uint32_t * pos, char **name)
{
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
AIFF_GetInstrumentData(AIFF_ReadRef r, Instrument * i)
{
	switch (r->format) {
	case AIFF_TYPE_AIFF:
	case AIFF_TYPE_AIFC:
		return get_aifx_instrument(r, i);
	default:
		return 0;
	}
	return 0;
}

int 
AIFF_GetSoundFormat(AIFF_ReadRef r, uint32_t * nSamples, int *channels,
    int *samplingRate, int *bitsPerSample, int *segmentSize)
{
	switch (r->format) {
	case AIFF_TYPE_AIFF:
	case AIFF_TYPE_AIFC:
		return get_aifx_format(r, nSamples, channels,
		    samplingRate, bitsPerSample,
		    segmentSize, NULL, NULL);
	default:
		return 0;
	}
	return 0;
}

size_t 
AIFF_ReadSamples(AIFF_ReadRef r, void *buffer, size_t len)
{
	int res = 1;

	if (r->stat == 0) {
		switch (r->format) {
		case AIFF_TYPE_AIFF:
		case AIFF_TYPE_AIFC:
			res = do_aifx_prepare(r);
			break;
		default:
			return 0;
		}
	}
	if (res < 1)
		return -1;

	switch (r->audioFormat) {
	case AUDIO_FORMAT_lpcm:
	case AUDIO_FORMAT_LPCM:
		return do_lpcm(r, buffer, len);
	default:
		return 0;
	}

	return 0;
}

int 
AIFF_Seek(AIFF_ReadRef r, uint32_t pos)
{
	int res = 0;

	r->stat = 0;
	switch (r->format) {
	case AIFF_TYPE_AIFF:
	case AIFF_TYPE_AIFC:
		res = do_aifx_prepare(r);
		break;
	}

	if (res < 1)
		return -1;

	switch (r->audioFormat) {
	case AUDIO_FORMAT_lpcm:
	case AUDIO_FORMAT_LPCM:
		return lpcm_seek(r, pos);
	default:
		return 0;
	}

	return 0;
}

int 
AIFF_ReadSamples32Bit(AIFF_ReadRef r, int32_t * samples, int nsamples)
{
	void *buffer;
	int i, j;
	size_t h;
	int n = nsamples;
	size_t len;
	int segmentSize;
	int32_t *dwords;
	int16_t *words;
	int8_t *sbytes;
	uint8_t *inbytes;
	uint8_t *outbytes;
	uint8_t x, y, z;

	if (!r || n < 1 || r->segmentSize == 0) {
		if (r->buffer) {
			free(r->buffer);
			r->buffer = NULL;
			r->buflen = 0;
		}
		return -1;
	}
	segmentSize = r->segmentSize;

	len = (size_t) n;
	len *= segmentSize;

	if ((r->buflen) < len) {
		if (r->buffer)
			free(r->buffer);
		r->buffer = malloc(len);
		if (!(r->buffer)) {
			return -1;
		}
		r->buflen = len;
	}
	buffer = r->buffer;

	h = AIFF_ReadSamples(r, buffer, len);
	if (h < (size_t) segmentSize) {
		free(r->buffer);
		r->buffer = NULL;
		r->buflen = 0;
		return 0;
	}
	n = (int) h;
	if (n % segmentSize != 0) {
		free(r->buffer);
		r->buffer = NULL;
		r->buflen = 0;
		return -1;
	}
	n /= segmentSize;

	switch (segmentSize) {
	case 4:
		dwords = (int32_t *) buffer;
		for (i = 0; i < n; ++i)
			samples[i] = dwords[i];
		break;
	case 3:
		inbytes = (uint8_t *) buffer;
		outbytes = (uint8_t *) samples;
		n <<= 2;	/* n *= 4 */
		j = 0;

		for (i = 0; i < n; i += 4) {
			x = inbytes[j++];
			y = inbytes[j++];
			z = inbytes[j++];
#ifdef WORDS_BIGENDIAN
			outbytes[i] = x;
			outbytes[i + 1] = y;
			outbytes[i + 2] = z;
			outbytes[i + 3] = 0;
#else
			outbytes[i] = 0;
			outbytes[i + 1] = x;
			outbytes[i + 2] = y;
			outbytes[i + 3] = z;
#endif
		}

		n >>= 2;
		break;
	case 2:
		words = (int16_t *) buffer;
		for (i = 0; i < n; ++i) {
			samples[i] = (int32_t) (words[i]);
			samples[i] <<= 16;
		}
		break;
	case 1:
		sbytes = (int8_t *) buffer;
		for (i = 0; i < n; ++i) {
			samples[i] = (int32_t) (sbytes[i]);
			samples[i] <<= 24;
		}
		break;
	}

	return n;
}


void 
AIFF_Close(AIFF_ReadRef r)
{
	if (r->buffer)
		free(r->buffer);
	fclose(r->fd);
	free(r);
	return;
}

AIFF_WriteRef 
AIFF_WriteOpen(const char *file)
{
	AIFF_WriteRef w;
	IFFHeader hdr;

	w = malloc(kAiffWriteRefSize);
	if (!w) {
		return NULL;
	}
	w->fd = fopen(file, "w+b");
	if (w->fd == NULL) {
		return NULL;
	}
	memcpy(&(hdr.hid), FormID, 4);
	w->len = 4;
	hdr.len = ARRANGE_BE32(w->len);
	memcpy(&(hdr.fid), AiffID, 4);

	if (fwrite(&hdr, 1, 12, w->fd) < 12) {
		fclose(w->fd);
		return NULL;
	}
	w->stat = 0;
	w->segmentSize = 0;
	w->buffer = NULL;
	w->buflen = 0;
	w->tics = 0;

	return w;
}

int 
AIFF_SetAttribute(AIFF_WriteRef w, IFFType attr, char *value)
{
	return set_iff_attribute(w, attr, value);
}

int 
AIFF_SetSoundFormat(AIFF_WriteRef w, int channels, int samplingRate,
    int bitsPerSample)
{
	double sRate;
	unsigned char buffer[10];
	CommonChunk c;
	IFFChunk chk;

	if (w->stat != 0)
		return 0;

	memcpy(&(chk.id), CommonID, 4);
	chk.len = ARRANGE_BE32(18);

	if (fwrite(&chk, 1, 8, w->fd) < 8) {
		return -1;
	}
	/* Fill in the chunk */
	c.numChannels = (uint16_t) channels;
	c.numChannels = ARRANGE_BE16(c.numChannels);
	c.numSampleFrames = 0;
	c.sampleSize = (uint16_t) bitsPerSample;
	c.sampleSize = ARRANGE_BE16(c.sampleSize);
	sRate = (double) samplingRate;
	ieee754_write_extended(sRate, buffer);

	/* Write out the data */
	if (fwrite(&(c.numChannels), 1, 2, w->fd) < 2
	    || fwrite(&(c.numSampleFrames), 1, 4, w->fd) < 4
	    || fwrite(&(c.sampleSize), 1, 2, w->fd) < 2
	    || fwrite(buffer, 1, 10, w->fd) < 10) {
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
	w->len += 18;
	w->segmentSize = bitsPerSample >> 3;
	if (bitsPerSample & 0x7)
		++(w->segmentSize);
	w->stat = 1;

	return 1;
}

int 
AIFF_StartWritingSamples(AIFF_WriteRef w)
{
	IFFChunk chk;
	SoundChunk s;

	if (w->stat != 1)
		return 0;

	memcpy(&(chk.id), SoundID, 4);
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
InitBuffer(AIFF_WriteRef w, size_t len)
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
DestroyBuffer(AIFF_WriteRef w)
{
	if (w->buffer)
		free(w->buffer);

	w->buffer = 0;
	w->buflen = 0;
	w->tics = 0;

	return;
}


static int AIFF_WriteSamplesNoSwap(AIFF_WriteRef, void *, size_t);

int 
AIFF_WriteSamples(AIFF_WriteRef w, void *samples, size_t len)
{
	int n;
	void *buffer;

	if (w->stat != 2)
		return 0;

	buffer = InitBuffer(w, len);
	if (!buffer)
		return -1;

	n = (int) len;
	if (n % (w->segmentSize) != 0)
		return 0;
	n /= w->segmentSize;

	lpcm_swap_samples(w->segmentSize, LPCM_BIG_ENDIAN, samples, buffer, n);

	return AIFF_WriteSamplesNoSwap(w, buffer, len);
}

static int 
AIFF_WriteSamplesNoSwap(AIFF_WriteRef w, void *samples, size_t len)
{
	uint32_t sampleBytes;

	if (w->stat != 2)
		return 0;

	if (fwrite(samples, 1, len, w->fd) < len) {
		return -1;
	}
	sampleBytes = (uint32_t) len;
	w->sampleBytes += sampleBytes;
	w->len += sampleBytes;

	return 1;
}

int 
AIFF_WriteSamples32Bit(AIFF_WriteRef w, int32_t * samples, int nsamples)
{
	register int i, j;
	register int n = nsamples;
	void *buffer;
	int res;
	int segmentSize;
	size_t len;
	int32_t cursample;
	int32_t *dwords;
	int16_t *words;
	int8_t *sbytes;
	uint8_t *inbytes;
	uint8_t *outbytes;
	uint8_t x, y, z;

	if (!w || w->stat != 2 || w->segmentSize == 0 || n < 1) {
		DestroyBuffer(w);
		return -1;
	}
	segmentSize = w->segmentSize;

	len = (size_t) n;
	len *= segmentSize;

	buffer = InitBuffer(w, len);
	if (!buffer)
		return -1;

	switch (segmentSize) {
	case 4:
		dwords = (int32_t *) buffer;
		for (i = 0; i < n; ++i)
			dwords[i] = ARRANGE_BE32(samples[i]);
		break;
	case 3:
		inbytes = (uint8_t *) samples;
		outbytes = (uint8_t *) buffer;
		j = 0;
		n *= 3;
		for (i = 0; i < n; i += 3) {
#ifdef WORDS_BIGENDIAN
			x = inbytes[j++];
			y = inbytes[j++];
			z = inbytes[j++];
			j++;
#else
			j++;
			z = inbytes[j++];
			y = inbytes[j++];
			x = inbytes[j++];
#endif
			outbytes[i] = x;
			outbytes[i + 1] = y;
			outbytes[i + 2] = z;
		}
		n /= 3;
		break;
	case 2:
		words = (int16_t *) buffer;
		for (i = 0; i < n; ++i) {
			cursample = samples[i];
			cursample >>= 16;
			words[i] = (int16_t) cursample;
			words[i] = ARRANGE_BE16(words[i]);
		}
		break;
	case 1:
		sbytes = (int8_t *) buffer;
		for (i = 0; i < n; ++i) {
			cursample = samples[i];
			cursample >>= 24;
			sbytes[i] = (int8_t) cursample;
		}
		break;
	}

	res = AIFF_WriteSamplesNoSwap(w, buffer, len);

	if (res < 1) {
		DestroyBuffer(w);
		return -1;
	}
	return 1;
}

int 
AIFF_EndWritingSamples(AIFF_WriteRef w)
{
	char car = 0;
	uint32_t segment;
	long of;
	IFFType typ;
	IFFChunk chk;
	CommonChunk c;
	uint32_t len;
	uint32_t curpos;

	DestroyBuffer(w);

	if (w->stat != 2)
		return 0;

	if (w->sampleBytes & 0x1) {
		if (fwrite(&car, 1, 1, w->fd) < 1) {
			return -1;
		}
		++(w->sampleBytes);
		++(w->len);
	}
	curpos = w->len + 8;

	of = (long) (w->soundOffSet);
	if (fseek(w->fd, of, SEEK_SET) < 0) {
		return -1;
	}
	if (fread(&chk, 1, 8, w->fd) < 8) {
		return -1;
	}
	memcpy(&typ, SoundID, 4);
	if (chk.id != typ) {
		return -1;
	}
	len = ARRANGE_BE32(chk.len);
	len += w->sampleBytes;
	chk.len = ARRANGE_BE32(len);

	if (fseek(w->fd, of, SEEK_SET) < 0) {
		return -1;
	}
	if (fwrite(&chk, 1, 8, w->fd) < 8) {
		return -1;
	}
	of = (long) (w->commonOffSet);
	if (fseek(w->fd, of, SEEK_SET) < 0) {
		return -1;
	}
	if (fread(&chk, 1, 8, w->fd) < 8) {
		return -1;
	}
	memcpy(&typ, CommonID, 4);
	if (chk.id != typ) {
		return -1;
	}
	if (fread(&(c.numChannels), 1, 2, w->fd) < 2
	    || fread(&(c.numSampleFrames), 1, 4, w->fd) < 4
	    || fread(&(c.sampleSize), 1, 2, w->fd) < 2) {
		return -1;
	}
	/* Correct the data of the chunk */
	c.numChannels = ARRANGE_BE16(c.numChannels);
	c.sampleSize = ARRANGE_BE16(c.sampleSize);
	segment = w->segmentSize;

	c.numSampleFrames = w->sampleBytes / c.numChannels;
	c.numSampleFrames /= segment;
	c.numChannels = ARRANGE_BE16(c.numChannels);
	c.numSampleFrames = ARRANGE_BE32(c.numSampleFrames);
	c.sampleSize = ARRANGE_BE16(c.sampleSize);

	/* Write out */
	of += 10;
	if (fseek(w->fd, of, SEEK_SET) < 0) {
		return -1;
	}
	if (fwrite(&(c.numSampleFrames), 1, 4, w->fd) < 4) {
		return -1;
	}
	/* Return back to current position in the file. */
	of = (long) curpos;
	if (fseek(w->fd, of, SEEK_SET) < 0) {
		return -1;
	}
	w->stat = 3;

	return 1;
}

int 
AIFF_StartWritingMarkers(AIFF_WriteRef w)
{
	IFFChunk chk;
	uint16_t nMarkers = 0;

	if (w->stat != 3)
		return -1;

	memcpy(&(chk.id), MarkerID, 4);
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
AIFF_WriteMarker(AIFF_WriteRef w, uint32_t position, char *name)
{
	Marker m;
	int l = 0;
	int pad = 0;

	if (w->stat != 4)
		return -1;

	/* max. number of markers --> 0xFFFF */
	if (w->markerPos == 0xFFFF)
		return 0;

	m.id = (MarkerId) (w->markerPos + 1);
	m.id = ARRANGE_BE16(m.id);
	m.position = ARRANGE_BE32(position);
	m.markerNameLen = 0;
	m.markerName = '\0';

	if (name) {
		l = strlen(name);
		if (l < 255) {
			/* Length must be even */
			if (l & 0x1)
				pad = 1;
			m.markerNameLen = (uint8_t) l;
			m.markerName = name[0];
		} else
			l = 0;
	}
	if (fwrite(&(m.id), 1, 2, w->fd) < 2
	    || fwrite(&(m.position), 1, 4, w->fd) < 4
	    || fwrite(&(m.markerNameLen), 1, 1, w->fd) < 1
	    || fwrite(&(m.markerName), 1, 1, w->fd) < 1) {
		return -1;
	}
	w->len += 8;
	if (l && !pad)
		--l;
	if (l > 0) {
		if (fwrite(name + 1, 1, l, w->fd) < (size_t) l) {
			return -1;
		}
		w->len += (uint32_t) l;
	}
	++(w->markerPos);
	return 1;
}

int 
AIFF_EndWritingMarkers(AIFF_WriteRef w)
{
	IFFType typ;
	IFFType ckid;
	uint32_t cklen;
	uint32_t curpos;
	long offSet;
	uint16_t nMarkers;

	if (w->stat != 4)
		return -1;

	curpos = w->len + 8;
	cklen = w->len;
	cklen -= w->markerOffSet;
	cklen = ARRANGE_BE32(cklen);

	offSet = (long) (w->markerOffSet);
	if (fseek(w->fd, offSet, SEEK_SET) < 0) {
		return -1;
	}
	if (fread(&ckid, 1, 4, w->fd) < 4)
		return -1;

	memcpy(&typ, MarkerID, 4);
	if (ckid != typ) {
		return -1;
	}
	nMarkers = (uint16_t) (w->markerPos);
	nMarkers = ARRANGE_BE16(nMarkers);

	/*
	 * Correct the chunk length
	 * and the 'nMarkers' field
	 */
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

int 
AIFF_WriteClose(AIFF_WriteRef w)
{
	int ret = 1;
	IFFType typ;
	IFFHeader hdr;

	if (w->stat != 3) {
		ret = 2;
	}
	if (fseek(w->fd, 0, SEEK_SET) < 0) {
		fclose(w->fd);
		free(w);
		return -1;
	}
	if (fread(&hdr, 1, 12, w->fd) < 12) {
		fclose(w->fd);
		free(w);
		return -1;
	}
	memcpy(&typ, FormID, 4);
	if (hdr.hid != typ) {
		fclose(w->fd);
		free(w);
		return -1;
	}
	/* Fix the 'length' field */
	hdr.len = ARRANGE_BE32(w->len);

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
