#ifndef CYD_H
#define CYD_H

/*
Copyright (c) 2009 Tero Lindeman (kometbomb)

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


#include "SDL.h"

#define USESDLMUTEXES

#include <signal.h>

#include "cydflt.h"
#include "cydrvb.h"

#define CYD_BASE_FREQ 22050
#define CYD_MAX_CHANNELS 32

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
	volatile Uint8 volume; // 0-127
	// ---- internal
	Uint32 sync_bit;
	volatile Uint32 frequency;
	Uint32 accumulator;
	Uint32 random;
	volatile Uint32 envelope, env_speed;
	volatile Uint8 envelope_state;
	CydFilter flt;
} CydChannel;

enum
{
	FLT_LP,
	FLT_HP,
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
	CYD_CHN_ENABLE_REVERB = 1024
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

#define WAVEFORMS (CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_PULSE|CYD_CHN_ENABLE_TRIANGLE|CYD_CHN_ENABLE_SAW)

#define LUT_SIZE 1024

typedef struct
{
	CydChannel *channel;
	int n_channels;
	Uint32 sample_rate;
	// ----- internal
	volatile Uint32 flags;
	void (*callback)(void*);
	void *callback_parameter;
	volatile Uint32 callback_period, callback_counter;
	Uint16 *lookup_table;
	CydFilter flt;
	CydReverb rvb;
#ifdef USESDLMUTEXES
	SDL_mutex *mutex;	
#else
	volatile sig_atomic_t lock_request;
	volatile sig_atomic_t lock_locked;
#endif
} CydEngine;

enum
{
	CYD_PAUSED = 1,
	CYD_ENABLE_REVERB = 2
};

/////////////////777

void cyd_init(CydEngine *cyd, Uint16 sample_rate, int channels);
void cyd_deinit(CydEngine *cyd);
void cyd_reset(CydEngine *cyd);
void cyd_set_frequency(CydEngine *cyd, CydChannel *chn, Uint16 frequency);
void cyd_enable_gate(CydEngine *cyd, CydChannel *chn, Uint8 enable);
void cyd_set_waveform(CydChannel *chn, Uint32 wave);
void cyd_set_filter_coeffs(CydEngine * cyd, CydChannel *chn, Uint16 cutoff, Uint8 resonance);
void cyd_pause(CydEngine *cyd, Uint8 enable);
void cyd_set_callback(CydEngine *cyd, void (*callback)(void*), void*param, Uint16 period);
int cyd_register(CydEngine * cyd);
int cyd_unregister(CydEngine * cyd);
void cyd_lock(CydEngine *cyd, Uint8 enable);


#endif