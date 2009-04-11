/*	$OpenBSD: legacy.c,v 1.5 2009/04/11 10:24:21 jakemsr Exp $	*/
/*
 * Copyright (c) 1997 Kenneth Stailey.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Kenneth Stailey.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sndio.h>

#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include "wav.h"

/*
 * This table converts a (8 bit) mu-law value two a 16 bit value.
 * The 16 bits are represented as an array of two bytes for easier access
 * to the individual bytes.
 */
const u_char mulawtolin16[256][2] = {
	{0x02,0x84}, {0x06,0x84}, {0x0a,0x84}, {0x0e,0x84},
	{0x12,0x84}, {0x16,0x84}, {0x1a,0x84}, {0x1e,0x84},
	{0x22,0x84}, {0x26,0x84}, {0x2a,0x84}, {0x2e,0x84},
	{0x32,0x84}, {0x36,0x84}, {0x3a,0x84}, {0x3e,0x84},
	{0x41,0x84}, {0x43,0x84}, {0x45,0x84}, {0x47,0x84},
	{0x49,0x84}, {0x4b,0x84}, {0x4d,0x84}, {0x4f,0x84},
	{0x51,0x84}, {0x53,0x84}, {0x55,0x84}, {0x57,0x84},
	{0x59,0x84}, {0x5b,0x84}, {0x5d,0x84}, {0x5f,0x84},
	{0x61,0x04}, {0x62,0x04}, {0x63,0x04}, {0x64,0x04},
	{0x65,0x04}, {0x66,0x04}, {0x67,0x04}, {0x68,0x04},
	{0x69,0x04}, {0x6a,0x04}, {0x6b,0x04}, {0x6c,0x04},
	{0x6d,0x04}, {0x6e,0x04}, {0x6f,0x04}, {0x70,0x04},
	{0x70,0xc4}, {0x71,0x44}, {0x71,0xc4}, {0x72,0x44},
	{0x72,0xc4}, {0x73,0x44}, {0x73,0xc4}, {0x74,0x44},
	{0x74,0xc4}, {0x75,0x44}, {0x75,0xc4}, {0x76,0x44},
	{0x76,0xc4}, {0x77,0x44}, {0x77,0xc4}, {0x78,0x44},
	{0x78,0xa4}, {0x78,0xe4}, {0x79,0x24}, {0x79,0x64},
	{0x79,0xa4}, {0x79,0xe4}, {0x7a,0x24}, {0x7a,0x64},
	{0x7a,0xa4}, {0x7a,0xe4}, {0x7b,0x24}, {0x7b,0x64},
	{0x7b,0xa4}, {0x7b,0xe4}, {0x7c,0x24}, {0x7c,0x64},
	{0x7c,0x94}, {0x7c,0xb4}, {0x7c,0xd4}, {0x7c,0xf4},
	{0x7d,0x14}, {0x7d,0x34}, {0x7d,0x54}, {0x7d,0x74},
	{0x7d,0x94}, {0x7d,0xb4}, {0x7d,0xd4}, {0x7d,0xf4},
	{0x7e,0x14}, {0x7e,0x34}, {0x7e,0x54}, {0x7e,0x74},
	{0x7e,0x8c}, {0x7e,0x9c}, {0x7e,0xac}, {0x7e,0xbc},
	{0x7e,0xcc}, {0x7e,0xdc}, {0x7e,0xec}, {0x7e,0xfc},
	{0x7f,0x0c}, {0x7f,0x1c}, {0x7f,0x2c}, {0x7f,0x3c},
	{0x7f,0x4c}, {0x7f,0x5c}, {0x7f,0x6c}, {0x7f,0x7c},
	{0x7f,0x88}, {0x7f,0x90}, {0x7f,0x98}, {0x7f,0xa0},
	{0x7f,0xa8}, {0x7f,0xb0}, {0x7f,0xb8}, {0x7f,0xc0},
	{0x7f,0xc8}, {0x7f,0xd0}, {0x7f,0xd8}, {0x7f,0xe0},
	{0x7f,0xe8}, {0x7f,0xf0}, {0x7f,0xf8}, {0x80,0x00},
	{0xfd,0x7c}, {0xf9,0x7c}, {0xf5,0x7c}, {0xf1,0x7c},
	{0xed,0x7c}, {0xe9,0x7c}, {0xe5,0x7c}, {0xe1,0x7c},
	{0xdd,0x7c}, {0xd9,0x7c}, {0xd5,0x7c}, {0xd1,0x7c},
	{0xcd,0x7c}, {0xc9,0x7c}, {0xc5,0x7c}, {0xc1,0x7c},
	{0xbe,0x7c}, {0xbc,0x7c}, {0xba,0x7c}, {0xb8,0x7c},
	{0xb6,0x7c}, {0xb4,0x7c}, {0xb2,0x7c}, {0xb0,0x7c},
	{0xae,0x7c}, {0xac,0x7c}, {0xaa,0x7c}, {0xa8,0x7c},
	{0xa6,0x7c}, {0xa4,0x7c}, {0xa2,0x7c}, {0xa0,0x7c},
	{0x9e,0xfc}, {0x9d,0xfc}, {0x9c,0xfc}, {0x9b,0xfc},
	{0x9a,0xfc}, {0x99,0xfc}, {0x98,0xfc}, {0x97,0xfc},
	{0x96,0xfc}, {0x95,0xfc}, {0x94,0xfc}, {0x93,0xfc},
	{0x92,0xfc}, {0x91,0xfc}, {0x90,0xfc}, {0x8f,0xfc},
	{0x8f,0x3c}, {0x8e,0xbc}, {0x8e,0x3c}, {0x8d,0xbc},
	{0x8d,0x3c}, {0x8c,0xbc}, {0x8c,0x3c}, {0x8b,0xbc},
	{0x8b,0x3c}, {0x8a,0xbc}, {0x8a,0x3c}, {0x89,0xbc},
	{0x89,0x3c}, {0x88,0xbc}, {0x88,0x3c}, {0x87,0xbc},
	{0x87,0x5c}, {0x87,0x1c}, {0x86,0xdc}, {0x86,0x9c},
	{0x86,0x5c}, {0x86,0x1c}, {0x85,0xdc}, {0x85,0x9c},
	{0x85,0x5c}, {0x85,0x1c}, {0x84,0xdc}, {0x84,0x9c},
	{0x84,0x5c}, {0x84,0x1c}, {0x83,0xdc}, {0x83,0x9c},
	{0x83,0x6c}, {0x83,0x4c}, {0x83,0x2c}, {0x83,0x0c},
	{0x82,0xec}, {0x82,0xcc}, {0x82,0xac}, {0x82,0x8c},
	{0x82,0x6c}, {0x82,0x4c}, {0x82,0x2c}, {0x82,0x0c},
	{0x81,0xec}, {0x81,0xcc}, {0x81,0xac}, {0x81,0x8c},
	{0x81,0x74}, {0x81,0x64}, {0x81,0x54}, {0x81,0x44},
	{0x81,0x34}, {0x81,0x24}, {0x81,0x14}, {0x81,0x04},
	{0x80,0xf4}, {0x80,0xe4}, {0x80,0xd4}, {0x80,0xc4},
	{0x80,0xb4}, {0x80,0xa4}, {0x80,0x94}, {0x80,0x84},
	{0x80,0x78}, {0x80,0x70}, {0x80,0x68}, {0x80,0x60},
	{0x80,0x58}, {0x80,0x50}, {0x80,0x48}, {0x80,0x40},
	{0x80,0x38}, {0x80,0x30}, {0x80,0x28}, {0x80,0x20},
	{0x80,0x18}, {0x80,0x10}, {0x80,0x08}, {0x80,0x00},
};

const u_char alawtolin16[256][2] = {
	{0x6a,0x80}, {0x6b,0x80}, {0x68,0x80}, {0x69,0x80},
	{0x6e,0x80}, {0x6f,0x80}, {0x6c,0x80}, {0x6d,0x80},
	{0x62,0x80}, {0x63,0x80}, {0x60,0x80}, {0x61,0x80},
	{0x66,0x80}, {0x67,0x80}, {0x64,0x80}, {0x65,0x80},
	{0x75,0x40}, {0x75,0xc0}, {0x74,0x40}, {0x74,0xc0},
	{0x77,0x40}, {0x77,0xc0}, {0x76,0x40}, {0x76,0xc0},
	{0x71,0x40}, {0x71,0xc0}, {0x70,0x40}, {0x70,0xc0},
	{0x73,0x40}, {0x73,0xc0}, {0x72,0x40}, {0x72,0xc0},
	{0x2a,0x00}, {0x2e,0x00}, {0x22,0x00}, {0x26,0x00},
	{0x3a,0x00}, {0x3e,0x00}, {0x32,0x00}, {0x36,0x00},
	{0x0a,0x00}, {0x0e,0x00}, {0x02,0x00}, {0x06,0x00},
	{0x1a,0x00}, {0x1e,0x00}, {0x12,0x00}, {0x16,0x00},
	{0x55,0x00}, {0x57,0x00}, {0x51,0x00}, {0x53,0x00},
	{0x5d,0x00}, {0x5f,0x00}, {0x59,0x00}, {0x5b,0x00},
	{0x45,0x00}, {0x47,0x00}, {0x41,0x00}, {0x43,0x00},
	{0x4d,0x00}, {0x4f,0x00}, {0x49,0x00}, {0x4b,0x00},
	{0x7e,0xa8}, {0x7e,0xb8}, {0x7e,0x88}, {0x7e,0x98},
	{0x7e,0xe8}, {0x7e,0xf8}, {0x7e,0xc8}, {0x7e,0xd8},
	{0x7e,0x28}, {0x7e,0x38}, {0x7e,0x08}, {0x7e,0x18},
	{0x7e,0x68}, {0x7e,0x78}, {0x7e,0x48}, {0x7e,0x58},
	{0x7f,0xa8}, {0x7f,0xb8}, {0x7f,0x88}, {0x7f,0x98},
	{0x7f,0xe8}, {0x7f,0xf8}, {0x7f,0xc8}, {0x7f,0xd8},
	{0x7f,0x28}, {0x7f,0x38}, {0x7f,0x08}, {0x7f,0x18},
	{0x7f,0x68}, {0x7f,0x78}, {0x7f,0x48}, {0x7f,0x58},
	{0x7a,0xa0}, {0x7a,0xe0}, {0x7a,0x20}, {0x7a,0x60},
	{0x7b,0xa0}, {0x7b,0xe0}, {0x7b,0x20}, {0x7b,0x60},
	{0x78,0xa0}, {0x78,0xe0}, {0x78,0x20}, {0x78,0x60},
	{0x79,0xa0}, {0x79,0xe0}, {0x79,0x20}, {0x79,0x60},
	{0x7d,0x50}, {0x7d,0x70}, {0x7d,0x10}, {0x7d,0x30},
	{0x7d,0xd0}, {0x7d,0xf0}, {0x7d,0x90}, {0x7d,0xb0},
	{0x7c,0x50}, {0x7c,0x70}, {0x7c,0x10}, {0x7c,0x30},
	{0x7c,0xd0}, {0x7c,0xf0}, {0x7c,0x90}, {0x7c,0xb0},
	{0x95,0x80}, {0x94,0x80}, {0x97,0x80}, {0x96,0x80},
	{0x91,0x80}, {0x90,0x80}, {0x93,0x80}, {0x92,0x80},
	{0x9d,0x80}, {0x9c,0x80}, {0x9f,0x80}, {0x9e,0x80},
	{0x99,0x80}, {0x98,0x80}, {0x9b,0x80}, {0x9a,0x80},
	{0x8a,0xc0}, {0x8a,0x40}, {0x8b,0xc0}, {0x8b,0x40},
	{0x88,0xc0}, {0x88,0x40}, {0x89,0xc0}, {0x89,0x40},
	{0x8e,0xc0}, {0x8e,0x40}, {0x8f,0xc0}, {0x8f,0x40},
	{0x8c,0xc0}, {0x8c,0x40}, {0x8d,0xc0}, {0x8d,0x40},
	{0xd6,0x00}, {0xd2,0x00}, {0xde,0x00}, {0xda,0x00},
	{0xc6,0x00}, {0xc2,0x00}, {0xce,0x00}, {0xca,0x00},
	{0xf6,0x00}, {0xf2,0x00}, {0xfe,0x00}, {0xfa,0x00},
	{0xe6,0x00}, {0xe2,0x00}, {0xee,0x00}, {0xea,0x00},
	{0xab,0x00}, {0xa9,0x00}, {0xaf,0x00}, {0xad,0x00},
	{0xa3,0x00}, {0xa1,0x00}, {0xa7,0x00}, {0xa5,0x00},
	{0xbb,0x00}, {0xb9,0x00}, {0xbf,0x00}, {0xbd,0x00},
	{0xb3,0x00}, {0xb1,0x00}, {0xb7,0x00}, {0xb5,0x00},
	{0x81,0x58}, {0x81,0x48}, {0x81,0x78}, {0x81,0x68},
	{0x81,0x18}, {0x81,0x08}, {0x81,0x38}, {0x81,0x28},
	{0x81,0xd8}, {0x81,0xc8}, {0x81,0xf8}, {0x81,0xe8},
	{0x81,0x98}, {0x81,0x88}, {0x81,0xb8}, {0x81,0xa8},
	{0x80,0x58}, {0x80,0x48}, {0x80,0x78}, {0x80,0x68},
	{0x80,0x18}, {0x80,0x08}, {0x80,0x38}, {0x80,0x28},
	{0x80,0xd8}, {0x80,0xc8}, {0x80,0xf8}, {0x80,0xe8},
	{0x80,0x98}, {0x80,0x88}, {0x80,0xb8}, {0x80,0xa8},
	{0x85,0x60}, {0x85,0x20}, {0x85,0xe0}, {0x85,0xa0},
	{0x84,0x60}, {0x84,0x20}, {0x84,0xe0}, {0x84,0xa0},
	{0x87,0x60}, {0x87,0x20}, {0x87,0xe0}, {0x87,0xa0},
	{0x86,0x60}, {0x86,0x20}, {0x86,0xe0}, {0x86,0xa0},
	{0x82,0xb0}, {0x82,0x90}, {0x82,0xf0}, {0x82,0xd0},
	{0x82,0x30}, {0x82,0x10}, {0x82,0x70}, {0x82,0x50},
	{0x83,0xb0}, {0x83,0x90}, {0x83,0xf0}, {0x83,0xd0},
	{0x83,0x30}, {0x83,0x10}, {0x83,0x70}, {0x83,0x50},
};

void
mulaw_to_slinear16_le(u_char *p, int cc)
{
	u_char *q = p;

	p += cc;
	q += cc << 1;
	while (--cc >= 0) {
		--p;
		q -= 2;
		q[1] = mulawtolin16[*p][0] ^ 0x80;
		q[0] = mulawtolin16[*p][1];
	}
}

void
alaw_to_slinear16_le(u_char *p, int cc)
{
	u_char *q = p;

	p += cc;
	q += cc << 1;
	while (--cc >= 0) {
		--p;
		q -= 2;
		q[1] = alawtolin16[*p][0] ^ 0x80;
		q[0] = alawtolin16[*p][1];
	}
}


/* headerless data files.  played at /dev/audio's defaults.
 */
#define FMT_RAW	0

/* Sun/NeXT .au files.  header is skipped and /dev/audio is configured
 * for monaural 8-bit ulaw @ 8kHz, the de facto format for .au files,
 * as well as the historical default configuration for /dev/audio.
 */
#define FMT_AU	1

/* RIFF WAV files.  header is parsed for format details which are
 * applied to /dev/audio.
 */
#define FMT_WAV	2


int
legacy_play(char *dev, char *aufile)
{
	struct sio_hdl *hdl;
	struct sio_par spar, par;
	struct aparams apar;
	ssize_t rd;
	off_t datasz;
	char buf[5120];
	size_t readsz;
	int fd, fmt = FMT_RAW;
	u_int32_t pos = 0, snd_fmt = 1, rate = 8000, chan = 1;
	char magic[4];
	int is_ulaw, is_alaw, wav_fmt;

	if ((fd = open(aufile, O_RDONLY)) < 0) {
		warn("cannot open %s", aufile);
		return(1);
	}

	if (read(fd, magic, sizeof(magic)) != sizeof(magic)) {
		/* read() error, or the file is smaller than sizeof(magic).
		 * treat as a raw file, like previous versions of aucat.
		 */
	} else if (!strncmp(magic, ".snd", 4)) {
		fmt = FMT_AU;
		if (read(fd, &pos, sizeof(pos)) == sizeof(pos))
			pos = ntohl(pos);
		/* data size */
		if (lseek(fd, 4, SEEK_CUR) == -1)
			warn("lseek hdr");
		if (read(fd, &snd_fmt, sizeof(snd_fmt)) == sizeof(snd_fmt))
			snd_fmt = ntohl(snd_fmt);
		if (read(fd, &rate, sizeof(rate)) == sizeof(rate))
			rate = ntohl(rate);
		if (read(fd, &chan, sizeof(chan)) == sizeof(chan))
			chan = ntohl(chan);
	} else if (!strncmp(magic, "RIFF", 4) &&
		    wav_readhdr(fd, &apar, &datasz, &wav_fmt)) {
			fmt = FMT_WAV;
	}

	/* seek to start of audio data.  wav_readhdr already took care
	 * of this for FMT_WAV.
	 */
	if (fmt == FMT_RAW || fmt == FMT_AU)
		if (lseek(fd, (off_t)pos, SEEK_SET) == -1)
			warn("lseek");

	if ((hdl = sio_open(dev, SIO_PLAY, 0)) < 0) {
		warnx("can't get sndio handle");
		return(1);
	}

	sio_initpar(&par);
	is_ulaw = is_alaw = 0;
	switch(fmt) {
	case FMT_WAV:
		par.rate = apar.rate;
		par.pchan = apar.cmax - apar.cmin + 1;
		par.sig = apar.sig;
		par.bits = apar.bits;
		par.le = apar.le;
		if (wav_fmt == 6 || wav_fmt == 7) {
			if (wav_fmt == 6)
				is_alaw = 1;
			else if (wav_fmt == 7)
				is_ulaw = 1;
			par.sig = 1;
			par.bits = 16;
			par.le = 1;
		} else if (wav_fmt != 1)
			warnx("format not supported");
		break;
	case FMT_AU:
		par.rate = rate;
		par.pchan = chan;
		par.sig = 1;
		par.bits = 16;
		par.le = 1;
		is_ulaw = 1;
		if (snd_fmt == 27)
			is_alaw = 1;
		break;
	case FMT_RAW:
	default:
		break;
	}
	spar = par;

	if (!sio_setpar(hdl, &par) || !sio_getpar(hdl, &par)) {
		warnx("can't set audio parameters");
		/* only WAV could fail in previous aucat versions (unless
		 * the parameters returned by AUDIO_GETINFO would fail,
		 * which is unlikely)
		 */
		if (fmt == FMT_WAV)
			return(1);
	}

	/* parameters may be silently modified.  see audio(9)'s
	 * description of set_params.  for compatability with previous
	 * aucat versions, continue running if something doesn't match.
	 */
	if (par.bits != spar.bits ||
	    par.sig != par.sig ||
	    par.le != spar.le ||
	    par.pchan != spar.pchan ||
	    /* devices may return a very close rate, such as 44099 when
	     * 44100 was requested.  the difference is inaudible.  allow
	     * 2% deviation as an example of how to cope.
	     */
	    (par.rate > spar.rate * 1.02 || par.rate < spar.rate * 0.98)) {
		warnx("format not supported");
	}
	if (!sio_start(hdl)) {
		warnx("could not start sndio");
		exit(1);
	}

	readsz = sizeof(buf);
	if (is_ulaw || is_alaw)
		readsz /= 2;
	while ((rd = read(fd, buf, readsz)) > 0) {
		if (is_ulaw) {
			mulaw_to_slinear16_le(buf, rd);
			rd *= 2;
		} else if (is_alaw) {
			alaw_to_slinear16_le(buf, rd);
			rd *= 2;
		}
		if (sio_write(hdl, buf, rd) != rd)
			warnx("sio_write: short write");
	}
	if (rd == -1)
		warn("read");

	sio_close(hdl);
	(void) close(fd);

	return(0);
}
