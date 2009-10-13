#ifndef MUSIC_H
#define MUSIC_H

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


#include "cyd.h"

#define MUS_PROG_LEN 32
#define MUS_CHANNELS 4

#define MUS_VERSION 5

#define MUS_TITLE_LEN 16

typedef struct
{
	Uint32 flags;
	Uint32 cydflags;
	CydAdsr adsr;
	Uint8 sync_source, ring_mod; // 0xff == self
	Uint16 pw;
	Uint8 volume;
	Uint16 program[MUS_PROG_LEN];
	Uint8 prog_period; 
	Uint8 vibrato_speed, vibrato_depth, slide_speed, pwm_speed, pwm_depth;
	Uint8 base_note;
	Uint16 cutoff;
	Uint8 resonance;
	Uint8 flttype;
	char name[16];
} MusInstrument;

enum
{
	MUS_INST_PROG_SPEED_RELATIVE = 0, // chn.current_tick / mus.tick_period * ins.prog_period
	MUS_INST_PROG_SPEED_ABSOLUTE = 1, // absolute number of ticks
	MUS_INST_DRUM = 2,
	MUS_INST_INVERT_VIBRATO_BIT = 4,
	MUS_INST_LOCK_NOTE = 8,
	MUS_INST_SET_PW = 16,
	MUS_INST_SET_CUTOFF = 32
};

typedef struct
{
	MusInstrument *instrument;
	Uint16 note;
	// ------
	Uint8 arpeggio_note;
	Uint16 target_note;
	volatile Uint32 flags;
	Uint32 current_tick;
	Uint8 program_counter, program_tick, program_loop;
} MusChannel;

typedef struct
{
	Uint8 note, instrument, ctrl;
	Uint16 command;
} MusStep;

typedef struct
{
	Uint16 position; 
	Uint16 pattern;
	Sint8 note_offset;
} MusSeqPattern;

typedef struct
{
	MusStep *step;
	Uint16 num_steps;
} MusPattern;

typedef struct
{
	MusInstrument *instrument;
	Uint8 num_instruments;
	MusPattern *pattern;
	Uint16 num_patterns;
	MusSeqPattern *sequence[MUS_CHANNELS];
	Uint16 num_sequences[MUS_CHANNELS];
	Uint16 song_length, loop_point;
	Uint8 song_speed, song_speed2, song_rate;
	Uint16 time_signature;
	Uint32 flags;
	char title[MUS_TITLE_LEN + 1];
	struct { int delay, gain; } rvbtap[CYDRVB_TAPS];
} MusSong;


typedef struct
{
	int channel;
	MusInstrument *instrument;
	Uint8 note;
} MusDelayedTrigger;

typedef struct
{
	MusDelayedTrigger delayed;
	MusPattern *pattern;
	Uint8 last_ctrl;
	Uint16 pw, pattern_step, sequence_position, slide_speed;
	Uint16 vibrato_position, pwm_position;
	Sint8 note_offset;
	Uint16 filter_cutoff;
	Uint8 extarp1, extarp2;
	Uint8 volume;
} MusTrackStatus;

typedef struct
{
	MusChannel channel[CYD_MAX_CHANNELS];
	Uint8 tick_period; // 1 = at the rate this is polled
	// ----
	MusTrackStatus song_track[CYD_MAX_CHANNELS];
	MusSong *song;
	Uint8 song_counter;
	Uint16 song_position;
	CydEngine *cyd;
	Uint8 current_tick;
	Uint8 volume; // 0..128
} MusEngine;


enum
{
	MUS_CHN_PLAYING = 1,
	MUS_CHN_PROGRAM_RUNNING = 2,
	MUS_CHN_DISABLED = 4
};

enum
{
	MUS_NOTE_NONE = 0xff,
	MUS_NOTE_RELEASE = 0xfe
};

#define MIDDLE_C (12*4)
#define MUS_NOTE_NO_INSTRUMENT 0xff
#define MUS_CTRL_BIT 1
#define MAX_VOLUME 128

enum
{
	MUS_FX_ARPEGGIO = 0x0000,
	MUS_FX_SET_EXT_ARP = 0x1000,
	MUS_FX_PORTA_UP = 0x0100,
	MUS_FX_PORTA_DN = 0x0200,
	MUS_FX_SLIDE = 0x0300,
	MUS_FX_VIBRATO = 0x0400,
	MUS_FX_FADE_VOLUME = 0x0a00,
	MUS_FX_SET_VOLUME = 0x0c00,
	MUS_FX_EXT = 0x0e00,
	MUS_FX_EXT_PORTA_UP = 0x0e10,
	MUS_FX_EXT_PORTA_DN = 0x0e20,
	MUS_FX_EXT_FADE_VOLUME_DN = 0x0ea0,
	MUS_FX_EXT_FADE_VOLUME_UP = 0x0eb0,
	MUS_FX_EXT_NOTE_CUT = 0x0ec0,
	MUS_FX_PORTA_UP_SEMI = 0x1100,
	MUS_FX_PORTA_DN_SEMI = 0x1200,
	MUS_FX_CUTOFF_UP = 0x2100,
	MUS_FX_CUTOFF_DN = 0x2200,
	MUS_FX_CUTOFF_SET = 0x2900,
	MUS_FX_PW_DN = 0x0700,
	MUS_FX_PW_UP = 0x0800,
	MUS_FX_PW_SET = 0x0900,
	MUS_FX_SET_WAVEFORM = 0x0b00,
	MUS_FX_END = 0xffff,
	MUS_FX_JUMP = 0xff00,
	MUS_FX_LABEL = 0xfd00,
	MUS_FX_LOOP = 0xfe00,
	MUS_FX_TRIGGER_RELEASE = 0xfc00,
	MUS_FX_NOP = 0xfffe
};

enum
{
	MUS_CTRL_LEGATO = MUS_CTRL_BIT,
	MUS_CTRL_SLIDE = MUS_CTRL_BIT << 1,
	MUS_CTRL_VIB = MUS_CTRL_BIT << 2
	
};

enum
{
	MUS_ENABLE_REVERB = 1
};

#define MUS_INST_SIG "cyd!inst"
#define MUS_SONG_SIG "cyd!song"

void mus_advance_tick(void* udata);
int mus_trigger_instrument(MusEngine* mus, int chan, MusInstrument *ins, Uint8 note);
void mus_release(MusEngine* mus, int chan);
void mus_init_engine(MusEngine *mus, CydEngine *cyd);
void mus_set_song(MusEngine *mus, MusSong *song, Uint16 position);
int mus_poll_status(MusEngine *mus, int *song_position, int *pattern_position, MusPattern **pattern, MusChannel *);
void mus_load_instrument_file(Uint8 version, FILE *f, MusInstrument *inst);
void mus_load_instrument_file2(FILE *f, MusInstrument *inst);
void mus_load_instrument(const char *path, MusInstrument *inst);
void mus_get_default_instrument(MusInstrument *inst);
void mus_load_song(const char *path, MusSong *song);
void mus_load_song_file(FILE *f, MusSong *song);
void mus_free_song(MusSong *song);
void mus_set_reverb(MusEngine *mus, MusSong *song);

#endif
