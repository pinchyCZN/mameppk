
/***************************************************************************
  Original Source from EmeraldMame 185
***************************************************************************/

#ifndef EMEMAME32WAV_H
#define EMEMAME32WAV_H

struct WAV_WAVEFORMAT
{
	UINT32 channel, bitrate, samplespersec;
};

struct WAV_SAMPLES_RESIZE
{
	FPTR pos, adder;
	FPTR samples;	//test
};

struct WAV_SAMPLES_RESAMPLE
{
	FPTR pos_start, pos, pos_end;
	FPTR adder;
	FPTR count;
	FPTR next;
	void *buf;
};

int wav_wavefile_create(const void* fname, UINT32 samplerate, int bitrate, int channel );
void wav_wavefile_close(int file);

struct WAV_WAVEFORMAT wav_wavefile_getformat(const void* fname);

int wav_start_log_wave(const void* fname, struct WAV_WAVEFORMAT *wfm );
int wav_append_log_wave(void);
void wav_stop_log_wave(void);
void wav_log_wave(void* buf, unsigned int len);

void wav_wavecnv_set_clipping(int min, int max);

int wav_convert_select(struct WAV_WAVEFORMAT *dstwfm, struct WAV_WAVEFORMAT *srcwfm, struct WAV_SAMPLES_RESIZE *wsr, struct WAV_SAMPLES_RESAMPLE *wsre);
void wav_wavecnv(int cnvnmb, void* dst, void* src, FPTR samples);
FPTR wav_wavecnv_resize(int cnvnmb, void* dst, void* src, FPTR samples, struct WAV_SAMPLES_RESIZE *wsr);
FPTR wav_wavecnv_resample(int cnvnmb, void* dst, void* src, FPTR samples, struct WAV_SAMPLES_RESAMPLE *wsre);
FPTR wav_wavecnv_resample_f(int cnvnmb, void* dst, void* src, FPTR samples, struct WAV_SAMPLES_RESAMPLE *wsre);
FPTR wav_wavecnv_stretch(int cnvnmb, void* dst, void* src, FPTR samples, struct WAV_SAMPLES_RESAMPLE *wsre);
FPTR wav_wavecnv_stretch_f(int cnvnmb, void* dst, void* src, FPTR samples, struct WAV_SAMPLES_RESAMPLE *wsre);

#endif /* EMEMAME32WAV_H */
