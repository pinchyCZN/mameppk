
/***************************************************************************
  Original Source from EmeraldMame 185
***************************************************************************/

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#define _WIN32_IE 0x0501
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

// standard C headers
#include <fcntl.h>
#include <io.h>
#include <sys\stat.h>

// MAME/MAMEUI headers
#include "emucore.h"
#include "Wav.h"

// for VC2005
#if defined(_MSC_VER) && _MSC_VER >= 1400
#define open _open
#define close _close
#define read _read
#define write _write
#define lseek _lseek
#define tell _tell
#endif



static struct RiffWaveHeader
{
    FOURCC     ckid;
    DWORD      cksize;
    FOURCC     fccType;
    FOURCC     fmt;
    DWORD      fmtsize;
    WAVEFORMAT wfmt;
    WORD       nBitRate;
    FOURCC     data;
    DWORD      datasize;
} wh;

static char wavfilename[_MAX_PATH];
static int wavfile = -1;

static int                  stereo_factor;
static UINT32               samples_per_sec;
static int                  samples_bit_rate;

int wav_wavefile_create(const void* fname, UINT32 samplerate, int bitrate, int channel )
{
	int file;
	file = open((const char *)fname, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IREAD|S_IWRITE);
	if (file == -1)
        return -1;


    wh.ckid                 = FOURCC_RIFF;
    wh.cksize               = 0;
    wh.fccType              = MAKEFOURCC('W', 'A', 'V', 'E');
    wh.fmt                  = MAKEFOURCC('f', 'm', 't', ' ');
    wh.fmtsize              = 16;
    wh.wfmt.wFormatTag      = WAVE_FORMAT_PCM;
    wh.wfmt.nChannels       = channel;
    wh.wfmt.nSamplesPerSec  = samplerate;
	wh.nBitRate             = bitrate;
    wh.wfmt.nBlockAlign     = wh.wfmt.nChannels * (wh.nBitRate / 8);
    wh.wfmt.nAvgBytesPerSec = wh.wfmt.nSamplesPerSec * wh.wfmt.nBlockAlign;
    wh.data                 = MAKEFOURCC('d', 'a', 't', 'a');
    wh.datasize             = 0;

	write(file, &wh, sizeof(struct RiffWaveHeader));
	return file;
}


struct WAV_WAVEFORMAT wav_wavefile_getformat(const void* fname)
{
	struct WAV_WAVEFORMAT wfm;
	struct RiffWaveHeader wh;
	int file;

	memset(&wfm, 0, sizeof(wfm));

	file = open((const char *)fname, O_RDONLY | O_BINARY);
	if (file == -1)
        return wfm;

	read(file, &wh, sizeof(struct RiffWaveHeader));

	close(file);

	if (
		wh.ckid                 == FOURCC_RIFF &&
		wh.fccType              == MAKEFOURCC('W', 'A', 'V', 'E') &&
		wh.fmt                  == MAKEFOURCC('f', 'm', 't', ' ') &&
		wh.wfmt.wFormatTag      == WAVE_FORMAT_PCM
		)
	{
		wfm.channel				= wh.wfmt.nChannels;
		wfm.bitrate				= wh.nBitRate;
		wfm.samplespersec		= wh.wfmt.nSamplesPerSec;
		return wfm;
	}
	return wfm;
}

int wav_start_log_wave(const void* fname, struct WAV_WAVEFORMAT *wfm )
{

    strcpy(wavfilename, (const char *)fname);
    wavfile = open(wavfilename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IREAD|S_IWRITE);
	if (wavfile == -1)
        return -1;

	samples_per_sec		= wfm->samplespersec;
	samples_bit_rate	= wfm->bitrate;
	stereo_factor		= wfm->channel;

	if (samples_per_sec < 1) samples_per_sec = 1;
	if (samples_per_sec > 44100*4) samples_per_sec = 44100*4;
	if (samples_bit_rate < 8) stereo_factor = 8;
	if (samples_bit_rate > 16) stereo_factor = 16;
	if (stereo_factor < 1) stereo_factor = 1;
	if (stereo_factor > 2) stereo_factor = 2;

    wh.ckid                 = FOURCC_RIFF;
    wh.cksize               = 0;
    wh.fccType              = MAKEFOURCC('W', 'A', 'V', 'E');
    wh.fmt                  = MAKEFOURCC('f', 'm', 't', ' ');
    wh.fmtsize              = 16;
    wh.wfmt.wFormatTag      = WAVE_FORMAT_PCM;
    wh.wfmt.nChannels       = stereo_factor;
    wh.wfmt.nSamplesPerSec  = samples_per_sec;
	wh.nBitRate             = samples_bit_rate;
    wh.wfmt.nBlockAlign     = wh.wfmt.nChannels * (wh.nBitRate / 8);
    wh.wfmt.nAvgBytesPerSec = wh.wfmt.nSamplesPerSec * wh.wfmt.nBlockAlign;
    wh.data                 = MAKEFOURCC('d', 'a', 't', 'a');
    wh.datasize             = 0;


	write(wavfile, &wh, sizeof(struct RiffWaveHeader));
	return 0;
}

int wav_append_log_wave(void)
{
	wavfile = open(wavfilename, O_WRONLY | O_BINARY);

    if (wavfile == -1)
        return -1;

	lseek(wavfile, 0, SEEK_END);
	return 0;
}

void wav_wavefile_close(int file)
{
    int len;

    if (file == -1)
        return;

	len = tell(file);

    wh.cksize   = len - 8;
    wh.datasize = len - sizeof(struct RiffWaveHeader);

    lseek(file, 0, SEEK_SET);
    write(file, &wh, sizeof(struct RiffWaveHeader));

    close(file);

	return;
}

void wav_stop_log_wave(void)
{
    int len;

    if (wavfile == -1)
        return;

	len = tell(wavfile);

    wh.cksize   = len - 8;
    wh.datasize = len - sizeof(struct RiffWaveHeader);

    lseek(wavfile, 0, SEEK_SET);
    write(wavfile, &wh, sizeof(struct RiffWaveHeader));

    close(wavfile);

    wavfile = -1;
}

void wav_log_wave(void* buf, UINT32 len)
{
    if (wavfile != -1 && buf)
	{
		write(wavfile, buf, len *(stereo_factor*(samples_bit_rate / 8)) );
	}
}

static int ClippingMin =-32768;
static int ClippingMax = 32767;

void wav_wavecnv_set_clipping(int min, int max)
{
	if (min < -32768) min =-32768;
	if (min >  32767) min = 32767;
	if (max < -32768) max =-32768;
	if (max >  32767) max = 32767;

	ClippingMin = min;
	ClippingMax = max;
}

int wav_convert_select(struct WAV_WAVEFORMAT *dstwfm, struct WAV_WAVEFORMAT *srcwfm, struct WAV_SAMPLES_RESIZE *wsr, struct WAV_SAMPLES_RESAMPLE *wsre)
{

	int nmb=0;

	if (wsr != NULL)
	{
		wsr->adder = 1 << 16;
		wsr->pos = 0;
	}

	if (wsre != NULL)
	{
		wsre->adder = 1 << 16;
		wsre->buf = NULL;
	}

	if (srcwfm->channel			!= dstwfm->channel ||
		srcwfm->bitrate			!= dstwfm->bitrate ||
		srcwfm->samplespersec	!= dstwfm->samplespersec
		)
	{
		nmb |= (srcwfm->channel == 2)	? 0x1:0;
		nmb |= (srcwfm->bitrate == 16)	? 0x2:0;
		nmb |= (dstwfm->channel == 2)	? 0x4:0;
		nmb |= (dstwfm->bitrate == 16)	? 0x8:0;
		nmb |= (srcwfm->samplespersec != dstwfm->samplespersec)	?0x10:0;

		if (nmb & 0x10)
			nmb |= (wsre != NULL) ? 0x20:0;
		
		if ((nmb & 0x30) == 0x10)
		{
			if (wsr != NULL)
			{
				wsr->adder = (UINT32)(((UINT64)srcwfm->samplespersec<<16) / (UINT32)dstwfm->samplespersec);
				wsr->pos = 0;
			}
		}
		if ( nmb & 0x20 )
		{
			wsre->adder = (UINT32)(((UINT64)srcwfm->samplespersec<<16) / (UINT32)dstwfm->samplespersec);
			wsre->next = 0;
			wsre->count = 0;
			wsre->pos_start = 0;
			wsre->pos		= wsre->pos_start;
			wsre->pos_end	= wsre->pos_start + wsre->adder;
			
			if (wsre->adder < 0x10000)
			{
				nmb |= 0x30;
				wsre->pos_end = 0;
				wsre->buf = malloc(3 * srcwfm->channel * (srcwfm->bitrate/8));
			} else
			{
				nmb &= ~0x10;
				wsre->buf = malloc(((wsre->adder + 0x1ffff) >> 16) * srcwfm->channel * (srcwfm->bitrate/8));
			}

			if (wsr != NULL)
			{
				wsr->adder = wsre->adder;
				wsr->pos = 0;
			}

		}


	} else
		return -1;

	return nmb;

}


FPTR wav_wavecnv_resize(int cnvnmb, void* dst, void* src, FPTR samples, struct WAV_SAMPLES_RESIZE *wsr)
{
	const FPTR destpos = (FPTR)dst;
	const FPTR adder = wsr->adder;
	FPTR fpos = wsr->pos;
	FPTR upos = fpos>>16;


	switch (cnvnmb & 0xf)
	{
	case 0:
		break;
	case 1:
		break;
	case 2:					//mo16_to_mo8
		while( upos < samples )
		{
			const int s = (int)(*((INT16*)src+upos));
			*((UINT8*)dst) = ((s >> 8) + 128) & 0xff;
			dst = (INT8*)dst+1;
			fpos = (fpos & 0xffff)+adder;
			upos += (fpos>>16);
		}
		wsr->pos = fpos - ((samples - (upos - (fpos>>16)))<<16);
		samples = ((FPTR)dst - destpos)/sizeof(INT8);
		break;
	case 3:					//st16_to_mo8
		while( upos < samples )
		{
			const int sl = (int)(*((INT16*)((INT32*)src+upos)));
			const int sr = (int)(*((INT16*)((INT32*)src+upos) + 1));
			const short s = (INT16)((sl + sr) >> 1);
			*((UINT8*)dst) = ((s >> 8) + 128) & 0xff;
			dst = (INT8*)dst+1;
			fpos = (fpos & 0xffff)+adder;
			upos += (fpos>>16);
		}
		wsr->pos = fpos - ((samples - (upos - (fpos>>16)))<<16);
		samples = ((FPTR)dst - destpos)/sizeof(INT8);
		break;
	case 4:
		break;
	case 5:
		break;
	case 6:					//mo16_to_st8
		while( upos < samples )
		{
			const UINT16 s = (((*((INT16*)src+upos)) >> 8) + 128) & 0xff;
			*((INT16*)dst) = s | (s << 8);
			dst = (INT16*)dst+1;
			fpos = (fpos & 0xffff)+adder;
			upos += (fpos>>16);
		}
		wsr->pos = fpos - ((samples - (upos - (fpos>>16)))<<16);
		samples = ((FPTR)dst - destpos)/sizeof(INT16);
		break;
	case 7:					//st16_to_st8
		while( upos < samples )
		{
			const UINT16 sl = (((*((INT16*)((INT32*)src+upos)))	>> 8) + 128) & 0xff;
			const UINT16 sr = (((*((INT16*)((INT32*)src+upos) + 1))>> 8) + 128) & 0xff;
			*((INT16*)dst) = sl | (sr << 8);
			dst = (INT16*)dst+1;
			fpos = (fpos & 0xffff)+adder;
			upos += (fpos>>16);
		}
		wsr->pos = fpos - ((samples - (upos - (fpos>>16)))<<16);
		samples = ((FPTR)dst - destpos)/sizeof(INT16);
		break;

	case 8:
		break;
	case 9:
		break;
	case 10:				//mo16_to_mo16
		while( upos < samples )
		{
			const INT16 s = *((INT16*)src+upos);
			*((INT16*)dst) = s;
			dst = (INT16*)dst+1;
			fpos = (fpos & 0xffff)+adder;
			upos += (fpos>>16);
		}
		wsr->pos = fpos - ((samples - (upos - (fpos>>16)))<<16);
		samples = ((FPTR)dst - destpos)/sizeof(INT16);
		break;
	case 11:				//st16_to_mo16
		while( upos < samples )
		{
			const int sl = (int)(*((INT16*)((INT32*)src+upos)));
			const int sr = (int)(*((INT16*)((INT32*)src+upos) + 1));
			*((INT16*)dst) = (short)((sl + sr) >> 1);
			dst = (INT16*)dst+1;
			fpos = (fpos & 0xffff)+adder;
			upos += (fpos>>16);
		}
		wsr->pos = fpos - ((samples - (upos - (fpos>>16)))<<16);
		samples = ((FPTR)dst - destpos)/sizeof(INT16);
		break;
	case 12:
		break;
	case 13:
		break;
	case 14:				//mo16_to_st16
		while( upos < samples )
		{
			const UINT32 s = *((UINT16*)src+upos);
			*((INT32*)dst) = s | (s << 16);
			dst = (INT32*)dst+1;
			fpos = (fpos & 0xffff)+adder;
			upos += (fpos>>16);
		}
		wsr->pos = fpos - ((samples - (upos - (fpos>>16)))<<16);
		samples = ((FPTR)dst - destpos)/sizeof(INT32);
		break;
	case 15:				//st16_to_st16
		while( upos < samples )
		{
			const int s = *((INT32*)src+upos);
			*((INT32*)dst) = s;
			dst = (INT32*)dst+1;
			fpos = (fpos & 0xffff)+adder;
			upos += (fpos>>16);
		}
		wsr->pos = fpos - ((samples - (upos - (fpos>>16)))<<16);
		samples = ((FPTR)dst - destpos)/sizeof(INT32);
		break;
	}

	return samples;
}


void wav_wavecnv(int cnvnmb, void* dst, void* src, FPTR samples)
{

	switch (cnvnmb & 0xf)
	{
	case 0:
		break;
	case 1:
		break;
	case 2:
		while( samples )	//mo16_to_mo8
		{
			const int s = (int)(*((INT16*)src));
			*((UINT8*)dst) = ((s >> 8) + 128) & 0xff;
			src = (INT16*)src+1;
			dst = (INT8*)dst+1;
			samples--;
		}
		break;
	case 3:					//st16_to_mo8
		while( samples )
		{
			const int sl = (int)(*((INT16*)src));
			const int sr = (int)(*((INT16*)src + 1));
			const short s = (INT16)((sl + sr) >> 1);
			*((UINT8*)dst) = ((s >> 8) + 128) & 0xff;
			src = (INT32*)src+1;
			dst = (INT8*)dst+1;
			samples--;
		}
		break;
	case 4:
		break;
	case 5:
		break;
	case 6:					//mo16_to_st8
		while( samples )
		{
			const UINT16 s = (((*((INT16*)src))	>> 8) + 128) & 0xff;
			*((INT16*)dst) = s | (s << 8);
			src = (INT16*)src+1;
			dst = (INT16*)dst+1;
			samples--;
		}
		break;
	case 7:					//st16_to_st8
		while( samples )
		{
			const UINT16 sl = (((*((INT16*)src))	>> 8) + 128) & 0xff;
			const UINT16 sr = (((*((INT16*)src + 1))>> 8) + 128) & 0xff;
			*((INT16*)dst) = sl | (sr << 8);
			src = (INT32*)src+1;
			dst = (INT16*)dst+1;
			samples--;
		}
		break;

	case 8:
		break;
	case 9:
		break;
	case 10:
		break;
	case 11:				//st16_to_mo16
		while( samples )
		{
			const int sl = (int)(*((INT16*)src));
			const int sr = (int)(*((INT16*)src + 1));
			*((INT16*)dst) = (INT16)((sl + sr) >> 1);
			src = (INT32*)src+1;
			dst = (INT16*)dst+1;
			samples--;
		}
		break;
	case 12:
		break;
	case 13:
		break;
	case 14:				//mo16_to_st16
		while( samples )
		{
			const UINT32 s = *((UINT16*)src);
			*((INT32*)dst) = s | (s << 16);
			src = (INT16*)src+1;	dst = (INT32*)dst+1;
			samples--;
		}
		break;
	case 15:
		break;
	}


}


static int rr1, rr2;
#define RESAMPLE_S(dsti,srci,samples,wsre)	\
		while( samples )\
		{\
			((srci*)wsre->buf)[wsre->count++] = (*(srci*)src);\
			src = (srci*)src+1;\
			wsre->pos += 0x10000;\
			if (wsre->pos >= wsre->pos_end)\
			{\
				const FPTR j = ((wsre->pos + 0xffff) >> 16) + wsre->next;\
				const FPTR s=wsre->pos_start & 0xffff;\
				FPTR e=wsre->pos_end & 0xffff;\
				FPTR t = j<<16;\
				int r1,r2;\
\
				wsre->count = 0;\
				\
				if (e==0) e=0x10000;\
				t = t - s;\
				t = t - (0x10000-e);\
\
				r1 = 0; r2 = 0;\
				for(i=0; i<j; i++)\
				{\
					const int sample = ((srci*)wsre->buf)[i];\
					int alpha = 0x10000;\
					if(i==0) \
						alpha = 0x10000 - s;\
					else \
						if(i==(j-1)) alpha = e;\
					alpha = (unsigned int)(((unsigned __int64)alpha << 16) / t)>>1;

static double 	f1, f2, ff1, ff2;
static int		r1, r2;
#define RESAMPLE_S_F(DSTI,SRCI,samples,wsre)	\
		while( samples )\
		{\
			\
			((SRCI*)wsre->buf)[wsre->count++] = (*(SRCI*)src);\
			src = (SRCI*)src+1;\
			wsre->pos += 0x10000;\
			if (wsre->pos >= wsre->pos_end)\
			{\
				const FPTR j = ((wsre->pos + 0xffff) >> 16) + wsre->next;\
				const double	s=(double)(wsre->pos_start & 0xffff) / 65536.0;\
				double			e=(double)(wsre->pos_end & 0xffff) / 65536.0;\
				double t = (double)j;\
\
				wsre->count = 0;\
				\
				if (e==0) e=1.0;\
				t = t - s;\
				t = t - (1.0-e);\
\
				f1 = 0; f2 = 0;\
				for(i=0; i<j; i++)\
				{\
					const int sample = ((SRCI*)wsre->buf)[i];\
					double alpha = 1.0;\
					if(i==0) \
						alpha = 1.0 - s;\
					else \
						if(i==(j-1)) alpha = e;\
					alpha /= t;


#define RESAMPLE_E(dsti,srci,samples,wsre)	\
				if((wsre->pos_end & 0xffff) && (j>1))\
				{\
					const srci tmp = ((srci*)wsre->buf)[0];\
					((srci*)wsre->buf)[0] = ((srci*)wsre->buf)[j-1];\
					((srci*)wsre->buf)[j-1] = tmp;\
					wsre->count++;\
					wsre->next = 1;\
				} else\
					wsre->next = 0;\
\
				wsre->pos_start = wsre->pos_end & 0xffff;\
				wsre->pos_end = wsre->pos_end +  wsre->adder - wsre->pos;\
				wsre->pos = wsre->pos & 0xffff;\
			}\
			samples--;\
		}\
		samples = ((FPTR)dst - destpos)/sizeof(dsti);

FPTR wav_wavecnv_resample(int cnvnmb, void* dst, void* src, FPTR samples, struct WAV_SAMPLES_RESAMPLE *wsre)
{
	const FPTR destpos = (FPTR)dst;
	FPTR i;


	switch (cnvnmb & 0xf)
	{
	case 0:
		break;
	case 1:
		break;
	case 2:					//mo16_to_mo8
		RESAMPLE_S(UINT8, INT16, samples, wsre)
					rr1 = (sample <<16 >>16);
					r1 += (rr1 * alpha);
				}
				if(r1>(ClippingMax<<15)) r1=ClippingMax<<15;
				else	if(r1<(ClippingMin<<15)) r1=ClippingMin<<15;

				*((UINT8*)dst) = ((r1 >> (15+8)) + 128) & 0xff;
				dst = (UINT8*)dst+1;
		RESAMPLE_E(UINT8, INT16, samples, wsre)
		break;
	case 3:					//st16_to_mo8
		RESAMPLE_S(UINT8, INT32, samples, wsre)
					rr1 = (sample <<16 >>16);
					rr2 = (sample >> 16);
					r1 += (rr1 * alpha);
					r2 += (rr2 * alpha);
				}

				if(r1>(ClippingMax<<15)) r1=ClippingMax<<15;
				else	if(r1<(ClippingMin<<15)) r1=ClippingMin<<15;
				if(r2>(ClippingMax<<15)) r2=ClippingMax<<15;
				else	if(r2<(ClippingMin<<15)) r2=ClippingMin<<15;

				*((UINT8*)dst) = ((((r1>>15) + (r2>>15))>>(1+8)) + 128) & 0xff;
				dst = (UINT8*)dst+1;
		RESAMPLE_E(UINT8, INT32, samples, wsre)
		break;
	case 4:
		break;
	case 5:
		break;
	case 6:					//mo16_to_st8
		RESAMPLE_S(INT16, INT16, samples, wsre)
					rr1 = (sample <<16 >>16);
					r1 += (rr1 * alpha);
				}
				if(r1>(ClippingMax<<15)) r1=ClippingMax<<15;
				else	if(r1<(ClippingMin<<15)) r1=ClippingMin<<15;

				*((INT16*)dst) = (((r1 >> (15+8))+ 128) & 0xff) | (((r1 >> 15)+ (128<<8)) & 0xff00);
				dst = (INT16*)dst+1;
		RESAMPLE_E(INT16, INT16, samples, wsre)
		break;
	case 7:					//st16_to_st8
		RESAMPLE_S(INT16, INT32, samples, wsre)
					rr1 = (sample <<16 >>16);
					rr2 = (sample >> 16);

					r1 += (rr1 * alpha);
					r2 += (rr2 * alpha);
				}

				if(r1>(ClippingMax<<15)) r1=ClippingMax<<15;
				else	if(r1<(ClippingMin<<15)) r1=ClippingMin<<15;
				if(r2>(ClippingMax<<15)) r2=ClippingMax<<15;
				else	if(r2<(ClippingMin<<15)) r2=ClippingMin<<15;
				
				*((INT16*)dst) = (((r1 >> (15+8))+ 128) & 0xff) | (((r2 >> 15)+ (128<<8)) & 0xff00);
				dst = (INT16*)dst+1;
		RESAMPLE_E(INT16, INT32, samples, wsre)
		break;

	case 8:
		break;
	case 9:
		break;
	case 10:				//mo16_to_mo16
		RESAMPLE_S(INT16, INT16, samples, wsre)
					rr1 = (sample <<16 >>16);
					r1 += (rr1 * alpha);
				}
				if(r1>(ClippingMax<<15)) r1=ClippingMax<<15;
				else	if(r1<(ClippingMin<<15)) r1=ClippingMin<<15;

				*((INT16*)dst) = (INT16)(r1>>15);
				dst = (INT16*)dst+1;
		RESAMPLE_E(INT16, INT16, samples ,wsre)
		break;
	case 11:				//st16_to_mo16
		RESAMPLE_S(INT16, INT32, samples, wsre)
					rr1 = (sample <<16 >>16);
					rr2 = (sample >> 16);

					r1 += (rr1 * alpha);
					r2 += (rr2 * alpha);
				}

				if(r1>(ClippingMax<<15)) r1=ClippingMax<<15;
				else	if(r1<(ClippingMin<<15)) r1=ClippingMin<<15;
				if(r2>(ClippingMax<<15)) r2=ClippingMax<<15;
				else	if(r2<(ClippingMin<<15)) r2=ClippingMin<<15;

				*((INT16*)dst) = (INT16)(((r1>>15) + (r2>>15))>>1);
				dst = (INT16*)dst+1;
		RESAMPLE_E(INT16, INT32, samples, wsre)
		break;
	case 12:
		break;
	case 13:
		break;
	case 14:				//mo16_to_st16
		RESAMPLE_S(INT32, INT16, samples, wsre)
					rr1 = (sample <<16 >>16);
					r1 += (rr1 * alpha);
				}
				if(r1>(ClippingMax<<15)) r1=ClippingMax<<15;
				else	if(r1<(ClippingMin<<15)) r1=ClippingMin<<15;

				*((INT32*)dst) = ((r1>>15) & 0xffff) | ((r1<<1) & 0xffff0000);
				dst = (INT32*)dst+1;
		RESAMPLE_E(INT32, INT16, samples, wsre)
		break;
	case 15:				//st16_to_st16
		RESAMPLE_S(INT32, INT32, samples, wsre)
					rr1 = (sample <<16 >>16);
					rr2 = (sample >> 16);

					r1 += (rr1 * alpha);
					r2 += (rr2 * alpha);
				}

				if(r1>(ClippingMax<<15)) r1=ClippingMax<<15;
				else	if(r1<(ClippingMin<<15)) r1=ClippingMin<<15;
				if(r2>(ClippingMax<<15)) r2=ClippingMax<<15;
				else	if(r2<(ClippingMin<<15)) r2=ClippingMin<<15;
				
				r1 = ((r1>>15) & 0xffff) | ((r2<<1)&0xffff0000);
				*((INT32*)dst) = r1;
				dst = (INT32*)dst+1;
		RESAMPLE_E(INT32, INT32, samples, wsre)
		break;
	}
	return samples;
}




FPTR wav_wavecnv_resample_f(int cnvnmb, void* dst, void* src, FPTR samples, struct WAV_SAMPLES_RESAMPLE *wsre)
{
	const FPTR destpos = (FPTR)dst;
	FPTR i;


	switch (cnvnmb & 0xf)
	{
	case 0:
		break;
	case 1:
		break;
	case 2:					//mo16_to_mo8
		RESAMPLE_S_F(UINT8, INT16, samples, wsre)
					ff1 = (double)(sample <<16 >>16);
					f1 += (ff1 * alpha);
				}
				r1 = (int)f1;

				if(r1>ClippingMax) r1=ClippingMax;
				else	if(r1<ClippingMin) r1=ClippingMin;

				*((UINT8*)dst) = ((r1 >> 8) + 128) & 0xff;
				dst = (INT8*)dst+1;
		RESAMPLE_E(UINT8, INT16, samples, wsre)
		break;
	case 3:					//st16_to_mo8
		RESAMPLE_S_F(UINT8, INT32, samples, wsre)
					ff1 = (double)(sample <<16 >>16);
					ff2 = (double)(sample >> 16);

					f1 += (ff1 * alpha);
					f2 += (ff2 * alpha);
				}
				r1 = (int)f1;
				r2 = (int)f2;

				if(r1>ClippingMax) r1=ClippingMax;
				else	if(r1<ClippingMin) r1=ClippingMin;
				if(r2>ClippingMax) r2=ClippingMax;
				else	if(r2<ClippingMin) r2=ClippingMin;

				*((UINT8*)dst) = (((r1 + r2)>>(1+8)) + 128) & 0xff;
				dst = (INT8*)dst+1;
		RESAMPLE_E(UINT8, INT32, samples, wsre)
		break;
	case 4:
		break;
	case 5:
		break;
	case 6:					//mo16_to_st8
		RESAMPLE_S_F(INT16, INT16, samples, wsre)
					ff1 = (double)(sample <<16 >>16);
					f1 += (ff1 * alpha);
				}
				r1 = (int)f1;

				if(r1>ClippingMax) r1=ClippingMax;
				else	if(r1<ClippingMin) r1=ClippingMin;

				*((INT16*)dst) = (((r1 >> 8)+ 128) & 0xff) | ((r1 + (128<<8)) & 0xff00);
				dst = (INT16*)dst+1;
		RESAMPLE_E(INT16, INT16, samples, wsre)
		break;
	case 7:					//st16_to_st8
		RESAMPLE_S_F(INT16, INT32, samples, wsre)
					ff1 = (double)(sample <<16 >>16);
					ff2 = (double)(sample >> 16);

					f1 += (ff1 * alpha);
					f2 += (ff2 * alpha);
				}
				r1 = (int)f1;
				r2 = (int)f2;

				if(r1>ClippingMax) r1=ClippingMax;
				else	if(r1<ClippingMin) r1=ClippingMin;
				if(r2>ClippingMax) r2=ClippingMax;
				else	if(r2<ClippingMin) r2=ClippingMin;


				*((INT16*)dst) = (((r1 >> 8)+ 128) & 0xff) | ((r2 + (128<<8)) & 0xff00);
				dst = (INT16*)dst+1;
		RESAMPLE_E(INT16, INT32, samples, wsre)
		break;

	case 8:
		break;
	case 9:
		break;
	case 10:				//mo16_to_mo16
		RESAMPLE_S_F(INT16, INT16, samples, wsre)
					ff1 = (double)(sample <<16 >>16);
					f1 += (ff1 * alpha);
				}
				r1 = (int)f1;

				if(r1>ClippingMax) r1=ClippingMax;
				else	if(r1<ClippingMin) r1=ClippingMin;

				*((INT16*)dst) = (INT16)r1;
				dst = (INT16*)dst+1;
		RESAMPLE_E(INT16, INT16, samples, wsre)
		break;
	case 11:				//st16_to_mo16
		RESAMPLE_S_F(INT16, INT32, samples, wsre)
					ff1 = (double)(sample <<16 >>16);
					ff2 = (double)(sample >> 16);

					f1 += (ff1 * alpha);
					f2 += (ff2 * alpha);
				}
				r1 = (int)f1;
				r2 = (int)f2;

				if(r1>ClippingMax) r1=ClippingMax;
				else	if(r1<ClippingMin) r1=ClippingMin;
				if(r2>ClippingMax) r2=ClippingMax;
				else	if(r2<ClippingMin) r2=ClippingMin;

				*((INT16*)dst) = (INT16)((r1 + r2)>>1);
				dst = (INT16*)dst+1;
		RESAMPLE_E(INT16, INT32, samples, wsre)
		break;
	case 12:
		break;
	case 13:
		break;
	case 14:				//mo16_to_st16
		RESAMPLE_S_F(INT32, INT16, samples, wsre)
					ff1 = (double)(sample <<16 >>16);
					f1 += (ff1 * alpha);
				}
				r1 = (int)f1;

				if(r1>ClippingMax) r1=ClippingMax;
				else	if(r1<ClippingMin) r1=ClippingMin;

				*((INT32*)dst) = (r1 & 0xffff) | ((r1<<16) & 0xffff0000);
				dst = (INT32*)dst+1;
		RESAMPLE_E(INT32, INT16, samples, wsre)
		break;
	case 15:				//st16_to_st16
		RESAMPLE_S_F(INT32, INT32, samples, wsre)
					ff1 = (double)(sample <<16 >>16);
					ff2 = (double)(sample >> 16);

					f1 += (ff1 * alpha);
					f2 += (ff2 * alpha);
				}
				r1 = (int)f1;
				r2 = (int)f2;

				if(r1>ClippingMax) r1=ClippingMax;
				else	if(r1<ClippingMin) r1=ClippingMin;
				if(r2>ClippingMax) r2=ClippingMax;
				else	if(r2<ClippingMin) r2=ClippingMin;

				*((INT32*)dst) = (r1 & 0xffff) | ((r2<<16) & 0xffff0000);
				dst = (INT32*)dst+1;
		RESAMPLE_E(INT32, INT32, samples, wsre)
		break;
	}
	return samples;
}


#define STRETCH_S(dsti,srci,samples,wsre)	\
		while( samples )\
		{\
			((srci*)wsre->buf)[wsre->count++] = (*(srci*)src);\
			src = (srci*)src+1;\
			if (wsre->count>1)\
			{\
				FPTR j;\
				for(j=wsre->pos_start; j<0x10000; j+=wsre->adder)\
				{\
					const int sample_a = ((srci*)wsre->buf)[1];\
					const int sample_b = ((srci*)wsre->buf)[0];\
					const unsigned int alpha = (j>>1) & 0x7fff;

#define STRETCH_S_F(DSTI,SRCI,samples,wsre)	\
		while( samples )\
		{\
			((SRCI*)wsre->buf)[wsre->count++] = (*(SRCI*)src);\
			src = (SRCI*)src+1;\
			if (wsre->count>1)\
			{\
				FPTR j;\
				for(j=wsre->pos_start; j<0x10000; j+=wsre->adder)\
				{\
					const int sample_a = ((SRCI*)wsre->buf)[1];\
					const int sample_b = ((SRCI*)wsre->buf)[0];\
					const double alpha = (double)(j & 0xffff) / 65536.0;

#define STRETCH_ALPHA_STEREO_F	\
					int r1,r2;\
					double ff1,ff2;\
					double f1 = (double)(sample_a <<16 >>16) * alpha;\
					double f2 = (double)(sample_a >> 16) * alpha;\
\
					ff1 = (sample_b <<16 >>16);\
					ff2 = (sample_b >> 16);\
					f1 += (ff1 * (1.0-alpha));\
					f2 += (ff2 * (1.0-alpha));\
\
					r1 = (int)f1;\
					r2 = (int)f2;\
\
					if(r1>ClippingMax) r1=ClippingMax;\
					else	if(r1<ClippingMin) r1=ClippingMin;\
					if(r2>ClippingMax) r2=ClippingMax;\
					else	if(r2<ClippingMin) r2=ClippingMin;

#define STRETCH_ALPHA_MONO_F	\
					int r1;\
					double ff1;\
					double f1 = (double)(sample_a <<16 >>16) * alpha;\
\
					ff1 = (sample_b <<16 >>16);\
					f1 += (ff1 * (1.0-alpha));\
\
					r1 = (int)f1;\
\
					if(r1>ClippingMax) r1=ClippingMax;\
					else	if(r1<ClippingMin) r1=ClippingMin;

#define STRETCH_E(dsti,srci,samples,wsre)	\
					wsre->pos += wsre->adder;\
				}\
\
				wsre->pos -= 0x10000;\
				wsre->pos_start = wsre->pos;\
				{\
					const srci tmp = ((srci*)wsre->buf)[0];\
					((srci*)wsre->buf)[0] = ((srci*)wsre->buf)[1];\
					((srci*)wsre->buf)[1] = tmp;\
				}\
			}\
			wsre->count=1;\
			samples--;\
		}\
		samples = ((FPTR)dst - destpos)/sizeof(dsti);

FPTR wav_wavecnv_stretch(int cnvnmb, void* dst, void* src, FPTR samples, struct WAV_SAMPLES_RESAMPLE *wsre)
{
	const FPTR destpos = (FPTR)dst;

	switch (cnvnmb & 0xf)
	{
	case 0:
		break;
	case 1:
		break;
	case 2:					//mo16_to_mo8
		STRETCH_S(INT8, INT16, samples, wsre)
					int rr1;
					int r1 = (sample_a <<16 >>16);
					r1 = r1 * alpha;

					rr1 = (sample_b <<16 >>16);
					r1 += (rr1 * (0x8000-alpha));

					if(r1>(ClippingMax<<15)) r1=ClippingMax<<15;
					else	if(r1<(ClippingMin<<15)) r1=ClippingMin<<15;

					*((UINT8*)dst) = ((r1 >> (15+8)) + 128) & 0xff;
					dst = (INT8*)dst+1;
		STRETCH_E(INT8, INT16, samples, wsre)
		break;
	case 3:					//st16_to_mo8
		STRETCH_S(INT8, INT32, samples, wsre)
					int rr1,rr2;
					int r1 = (sample_a <<16 >>16);
					int r2 = (sample_a >> 16);
					r1 = r1 * alpha;
					r2 = r2 * alpha;

					rr1 = (sample_b <<16 >>16);
					rr2 = (sample_b >> 16);
					r1 += (rr1 * (0x8000-alpha));
					r2 += (rr2 * (0x8000-alpha));

					if(r1>(ClippingMax<<15)) r1=ClippingMax<<15;
					else	if(r1<(ClippingMin<<15)) r1=ClippingMin<<15;
					if(r2>(ClippingMax<<15)) r2=ClippingMax<<15;
					else	if(r2<(ClippingMin<<15)) r2=ClippingMin<<15;
					
					*((UINT8*)dst) = ((((r1>>15) + (r2>>15))>>(1+8)) + 128) & 0xff;
					dst = (INT8*)dst+1;
		STRETCH_E(INT8, INT32, samples, wsre)
		break;
	case 4:
		break;
	case 5:
		break;
	case 6:					//mo16_to_st8
		STRETCH_S(INT16, INT16, samples, wsre)
					int rr1;
					int r1 = (sample_a <<16 >>16);
					r1 = r1 * alpha;

					rr1 = (sample_b <<16 >>16);
					r1 += (rr1 * (0x8000-alpha));

					if(r1>(ClippingMax<<15)) r1=ClippingMax<<15;
					else	if(r1<(ClippingMin<<15)) r1=ClippingMin<<15;
		
					*((INT16*)dst) = (((r1 >> (15+8))+ 128) & 0xff) | (((r1 >> 15)+ (128<<8)) & 0xff00);
					dst = (INT16*)dst+1;
		STRETCH_E(INT16, INT16, samples, wsre)
		break;
	case 7:					//st16_to_st8
		STRETCH_S(INT16, INT32, samples, wsre)
					int rr1,rr2;
					int r1 = (sample_a <<16 >>16);
					int r2 = (sample_a >> 16);
					r1 = r1 * alpha;
					r2 = r2 * alpha;

					rr1 = (sample_b <<16 >>16);
					rr2 = (sample_b >> 16);
					r1 += (rr1 * (0x8000-alpha));
					r2 += (rr2 * (0x8000-alpha));

					if(r1>(ClippingMax<<15)) r1=ClippingMax<<15;
					else	if(r1<(ClippingMin<<15)) r1=ClippingMin<<15;
					if(r2>(ClippingMax<<15)) r2=ClippingMax<<15;
					else	if(r2<(ClippingMin<<15)) r2=ClippingMin<<15;

					*((INT16*)dst) = (((r1 >> (15+8))+ 128) & 0xff) | (((r2 >> 15)+ (128<<8)) & 0xff00);
					dst = (INT16*)dst+1;
		STRETCH_E(INT16, INT32, samples, wsre)
		break;

	case 8:
		break;
	case 9:
		break;
	case 10:				//mo16_to_mo16
		STRETCH_S(INT16, INT16, samples, wsre)
					int rr1;
					int r1 = (sample_a <<16 >>16);
					r1 = r1 * alpha;

					rr1 = (sample_b <<16 >>16);
					r1 += (rr1 * (0x8000-alpha));

					if(r1>(ClippingMax<<15)) r1=ClippingMax<<15;
					else	if(r1<(ClippingMin<<15)) r1=ClippingMin<<15;

					*((INT16*)dst) = (r1>>15) & 0xffff;
					dst = (INT16*)dst+1;
		STRETCH_E(INT16, INT16, samples, wsre)
		break;
	case 11:				//st16_to_mo16
		STRETCH_S(INT16, INT32, samples, wsre)
					int rr1,rr2;
					int r1 = (sample_a <<16 >>16);
					int r2 = (sample_a >> 16);
					r1 = r1 * alpha;
					r2 = r2 * alpha;

					rr1 = (sample_b <<16 >>16);
					rr2 = (sample_b >> 16);
					r1 += (rr1 * (0x8000-alpha));
					r2 += (rr2 * (0x8000-alpha));

					if(r1>(ClippingMax<<15)) r1=ClippingMax<<15;
					else	if(r1<(ClippingMin<<15)) r1=ClippingMin<<15;
					if(r2>(ClippingMax<<15)) r2=ClippingMax<<15;
					else	if(r2<(ClippingMin<<15)) r2=ClippingMin<<15;
					
					*((INT16*)dst) = (INT16)(((r1>>15) + (r2>>15))>>1);
					dst = (INT16*)dst+1;
		STRETCH_E(INT16, INT32, samples, wsre)
		break;
	case 12:
		break;
	case 13:
		break;
	case 14:				//mo16_to_st16
		STRETCH_S(INT32, INT16, samples, wsre)
					int rr1;
					int r1 = (sample_a <<16 >>16);
					r1 = r1 * alpha;

					rr1 = (sample_b <<16 >>16);
					r1 += (rr1 * (0x8000-alpha));

					if(r1>(ClippingMax<<15)) r1=ClippingMax<<15;
					else	if(r1<(ClippingMin<<15)) r1=ClippingMin<<15;
		
					*((INT32*)dst) = ((r1>>15) & 0xffff) | ((r1<<1) & 0xffff0000);
					dst = (INT32*)dst+1;
		STRETCH_E(INT32, INT16, samples, wsre)
		break;
	case 15:				//st16_to_st16
		STRETCH_S(INT32, INT32, samples, wsre)
					int rr1,rr2;
					int r1 = (sample_a <<16 >>16);
					int r2 = (sample_a >> 16);
					r1 = r1 * alpha;
					r2 = r2 * alpha;

					rr1 = (sample_b <<16 >>16);
					rr2 = (sample_b >> 16);
					r1 += (rr1 * (0x8000-alpha));
					r2 += (rr2 * (0x8000-alpha));

					if(r1>(ClippingMax<<15)) r1=ClippingMax<<15;
					else	if(r1<(ClippingMin<<15)) r1=ClippingMin<<15;
					if(r2>(ClippingMax<<15)) r2=ClippingMax<<15;
					else	if(r2<(ClippingMin<<15)) r2=ClippingMin<<15;

					*((INT32*)dst) = ((r1>>15) & 0xffff) | ((r2<<1)&0xffff0000);
					dst = (INT32*)dst+1;
		STRETCH_E(INT32, INT32, samples, wsre)
		break;
	}

	return samples;
}




FPTR wav_wavecnv_stretch_f(int cnvnmb, void* dst, void* src, FPTR samples, struct WAV_SAMPLES_RESAMPLE *wsre)
{
	const FPTR destpos = (FPTR)dst;


	switch (cnvnmb & 0xf)
	{
	case 0:
		break;
	case 1:
		break;
	case 2:					//mo16_to_mo8
		STRETCH_S_F(INT8, INT16, samples, wsre)
		STRETCH_ALPHA_MONO_F
					*((UINT8*)dst) = ((r1 >> 8) + 128) & 0xff;
					dst = (INT8*)dst+1;
		STRETCH_E(INT8, INT16, samples, wsre)
		break;
	case 3:					//st16_to_mo8
		STRETCH_S_F(INT8, INT32, samples, wsre)
		STRETCH_ALPHA_STEREO_F
					*((UINT8*)dst) = (((r1 + r2)>>(1+8)) + 128) & 0xff;
					dst = (INT8*)dst+1;
		STRETCH_E(INT8, INT32, samples, wsre)
		break;
	case 4:
		break;
	case 5:
		break;
	case 6:					//mo16_to_st8
		STRETCH_S_F(INT16, INT16, samples, wsre)
		STRETCH_ALPHA_MONO_F
					*((INT16*)dst) = (((r1 >> 8)+ 128) & 0xff) | ((r1 + (128<<8)) & 0xff00);
					dst = (INT16*)dst+1;
		STRETCH_E(INT16, INT16, samples, wsre)
		break;
	case 7:					//st16_to_st8
		STRETCH_S_F(INT16, INT32, samples, wsre)
		STRETCH_ALPHA_STEREO_F
					*((INT16*)dst) = (((r1 >> 8)+ 128) & 0xff) | ((r2 + (128<<8)) & 0xff00);
					dst = (INT16*)dst+1;
		STRETCH_E(INT16, INT32, samples, wsre)
		break;

	case 8:
		break;
	case 9:
		break;
	case 10:				//mo16_to_mo16
		STRETCH_S_F(INT16, INT16, samples, wsre)
		STRETCH_ALPHA_MONO_F
					*((INT16*)dst) = r1 & 0xffff;
					dst = (INT16*)dst+1;
		STRETCH_E(INT16, INT16, samples, wsre)
		break;
	case 11:				//st16_to_mo16
		STRETCH_S_F(INT16, INT32, samples, wsre)
		STRETCH_ALPHA_STEREO_F
					*((INT16*)dst) = (INT16)((r1 + r2)>>1);
					dst = (INT16*)dst+1;
		STRETCH_E(INT16, INT32, samples, wsre)
		break;
	case 12:
		break;
	case 13:
		break;
	case 14:				//mo16_to_st16
		STRETCH_S_F(INT32, INT16, samples, wsre)
		STRETCH_ALPHA_MONO_F
					*((INT32*)dst) = (r1 & 0xffff) | (r1 & 0xffff0000);
					dst = (INT32*)dst+1;
		STRETCH_E(INT32, INT16, samples, wsre)
		break;
	case 15:				//st16_to_st16
		STRETCH_S_F(INT32, INT32, samples, wsre)
		STRETCH_ALPHA_STEREO_F
					*((INT32*)dst) = (r1 & 0xffff) | ((r2<<16) & 0xffff0000);
					dst = (INT32*)dst+1;
		STRETCH_E(INT32, INT32, samples, wsre)

		break;
	}

	return samples;
}
//		resize
