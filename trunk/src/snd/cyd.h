#ifndef CYD_H
#define CYD_H

/*
Copyright (c) 2009-2010 Tero Lindeman (kometbomb)

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include <signal.h>
#include "cydtypes.h"
#include "cydflt.h"
#include "cydfx.h"
#include "cydentry.h"
#include "cyddefs.h"

#ifdef LOWRESWAVETABLE
typedef Uint32 CydWaveAcc;
#else
typedef Uint64 CydWaveAcc;
#endif

typedef struct
{
	Uint8 a, d, s , r; // 0-15
} CydAdsr;

typedef struct
{
	// ---- interface
	volatile Uint32 flags;
	volatile Uint16 pw; // 0-2047
	Uint8 sync_source, ring_mod; // channel
	Uint8 flttype;
	CydAdsr adsr;
	Uint8 ym_env_shape;
	volatile Uint8 volume; // 0-127
#ifdef STEREOOUTPUT
	volatile Uint8 panning; // 0-128, 64 = center
	volatile Sint32 gain_left, gain_right;
#endif
	// ---- internal
	Uint32 sync_bit;
	volatile Uint32 frequency;
	Uint32 accumulator;
	Uint32 random; // random lfsr
	Uint32 lfsr, lfsr_ctr, lfsr_type, lfsr_period; // lfsr state
	volatile Uint32 envelope, env_speed;
	volatile Uint8 envelope_state;
	CydFilter flt;
	int fx_bus;
	const CydWavetableEntry *wave_entry;
	int wave_direction; // 0 = forward, 1 = backwards
	CydWaveAcc wave_acc; // probably overkill
	Uint32 wave_frequency;
} CydChannel;

enum
{
	FLT_LP,
	FLT_HP,
	FLT_BP,
	FLT_TYPES
};

enum
{
	CYD_CHN_ENABLE_NOISE = 1,
	CYD_CHN_ENABLE_PULSE = 2,
	CYD_CHN_ENABLE_TRIANGLE = 4,
	CYD_CHN_ENABLE_SAW = 8,
	CYD_CHN_ENABLE_SYNC = 16,
	CYD_CHN_ENABLE_GATE = 32,
	CYD_CHN_ENABLE_KEY_SYNC = 64,
	CYD_CHN_ENABLE_METAL = 128,
	CYD_CHN_ENABLE_RING_MODULATION = 256,
	CYD_CHN_ENABLE_FILTER = 512,
	CYD_CHN_ENABLE_FX = 1024,
	CYD_CHN_ENABLE_YM_ENV = 2048,
	CYD_CHN_ENABLE_WAVE = 4096,
	CYD_CHN_WAVE_OVERRIDE_ENV = 8192,
	CYD_CHN_ENABLE_LFSR = 16384
};

enum {
	ATTACK,
	DECAY,
	SUSTAIN,
	RELEASE,
	DONE
};

enum
{
	CYD_LOCK_REQUEST=1,
	CYD_LOCK_LOCKED=2,
	CYD_LOCK_CALLBACK=4
	
};

#define WAVEFORMS (CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_PULSE|CYD_CHN_ENABLE_TRIANGLE|CYD_CHN_ENABLE_SAW|CYD_CHN_ENABLE_WAVE|CYD_CHN_ENABLE_LFSR)

#define LUT_SIZE 1024
#define YM_LUT_SIZE 16

#define CYD_NUM_WO_BUFFER_SIZE 2000
#define CYD_NUM_WO_BUFFERS 4

typedef struct
{
	CydChannel *channel;
	int n_channels;
	Uint32 sample_rate;
	// ----- internal
	volatile Uint32 flags;
	int (*callback)(void*);
	void *callback_parameter;
	volatile Uint32 callback_period, callback_counter;
	Uint16 *lookup_table, *lookup_table_ym;
	CydFx fx[CYD_MAX_FX_CHANNELS];
#ifdef USESDLMUTEXES
	CydMutex mutex;	
#else
	volatile sig_atomic_t lock_request;
	volatile sig_atomic_t lock_locked;
#endif
#ifdef ENABLEAUDIODUMP
	FILE *dump;
#endif
	size_t samples_output; // bytes in last cyd_output_buffer
	CydWavetableEntry *wavetable_entries;
#ifdef USENATIVEAPIS
# ifdef WIN32
	BOOL thread_running;
	DWORD thread_handle, cb_handle;
	CRITICAL_SECTION thread_lock;
	int buffers_available;
	int waveout_hdr_idx;
	HWAVEOUT hWaveOut;
	WAVEHDR waveout_hdr[CYD_NUM_WO_BUFFERS]; 
# endif
#endif
	Uint32 lfsr_ctr, lfsr_ctr2; // "pokey" lfsr update counter
	Uint32 reg4, reg5, reg9, reg17; // "pokey" lfsr registers
} CydEngine;

enum
{
	CYD_PAUSED = 1,
	CYD_CLIPPING = 2,
	CYD_SINGLE_THREAD = 8,
};

// YM2149 envelope shape flags, CONT is assumed to be always set

enum { CYD_YM_ENV_ATT = 1, CYD_YM_ENV_ALT = 2};

/////////////////777

void cyd_init(CydEngine *cyd, Uint16 sample_rate, int initial_channels);
void cyd_reserve_channels(CydEngine *cyd, int channels);
void cyd_deinit(CydEngine *cyd);
void cyd_reset(CydEngine *cyd);
void cyd_set_frequency(CydEngine *cyd, CydChannel *chn, Uint16 frequency);
void cyd_set_wavetable_frequency(CydEngine *cyd, CydChannel *chn, Uint16 frequency);
void cyd_reset_wavetable(CydEngine *cyd);
void cyd_set_wavetable_offset(CydChannel *chn, Uint16 offset /* 0..0x1000 = 0-100% */);
void cyd_set_env_frequency(CydEngine *cyd, CydChannel *chn, Uint16 frequency);
void cyd_set_env_shape(CydChannel *chn, Uint8 shape);
void cyd_enable_gate(CydEngine *cyd, CydChannel *chn, Uint8 enable);
void cyd_set_waveform(CydChannel *chn, Uint32 wave);
void cyd_set_wave_entry(CydChannel *chn, const CydWavetableEntry * entry);
void cyd_set_filter_coeffs(CydEngine * cyd, CydChannel *chn, Uint16 cutoff, Uint8 resonance);
void cyd_pause(CydEngine *cyd, Uint8 enable);
void cyd_set_callback(CydEngine *cyd, int (*callback)(void*), void*param, Uint16 period);
void cyd_set_callback_rate(CydEngine *cyd, Uint16 period);
int cyd_register(CydEngine * cyd);
int cyd_unregister(CydEngine * cyd);
void cyd_lock(CydEngine *cyd, Uint8 enable);
#ifdef ENABLEAUDIODUMP
void cyd_enable_audio_dump(CydEngine *cyd);
void cyd_disable_audio_dump(CydEngine *cyd);
#endif
#ifdef STEREOOUTPUT
void cyd_set_panning(CydEngine *cyd, CydChannel *chn, Uint8 panning);
#endif

void cyd_output_buffer_stereo(int chan, void *_stream, int len, void *udata);
Sint32 cyd_env_output(CydEngine *cyd, CydChannel *chn, Sint32 input);

#endif
