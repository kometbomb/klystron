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


#include "music.h"
#include <assert.h>
#include <stdio.h>
#include "freqs.h"
#include "macros.h"

#define VIB_TAB_SIZE 128

static int mus_trigger_instrument_internal(MusEngine* mus, int chan, MusInstrument *ins, Uint8 note);


static void update_volumes(MusEngine *mus, MusTrackStatus *ts, MusChannel *chn, CydChannel *cydchn, int volume)
{
	if (chn->instrument && (chn->instrument->flags & MUS_INST_RELATIVE_VOLUME))
	{
		ts->volume = volume;
		cydchn->volume = (chn->flags & MUS_CHN_DISABLED) ? 0 : (int)chn->instrument->volume * volume / MAX_VOLUME * (int)mus->volume / MAX_VOLUME;
	}
	else
	{
		ts->volume = volume;
		cydchn->volume = (chn->flags & MUS_CHN_DISABLED) ? 0 : ts->volume * (int)mus->volume / MAX_VOLUME;
	}
}


static void mus_set_buzz_frequency(MusEngine *mus, int chan, Uint16 note)
{
	MusChannel *chn = &mus->channel[chan];
	if (chn->instrument->flags & MUS_INST_YM_BUZZ)
	{
		Uint16 buzz_frequency = get_freq(note + chn->buzz_offset);
		cyd_set_env_frequency(mus->cyd, &mus->cyd->channel[chan], buzz_frequency);
	}
}


static void mus_set_wavetable_frequency(MusEngine *mus, int chan, Uint16 note)
{
	MusChannel *chn = &mus->channel[chan];
	CydChannel *cydchn = &mus->cyd->channel[chan];
	
	if ((chn->instrument->cydflags & CYD_CHN_ENABLE_WAVE) && (cydchn->wave_entry))
	{
		Uint16 wave_frequency = get_freq(note);
		cyd_set_wavetable_frequency(mus->cyd, cydchn, wave_frequency);
	}
}


static void mus_set_note(MusEngine *mus, int chan, Uint16 note, int update_note, int divider)
{
	MusChannel *chn = &mus->channel[chan];
	
	if (update_note) chn->note = note;
	
	Uint16 frequency = get_freq(note);
		
	cyd_set_frequency(mus->cyd, &mus->cyd->channel[chan], frequency / divider);
	
	mus_set_wavetable_frequency(mus, chan, note);
	
	mus_set_buzz_frequency(mus, chan, note);
}


static void mus_set_slide(MusEngine *mus, int chan, Uint16 note)
{
	MusChannel *chn = &mus->channel[chan];
	chn->target_note = note;
	//if (update_note) chn->note = note;
}


void mus_init_engine(MusEngine *mus, CydEngine *cyd)
{
	memset(mus, 0, sizeof(*mus));
	mus->cyd = cyd;
	mus->volume = MAX_VOLUME;
}


static void do_command(MusEngine *mus, int chan, int tick, Uint16 inst, int from_program)
{
	MusChannel *chn = &mus->channel[chan];
	CydChannel *cydchn = &mus->cyd->channel[chan];
	
	switch (inst & 0x7f00)
	{
		case MUS_FX_PORTA_UP:
		{
			Uint16 prev = chn->note;
			chn->note += ((inst & 0xff) << 2);
			if (prev > chn->note) chn->note = 0xffff;
			
			mus_set_slide(mus, chan, chn->note);
		}
		break;
		
		case MUS_FX_PORTA_DN:
		{
			Uint16 prev = chn->note;
			chn->note -= ((inst & 0xff) << 2);
			
			if (prev < chn->note) chn->note = 0x0;
			
			mus_set_slide(mus, chan, chn->note);
		}
		break;
		
		case MUS_FX_PW_DN:
		{
			mus->song_track[chan].pw -= inst & 0xff;
			if (mus->song_track[chan].pw > 0xf000) mus->song_track[chan].pw = 0;
		}
		break;
		
		case MUS_FX_PW_UP:
		{
			mus->song_track[chan].pw += inst & 0xff;
			if (mus->song_track[chan].pw > 0x7ff) mus->song_track[chan].pw = 0x7ff;
		}
		break;
		
		case MUS_FX_CUTOFF_DN:
		{
			mus->song_track[chan].filter_cutoff -= inst & 0xff;
			if (mus->song_track[chan].filter_cutoff > 0xf000) mus->song_track[chan].filter_cutoff = 0;
			cyd_set_filter_coeffs(mus->cyd, cydchn, mus->song_track[chan].filter_cutoff, 0);
		}
		break;
		
		case MUS_FX_CUTOFF_UP:
		{
			mus->song_track[chan].filter_cutoff += inst & 0xff;
			if (mus->song_track[chan].filter_cutoff > 0x7ff) mus->song_track[chan].filter_cutoff = 0x7ff;
			cyd_set_filter_coeffs(mus->cyd, cydchn, mus->song_track[chan].filter_cutoff, 0);
		}
		break;
		
		case MUS_FX_BUZZ_DN:
		{
			if (chn->buzz_offset >= -32768 + (inst & 0xff))
				chn->buzz_offset -= inst & 0xff;
				
			mus_set_buzz_frequency(mus, chan, chn->note);
		}
		break;
		
		case MUS_FX_BUZZ_UP:
		{
			if (chn->buzz_offset <= 32767 - (inst & 0xff))
				chn->buzz_offset += inst & 0xff;
				
			mus_set_buzz_frequency(mus, chan, chn->note);
		}
		break;
		
		case MUS_FX_TRIGGER_RELEASE:
		{
			if (tick == (inst & 0xff)) 
				cyd_enable_gate(mus->cyd, cydchn, 0);
		}
		break;
		
		case MUS_FX_FADE_VOLUME:
		{
			if (!(chn->flags & MUS_CHN_DISABLED))
			{
				mus->song_track[chan].volume -= inst & 0xf;
				if (mus->song_track[chan].volume > MAX_VOLUME) mus->song_track[chan].volume = 0;
				mus->song_track[chan].volume += (inst >> 4) & 0xf;
				if (mus->song_track[chan].volume > MAX_VOLUME) mus->song_track[chan].volume = MAX_VOLUME;
				
				update_volumes(mus, &mus->song_track[chan], chn, cydchn, mus->song_track[chan].volume);
			}
		}
		break;
		
		case MUS_FX_PAN_RIGHT:
		case MUS_FX_PAN_LEFT:
		{
			int p = cydchn->panning;
			if ((inst & 0xff00) == MUS_FX_PAN_LEFT) 
			{
				p -= inst & 0x00ff;
			}
			else
			{
				p += inst & 0x00ff;
			}
			
			p = my_min(CYD_PAN_RIGHT, my_max(CYD_PAN_LEFT, p));
			
			cyd_set_panning(mus->cyd, cydchn, p);
		}
		break;
		
		case MUS_FX_EXT:
		{
			// Protracker style Exy commands
		
			switch (inst & 0xfff0)
			{
				case MUS_FX_EXT_NOTE_CUT:
				{
					if (!(chn->flags & MUS_CHN_DISABLED))
					{
						if ((inst & 0xf) <= tick)
						{
							cydchn->volume = 0;
							mus->song_track[chan].volume = 0;
						}
					}
				}
				break;
				
				case MUS_FX_EXT_RETRIGGER:
				{
					if ((inst & 0xf) > 0 && (tick % (inst & 0xf)) == 0)
					{
						Uint8 prev_vol_tr = mus->song_track[chan].volume;
						Uint8 prev_vol_cyd = cydchn->volume;
						mus_trigger_instrument_internal(mus, chan, chn->instrument, chn->last_note);
						mus->song_track[chan].volume = prev_vol_tr;
						cydchn->volume = prev_vol_cyd;
					}
				}
				break;
			}
		}
		break;
	}
	
	if (tick == 0) 
	{
		// --- commands that run only on tick 0
		
		switch (inst & 0xff00)
		{
			case MUS_FX_EXT:
			{
				// Protracker style Exy commands
			
				switch (inst & 0xfff0)
				{
					case MUS_FX_EXT_FADE_VOLUME_DN:
					{
						if (!(chn->flags & MUS_CHN_DISABLED))
						{
							mus->song_track[chan].volume -= inst & 0xf;
							if (mus->song_track[chan].volume > MAX_VOLUME) mus->song_track[chan].volume = 0;
							
							update_volumes(mus, &mus->song_track[chan], chn, cydchn, mus->song_track[chan].volume);
						}
					}
					break;
					
					case MUS_FX_EXT_FADE_VOLUME_UP:
					{
						if (!(chn->flags & MUS_CHN_DISABLED))
						{
							mus->song_track[chan].volume += inst & 0xf;
							if (mus->song_track[chan].volume > MAX_VOLUME) mus->song_track[chan].volume = MAX_VOLUME;
							
							update_volumes(mus, &mus->song_track[chan], chn, cydchn, mus->song_track[chan].volume);
						}
					}
					break;
					
					case MUS_FX_EXT_PORTA_UP:
					{
						Uint16 prev = chn->note;
						chn->note += ((inst & 0x0f));
						
						if (prev > chn->note) chn->note = 0xffff;
						
						mus_set_slide(mus, chan, chn->note);
					}
					break;
					
					case MUS_FX_EXT_PORTA_DN:
					{
						Uint16 prev = chn->note;
						chn->note -= ((inst & 0x0f));
						
						if (prev < chn->note) chn->note = 0x0;
						
						mus_set_slide(mus, chan, chn->note);
					}
					break;
				}
			}
			break;
			
			default:
			
			switch (inst & 0xf000)
			{
				case MUS_FX_CUTOFF_FINE_SET:
				{
					mus->song_track[chan].filter_cutoff = (inst & 0xfff);
					if (mus->song_track[chan].filter_cutoff > 0x7ff) mus->song_track[chan].filter_cutoff = 0x7ff;
					cyd_set_filter_coeffs(mus->cyd, cydchn, mus->song_track[chan].filter_cutoff, chn->instrument->resonance);
				}
				break;
			}
			
			switch (inst & 0x7f00)
			{
				case MUS_FX_PW_SET:
				{
					mus->song_track[chan].pw = (inst & 0xff) << 4;
				}
				break;
				
				case MUS_FX_BUZZ_SHAPE:
				{
					cyd_set_env_shape(cydchn, inst & 3);
				}
				break;
				
				case MUS_FX_BUZZ_SET_SEMI:
				{
					chn->buzz_offset = (((inst & 0xff)) - 0x80) << 8;
						
					mus_set_buzz_frequency(mus, chan, chn->note);
				}
				break;
			
				case MUS_FX_BUZZ_SET:
				{
					chn->buzz_offset = ((inst & 0xff)) - 0x80;
						
					mus_set_buzz_frequency(mus, chan, chn->note);
				}
				break;
				
				case MUS_FX_SET_PANNING:
				{
					cyd_set_panning(mus->cyd, cydchn, inst & 0xff);
				}
				break;
				
				case MUS_FX_FILTER_TYPE:
				{
					cydchn->flttype = (inst & 0xf) % FLT_TYPES;
				}
				break;
			
				case MUS_FX_CUTOFF_SET:
				{
					mus->song_track[chan].filter_cutoff = (inst & 0xff) << 3;
					if (mus->song_track[chan].filter_cutoff > 0x7ff) mus->song_track[chan].filter_cutoff = 0x7ff;
					cyd_set_filter_coeffs(mus->cyd, cydchn, mus->song_track[chan].filter_cutoff, chn->instrument->resonance);
				}
				break;
				
				case MUS_FX_SET_SPEED:
				{
					if (from_program)
					{
						chn->prog_period = inst & 0xff;
					}
					else
					{
						mus->song->song_speed = inst & 0xf;
						if ((inst & 0xf0) == 0) mus->song->song_speed2 = mus->song->song_speed;
						else mus->song->song_speed2 = (inst >> 4) & 0xf;
					}
				}
				break;
				
				case MUS_FX_SET_RATE:
				{
					mus->song->song_rate = inst & 0xff;
					if (mus->song->song_rate < 1) mus->song->song_rate = 1;
					cyd_set_callback_rate(mus->cyd, mus->song->song_rate);
				}
				break;
			
				case MUS_FX_PORTA_UP_SEMI:
				{
					Uint16 prev = chn->note;
					chn->note += (inst&0xff) << 8;
					if (prev > chn->note || chn->note >= (FREQ_TAB_SIZE << 8)) chn->note = ((FREQ_TAB_SIZE-1) << 8);
					mus_set_slide(mus, chan, chn->note);
				}
				break;
				
				case MUS_FX_PORTA_DN_SEMI:
				{
					Uint16 prev = chn->note;
					chn->note -= (inst&0xff) << 8;
					if (prev < chn->note) chn->note = 0x0;
					mus_set_slide(mus, chan, chn->note);
				}
				break;
				
				case MUS_FX_ARPEGGIO_ABS:
				{	
					chn->arpeggio_note = 0;
					chn->fixed_note = (inst & 0xff) << 8;
				}
				break;
				
				case MUS_FX_ARPEGGIO:
				{
					if (chn->fixed_note != 0xffff)
					{
						chn->note = chn->last_note;
						chn->fixed_note = 0xffff;
					}
					
					if ((inst & 0xff) == 0xf0)
						chn->arpeggio_note = mus->song_track[chan].extarp1;
					else if ((inst & 0xff) == 0xf1)
						chn->arpeggio_note = mus->song_track[chan].extarp2;
					else
						chn->arpeggio_note = inst & 0xff;
				}
				break;
				
				case MUS_FX_SET_VOLUME:
				{
					mus->song_track[chan].volume = my_min(MAX_VOLUME, inst & 0xff);
					
					update_volumes(mus, &mus->song_track[chan], chn, cydchn, mus->song_track[chan].volume);
				}
				break;
				
				case MUS_FX_SET_WAVEFORM:
				{
					cyd_set_waveform(&mus->cyd->channel[chan], inst & 0xff);
				}
				break;
			}
			
			break;
		}
	}
}


static void mus_exec_track_command(MusEngine *mus, int chan)
{
	const Uint16 inst = mus->song_track[chan].pattern->step[mus->song_track[chan].pattern_step].command;
	
	switch (inst & 0xff00)
	{
		case MUS_FX_ARPEGGIO:
			if (!(inst & 0xff)) break; // no params = use the same settings
		case MUS_FX_SET_EXT_ARP:
		{
			mus->song_track[chan].extarp1 = (inst & 0xf0) >> 4;
			mus->song_track[chan].extarp2 = (inst & 0xf);
		}
		break;
		
		default:
		do_command(mus, chan, mus->song_counter, inst, 0);
		break;
	}
	
	
}


static void mus_exec_prog_tick(MusEngine *mus, int chan, int advance)
{
	MusChannel *chn = &mus->channel[chan];
	int tick = chn->program_tick;
	int visited[MUS_PROG_LEN] = { 0 };
	
	do_it_again:;
	
	const Uint16 inst = chn->instrument->program[tick];
	
	switch (inst)
	{	
		case MUS_FX_END:
		{
			chn->flags &= ~MUS_CHN_PROGRAM_RUNNING;
			return;
		}
		break;
	}
	
	if(inst != MUS_FX_NOP)
	{
		switch (inst & 0xff00)
		{
			case MUS_FX_JUMP:
			{
				if (!visited[tick])
				{
					visited[tick] = 1;
					tick = inst & (MUS_PROG_LEN - 1);
				}
				else return;
			}
			break;
			
			case MUS_FX_LABEL:
			{
				
				
			}
			break;
			
			case MUS_FX_LOOP:
			{
				if (chn->program_loop == (inst & 0xff))
				{
					chn->program_loop = 1;
					
				}
				else
				{
					++chn->program_loop;
					while ((chn->instrument->program[tick] & 0xff00) != MUS_FX_LABEL && tick > 0) 
						--tick;
						
					--tick;
				}
			}
			break;
			
			default:
			
			do_command(mus, chan, chn->program_counter, inst, 1);
			
			break;
		}
	}
	
	if (inst == MUS_FX_NOP || (inst & 0xff00) != MUS_FX_JUMP)
	{
		++tick;
		if (tick >= MUS_PROG_LEN)
		{
			tick = 0;
		}
	}
	
	// skip to next on msb
	
	if ((inst & 0x8000) && inst != MUS_FX_NOP)
	{
		goto do_it_again;
	}
	
	if (advance) 
	{
		chn->program_tick = tick;
	}
}


static Sint8 mus_shape(Uint16 position, Uint8 shape)
{
	static const Sint8 rnd_table[VIB_TAB_SIZE] = {
		110, -1, 88, -31, 64,
		-13, 29, -70, -113, 71,
		99, -71, 74, 82, 52,
		-82, -58, 37, 20, -76,
		46, -97, -69, 41, 31,
		-62, -5, 99, -2, -48,
		-89, 17, -19, 4, -27,
		-43, -20, 25, 112, -34,
		78, 26, -56, -54, 72,
		-75, 22, 72, -119, 115,
		56, -66, 25, 87, 93,
		14, 82, 127, 79, -40,
		-100, 21, 17, 17, -116,
		-110, 61, -99, 105, 73,
		116, 53, -9, 105, 91,
		120, -73, 112, -10, 66,
		-10, -30, 99, -67, 60,
		84, 110, 87, -27, -46,
		114, 77, -27, -46, 75,
		-78, 83, -110, 92, -9,
		107, -64, 31, 77, -39,
		115, 126, -7, 121, -2,
		66, 116, -45, 91, 1,
		-96, -27, 17, 76, -82,
		58, -7, 75, -35, 49,
		3, -52, 40
	};
	
	const Sint8 sine_table[VIB_TAB_SIZE] =
	{
		0, 6, 12, 18, 24, 31, 37, 43, 48, 54, 60, 65, 71, 76, 81, 85, 90, 94, 98, 102, 106, 109, 112,		115, 118, 120, 122, 124, 125, 126, 127, 127, 127, 127, 127, 126, 125, 124, 122, 120, 118, 115, 112,		109, 106, 102, 98, 94, 90, 85, 81, 76, 71, 65, 60, 54, 48, 43, 37, 31, 24, 18, 12, 6,		0, -6, -12, -18, -24, -31, -37, -43, -48, -54, -60, -65, -71, -76, -81, -85, -90, -94, -98, -102,		-106, -109, -112, -115, -118, -120, -122, -124, -125, -126, -127, -127, -128, -127, -127, -126, -125, -124, -122,		-120, -118, -115, -112, -109, -106, -102, -98, -94, -90, -85, -81, -76, -71, -65, -60, -54, -48, -43, -37, -31, -24, -18, -12, -6
	};
	
	switch (shape)
	{
		case MUS_SHAPE_SINE:
			return sine_table[position % VIB_TAB_SIZE];
			break;
			
		case MUS_SHAPE_SQUARE:
			return ((position % VIB_TAB_SIZE) & (VIB_TAB_SIZE / 2)) ? -128 : 127;
			break;
			
		case MUS_SHAPE_RAMP_UP:
			return (position % VIB_TAB_SIZE) * 2 - 128;
			break;
			
		case MUS_SHAPE_RAMP_DN:
			return 127 - (position % VIB_TAB_SIZE) * 2;
			break;
			
		default:
		case MUS_SHAPE_RANDOM:
			return rnd_table[(position / 8) % VIB_TAB_SIZE];
			break;
	}
}


static void do_pwm(MusEngine* mus, int chan)
{
	MusChannel *chn = &mus->channel[chan];
	MusInstrument *ins = chn->instrument;

	mus->song_track[chan].pwm_position += ins->pwm_speed;
	mus->cyd->channel[chan].pw = mus->song_track[chan].pw + mus_shape(mus->song_track[chan].pwm_position >> 1, ins->pwm_shape) * ins->pwm_depth / 32;
}


//***** USE THIS INSIDE MUS_ADVANCE_TICK TO AVOID MUTEX DEADLOCK
int mus_trigger_instrument_internal(MusEngine* mus, int chan, MusInstrument *ins, Uint8 note)
{
	if (chan == -1)
	{
		for (int i = 0 ; i < mus->cyd->n_channels ; ++i)
		{
			if (!(mus->cyd->channel[i].flags & CYD_CHN_ENABLE_GATE))
				chan = i;
		}
		
		if (chan == -1)
			chan = (rand() %  mus->cyd->n_channels);
	}
	
	MusChannel *chn = &mus->channel[chan];

	chn->flags = MUS_CHN_PLAYING | (chn->flags & MUS_CHN_DISABLED);
	if (ins->prog_period > 0) chn->flags |= MUS_CHN_PROGRAM_RUNNING;
	chn->prog_period = ins->prog_period;
	chn->instrument = ins;
	chn->program_counter = 0;
	chn->program_tick = 0;
	chn->program_loop = 1;
	mus->cyd->channel[chan].flags = ins->cydflags;
	chn->arpeggio_note = 0;
	chn->fixed_note = 0xffff;
	mus->cyd->channel[chan].fx_bus = ins->fx_bus;
	
	if (ins->flags & MUS_INST_DRUM)
	{
		cyd_set_waveform(&mus->cyd->channel[chan], CYD_CHN_ENABLE_NOISE);
	}
	
	if (ins->flags & MUS_INST_LOCK_NOTE)
	{
		note = (Uint16)ins->base_note;
	}
	else
	{
		note += (Uint16)((int)ins->base_note-MIDDLE_C);
	}
	
	mus_set_note(mus, chan, (Uint16)note << 8, 1, ins->flags & MUS_INST_QUARTER_FREQ ? 4 : 1);
	chn->last_note = chn->target_note = (Uint16)note << 8;
	chn->current_tick = 0;
	
	mus->song_track[chan].vibrato_position = 0;
	mus->song_track[chan].vib_delay = ins->vib_delay;
	
	mus->song_track[chan].slide_speed = 0;
	
	update_volumes(mus, &mus->song_track[chan], chn, &mus->cyd->channel[chan], ins->flags & MUS_INST_RELATIVE_VOLUME ? MAX_VOLUME : ins->volume);
	
	mus->cyd->channel[chan].sync_source = ins->sync_source == 0xff? chan : ins->sync_source;
	mus->cyd->channel[chan].ring_mod = ins->ring_mod == 0xff? chan : ins->ring_mod;
	
	mus->cyd->channel[chan].flttype = ins->flttype;
	
	
	if (ins->cydflags & CYD_CHN_ENABLE_KEY_SYNC)
	{
		mus->song_track[chan].pwm_position = 0;
	}	
	
	if (ins->flags & MUS_INST_SET_CUTOFF)
	{
		mus->song_track[chan].filter_cutoff = ins->cutoff;
		cyd_set_filter_coeffs(mus->cyd, &mus->cyd->channel[chan], ins->cutoff, ins->resonance);
	}
	
	if (ins->flags & MUS_INST_SET_PW)
	{
		mus->song_track[chan].pw = ins->pw;
		do_pwm(mus,chan);
	}
	
	if (ins->flags & MUS_INST_YM_BUZZ)
	{
		mus->cyd->channel[chan].flags |= CYD_CHN_ENABLE_YM_ENV;
		cyd_set_env_shape(&mus->cyd->channel[chan], ins->ym_env_shape);
		mus->channel[chan].buzz_offset = ins->buzz_offset;
	}
	else
	{
		mus->cyd->channel[chan].flags &= ~CYD_CHN_ENABLE_YM_ENV;
		memcpy(&mus->cyd->channel[chan].adsr, &ins->adsr, sizeof(ins->adsr));
	}
	
	if (ins->cydflags & CYD_CHN_ENABLE_WAVE)
	{
		cyd_set_wave_entry(&mus->cyd->channel[chan], &mus->cyd->wavetable_entries[ins->wavetable_entry]);
	}
	else
	{
		cyd_set_wave_entry(&mus->cyd->channel[chan], NULL);
	}
	
	//cyd_set_frequency(mus->cyd, &mus->cyd->channel[chan], chn->frequency);
	cyd_enable_gate(mus->cyd, &mus->cyd->channel[chan], 1);
	
	return chan;
}


int mus_trigger_instrument(MusEngine* mus, int chan, MusInstrument *ins, Uint8 note)
{
	cyd_lock(mus->cyd, 1);

	if (mus->cyd->callback_counter > mus->cyd->callback_period/2)
	{
		if (chan == -1)
		{
			for (int i = 0 ; i < mus->cyd->n_channels ; ++i)
			{
				if (!(mus->cyd->channel[i].flags & CYD_CHN_ENABLE_GATE) && !(mus->song_track[i].delayed.instrument))
					chan = i;
			}
			
			if (chan == -1)
				chan = rand() %  mus->cyd->n_channels;
		}
	
		mus->song_track[chan].delayed.note = note;
		mus->song_track[chan].delayed.instrument = ins;
		mus->song_track[chan].delayed.channel = chan;
	}
	else
	{
		chan = mus_trigger_instrument_internal(mus, chan, ins, note);
		mus->song_track[chan].delayed.instrument = NULL;
	}
	
	cyd_lock(mus->cyd, 0);
	
	return chan;
}


static void mus_advance_channel(MusEngine* mus, int chan)
{
	MusChannel *chn = &mus->channel[chan];
	
	if (mus->song_track[chan].delayed.instrument != NULL)
	{
		mus_trigger_instrument_internal(mus, mus->song_track[chan].delayed.channel, mus->song_track[chan].delayed.instrument, mus->song_track[chan].delayed.note);
		mus->song_track[chan].delayed.instrument = NULL;
	}
	

	if (!(mus->cyd->channel[chan].flags & CYD_CHN_ENABLE_GATE))
	{
		chn->flags &= ~MUS_CHN_PLAYING;
		return;
	}

	
	MusInstrument *ins = chn->instrument;
	
	if (ins->flags & MUS_INST_DRUM && chn->current_tick == 1) 
	{
		cyd_set_waveform(&mus->cyd->channel[chan], ins->cydflags);
	}
		
	if (mus->song_track[chan].slide_speed != 0)
	{
		if (chn->target_note > chn->note)
		{
			chn->note += my_min((Uint16)mus->song_track[chan].slide_speed, chn->target_note - chn->note);
		}
		else if (chn->target_note < chn->note)
		{
			chn->note -= my_min((Uint16)mus->song_track[chan].slide_speed , chn->note - chn->target_note);
		}
	}
		
	++chn->current_tick;
	
	if (mus->channel[chan].flags & MUS_CHN_PROGRAM_RUNNING)
	{
		int u = (chn->program_counter + 1) >= chn->prog_period;
		mus_exec_prog_tick(mus, chan, u);
		++chn->program_counter;
		if (u) chn->program_counter = 0;
		
		/*++chn->program_counter;	
		if (chn->program_counter >= chn->instrument->prog_period)
		{
			++chn->program_tick;
		
			if (chn->program_tick >= MUS_PROG_LEN)
			{
				chn->program_tick = 0;
			}
			chn->program_counter = 0;
		}*/
	}
	
	Uint8 ctrl = 0;
	int vibdep = my_max(0, (int)ins->vibrato_depth - (int)mus->song_track[chan].vib_delay);
	int vibspd = ins->vibrato_speed;
	
	if (mus->song_track[chan].pattern)
	{
		ctrl = mus->song_track[chan].pattern->step[mus->song_track[chan].pattern_step].ctrl;
		if ((mus->song_track[chan].pattern->step[mus->song_track[chan].pattern_step].command & 0xff00) == MUS_FX_VIBRATO)
		{
			ctrl |= MUS_CTRL_VIB;
			if (mus->song_track[chan].pattern->step[mus->song_track[chan].pattern_step].command & 0xff)
			{
				vibdep = (mus->song_track[chan].pattern->step[mus->song_track[chan].pattern_step].command & 0xf) << 2;
				vibspd = (mus->song_track[chan].pattern->step[mus->song_track[chan].pattern_step].command & 0xf0) >> 2;
				
				if (!vibspd)
					vibspd = ins->vibrato_speed;
				if (!vibdep)
					vibdep = ins->vibrato_depth;
			}
		}
			
		/*do_vib(mus, chan, mus->song_track[chan].pattern->step[mus->song_track[chan].pattern_step].ctrl);
		
		if ((mus->song_track[chan].last_ctrl & MUS_CTRL_VIB) && !(mus->song_track[chan].pattern->step[mus->song_track[chan].pattern_step].ctrl & MUS_CTRL_VIB))
		{
			cyd_set_frequency(mus->cyd, &mus->cyd->channel[chan], mus->channel[chan].frequency);
		}
		
		mus->song_track[chan].last_ctrl = mus->song_track[chan].pattern->step[mus->song_track[chan].pattern_step].ctrl;*/
	}
	
	Sint16 vib = 0;
	
	if (((ctrl & MUS_CTRL_VIB) && !(ins->flags & MUS_INST_INVERT_VIBRATO_BIT)) || (!(ctrl & MUS_CTRL_VIB) && (ins->flags & MUS_INST_INVERT_VIBRATO_BIT)))
	{
		mus->song_track[chan].vibrato_position += vibspd;
		vib = mus_shape(mus->song_track[chan].vibrato_position >> 1, ins->vib_shape) * vibdep / 64;
		if (mus->song_track[chan].vib_delay) --mus->song_track[chan].vib_delay;
	}
	
	
	do_pwm(mus, chan);
	
	Sint32 note = (mus->channel[chan].fixed_note != 0xffff ? mus->channel[chan].fixed_note : mus->channel[chan].note) + vib + ((Uint16)mus->channel[chan].arpeggio_note << 8);
	
	if (note < 0) note = 0;
	if (note > FREQ_TAB_SIZE << 8) note = (FREQ_TAB_SIZE - 1) << 8;
	
	mus_set_note(mus, chan, note, 0, ins->flags & MUS_INST_QUARTER_FREQ ? 4 : 1);
}


int mus_advance_tick(void* udata)
{
	MusEngine *mus = udata;
	
	if (mus->song)
	{
		for (int i = 0 ; i < mus->song->num_channels ; ++i)
		{
			if (mus->song_counter == 0)
			{
				while (mus->song_track[i].sequence_position < mus->song->num_sequences[i] && mus->song->sequence[i][mus->song_track[i].sequence_position].position <= mus->song_position)
				{
					mus->song_track[i].pattern = &mus->song->pattern[mus->song->sequence[i][mus->song_track[i].sequence_position].pattern];
					mus->song_track[i].pattern_step = mus->song_position - mus->song->sequence[i][mus->song_track[i].sequence_position].position;
					if (mus->song_track[i].pattern_step >= mus->song->pattern[mus->song->sequence[i][mus->song_track[i].sequence_position].pattern].num_steps) 
						mus->song_track[i].pattern = NULL;
					mus->song_track[i].note_offset = mus->song->sequence[i][mus->song_track[i].sequence_position].note_offset;
					++mus->song_track[i].sequence_position;
				}
			}
			
			int delay = 0;
			
			if (mus->song_track[i].pattern)
			{
				if ((mus->song_track[i].pattern->step[mus->song_track[i].pattern_step].command & 0x7FF0) == MUS_FX_EXT_NOTE_DELAY)
					delay = mus->song_track[i].pattern->step[mus->song_track[i].pattern_step].command & 0xf;
			}
			
			if (mus->song_counter == delay)
			{			
				if (mus->song_track[i].pattern)
				{
					
					if (1 || mus->song_track[i].pattern_step == 0)
					{
						Uint8 note = mus->song_track[i].pattern->step[mus->song_track[i].pattern_step].note < 0xf0 ? 
							mus->song_track[i].pattern->step[mus->song_track[i].pattern_step].note + mus->song_track[i].note_offset :
							mus->song_track[i].pattern->step[mus->song_track[i].pattern_step].note;
						Uint8 inst = mus->song_track[i].pattern->step[mus->song_track[i].pattern_step].instrument;
						MusInstrument *pinst = NULL;
						
						if (inst == MUS_NOTE_NO_INSTRUMENT)
						{
							pinst = mus->channel[i].instrument;
						}
						else
						{
							pinst = &mus->song->instrument[inst];
						}
						
						if (note == MUS_NOTE_RELEASE)
						{
							cyd_enable_gate(mus->cyd, &mus->cyd->channel[i], 0);
						}
						else if (pinst && note != MUS_NOTE_NONE)
						{
							mus->song_track[i].slide_speed = 0;
							int speed = pinst->slide_speed | 1;
							Uint8 ctrl = mus->song_track[i].pattern->step[mus->song_track[i].pattern_step].ctrl;
							
							if ((mus->song_track[i].pattern->step[mus->song_track[i].pattern_step].command & 0xff00) == MUS_FX_SLIDE)
							{
								ctrl |= MUS_CTRL_SLIDE | MUS_CTRL_LEGATO; 
								speed = (mus->song_track[i].pattern->step[mus->song_track[i].pattern_step].command & 0xff);
							}
							
							if (ctrl & MUS_CTRL_SLIDE)
							{
								if (ctrl & MUS_CTRL_LEGATO)
								{
									mus_set_slide(mus, i, ((Uint16)note + pinst->base_note - MIDDLE_C) << 8);
								}
								else
								{
									Uint16 oldnote = mus->channel[i].note;
									mus_trigger_instrument_internal(mus, i, pinst, note);
									mus->channel[i].note = oldnote;
								}
								mus->song_track[i].slide_speed = speed;
							}
							else if (ctrl & MUS_CTRL_LEGATO)
							{
								mus_set_note(mus, i, ((Uint16)note + pinst->base_note - MIDDLE_C) << 8, 1, pinst->flags & MUS_INST_QUARTER_FREQ ? 4 : 1);
								mus->channel[i].target_note = ((Uint16)note + pinst->base_note - MIDDLE_C) << 8;
							}
							else 
							{
								Uint8 prev_vol_track = mus->song_track[i].volume;
								Uint8 prev_vol_cyd = mus->cyd->channel[i].volume;
								mus_trigger_instrument_internal(mus, i, pinst, note);
								mus->channel[i].target_note = ((Uint16)note + pinst->base_note - MIDDLE_C) << 8;
								
								if (inst == MUS_NOTE_NO_INSTRUMENT)
								{
									mus->song_track[i].volume = prev_vol_track;
									mus->cyd->channel[i].volume = prev_vol_cyd;
								}
							}
							
							if (inst != MUS_NOTE_NO_INSTRUMENT)
								update_volumes(mus, &mus->song_track[i], &mus->channel[i], &mus->cyd->channel[i], MAX_VOLUME);
						}
					}
				}
			}
		}
		
		for (int i = 0 ; i < mus->cyd->n_channels ; ++i)
		{
			if (mus->song_track[i].pattern) mus_exec_track_command(mus, i);
		}
		
		++mus->song_counter;
		if (mus->song_counter >= ((!(mus->song_position & 1)) ? mus->song->song_speed : mus->song->song_speed2))
		{
			for (int i = 0 ; i < mus->cyd->n_channels ; ++i)
			{
				if (mus->song_track[i].pattern)
				{
					++mus->song_track[i].pattern_step;
					if (mus->song_track[i].pattern_step >= mus->song_track[i].pattern->num_steps)
					{
						mus->song_track[i].pattern = NULL;
						mus->song_track[i].pattern_step = 0;
					}
				}
			}
			mus->song_counter = 0;
			++mus->song_position;
			if (mus->song_position >= mus->song->song_length)
			{
				if (mus->song->flags & MUS_NO_REPEAT)
					return 0;
				
				mus->song_position = mus->song->loop_point;
				for (int i = 0 ; i < mus->cyd->n_channels ; ++i)
				{
					mus->song_track[i].pattern = NULL;
					mus->song_track[i].pattern_step = 0;
					mus->song_track[i].sequence_position = 0;
				}
			}
		}
	}
	
	for (int i = 0 ; i < mus->cyd->n_channels ; ++i)
	{
		if (mus->channel[i].flags & MUS_CHN_PLAYING || mus->song_track[i].delayed.instrument) 
		{
			mus_advance_channel(mus, i);
		}
	}
	
	if (mus->song && (mus->song->flags & MUS_ENABLE_MULTIPLEX) && mus->song->multiplex_period > 0)
	{
		for (int i = 0 ; i < mus->cyd->n_channels ; ++i)
		{
			CydChannel *cydchn = &mus->cyd->channel[i];
			
			if ((mus->multiplex_ctr / mus->song->multiplex_period) == i)
			{
				update_volumes(mus, &mus->song_track[i], &mus->channel[i], cydchn, mus->song_track[i].volume);
			}
			else
			{
				cydchn->volume = 0;
			}
		}
		
		if (++mus->multiplex_ctr >= mus->song->num_channels * mus->song->multiplex_period)
			mus->multiplex_ctr = 0;
	}
	
	return 1;
}


void mus_set_song(MusEngine *mus, MusSong *song, Uint16 position)
{
	cyd_lock(mus->cyd, 1);
	cyd_reset(mus->cyd);
	mus->song = song;
	
	if (song != NULL)
	{
		mus->song_counter = 0;
		mus->multiplex_ctr = 0;
	}
	
	mus->song_position = position;
	
	for (int i = 0 ; i < MUS_MAX_CHANNELS ; ++i)
	{
		mus->song_track[i].pattern = NULL;
		mus->song_track[i].pattern_step = 0;
		mus->song_track[i].sequence_position = 0;
		mus->song_track[i].last_ctrl = 0;
		mus->song_track[i].delayed.instrument = NULL;
		mus->song_track[i].note_offset = 0;
	}
	
	cyd_lock(mus->cyd, 0);
}


int mus_poll_status(MusEngine *mus, int *song_position, int *pattern_position, MusPattern **pattern, MusChannel *channel)
{
	cyd_lock(mus->cyd, 1);
	
	if (song_position) *song_position = mus->song_position;
	
	if (pattern_position)
	{
		for (int i = 0 ; i < MUS_MAX_CHANNELS ; ++i)
		{
			pattern_position[i] = mus->song_track[i].pattern_step;
		}
	}
	
	if (pattern)
	{
		for (int i = 0 ; i < MUS_MAX_CHANNELS ; ++i)
		{
			pattern[i] = mus->song_track[i].pattern;
		}
	}
	
	if (channel)
	{
		memcpy(channel, mus->channel, sizeof(mus->channel));
	}
	
	cyd_lock(mus->cyd, 0);
	
	return mus->song != NULL;
}


int mus_load_instrument(const char *path, MusInstrument *inst)
{
	FILE *f = fopen(path, "rb");
	
	if (f)
	{
		int r = mus_load_instrument_file2(f, inst);
	
		fclose(f);
		
		return r;
	}
	
	return 0;
}


int mus_load_instrument_file(Uint8 version, FILE *f, MusInstrument *inst)
{
	mus_get_default_instrument(inst);

	_VER_READ(&inst->flags, 0);
	_VER_READ(&inst->cydflags, 0);
	_VER_READ(&inst->adsr, 0);
	_VER_READ(&inst->sync_source, 0);
	_VER_READ(&inst->ring_mod, 0); 
	_VER_READ(&inst->pw, 0);
	_VER_READ(&inst->volume, 0);
	Uint8 progsteps = 0;
	_VER_READ(&progsteps, 0);
	if (progsteps)
		_VER_READ(&inst->program, (int)progsteps*sizeof(inst->program[0]));
	_VER_READ(&inst->prog_period, 0); 
	_VER_READ(&inst->vibrato_speed, 0); 
	_VER_READ(&inst->vibrato_depth, 0); 
	_VER_READ(&inst->pwm_speed, 0); 
	_VER_READ(&inst->pwm_depth, 0); 
	_VER_READ(&inst->slide_speed, 0);
	_VER_READ(&inst->base_note, 0);
	Uint8 len = 16;
	VER_READ(version, 11, 0xff, &len, 0);
	if (len)
	{
		memset(inst->name, 0, sizeof(inst->name));
		_VER_READ(inst->name, my_min(len, sizeof(inst->name)));
		inst->name[sizeof(inst->name) - 1] = '\0';
	}
	
	VER_READ(version, 1, 0xff, &inst->cutoff, 0);
	VER_READ(version, 1, 0xff, &inst->resonance, 0);
	VER_READ(version, 1, 0xff, &inst->flttype, 0);
	VER_READ(version, 7, 0xff, &inst->ym_env_shape, 0);
	VER_READ(version, 7, 0xff, &inst->buzz_offset, 0);
	VER_READ(version, 10, 0xff, &inst->fx_bus, 0);
	VER_READ(version, 11, 0xff, &inst->vib_shape, 0);
	VER_READ(version, 11, 0xff, &inst->vib_delay, 0);
	VER_READ(version, 11, 0xff, &inst->pwm_shape, 0);
	VER_READ(version, 12, 0xff, &inst->wavetable_entry, 0);
	
	/* The file format is little-endian, the following only does something on big-endian machines */
	
	FIX_ENDIAN(inst->flags);
	FIX_ENDIAN(inst->cydflags);
	FIX_ENDIAN(inst->pw);
	FIX_ENDIAN(inst->cutoff);
	FIX_ENDIAN(inst->buzz_offset);
	
	for (int i = 0 ; i < progsteps ; ++i)
		FIX_ENDIAN(inst->program[i]);
	
	return 1;
}


int mus_load_instrument_file2(FILE *f, MusInstrument *inst)
{
	char id[9];
				
	id[8] = '\0';

	fread(id, 8, sizeof(id[0]), f);
	
	if (strcmp(id, MUS_INST_SIG) == 0)
	{
		Uint8 version = 0;
		fread(&version, 1, sizeof(version), f);
		
		if (version > MUS_VERSION)
			return 0;
	
		mus_load_instrument_file(version, f, inst);
		
		return 1;
	}
	else
	{
		debug("Instrument signature does not match");
		return 0;
	}
}


void mus_get_default_instrument(MusInstrument *inst)
{
	memset(inst, 0, sizeof(*inst));
	inst->flags = MUS_INST_DRUM|MUS_INST_SET_PW|MUS_INST_SET_CUTOFF;
	inst->pw = 0x600;
	inst->cydflags = CYD_CHN_ENABLE_TRIANGLE;
	inst->adsr.a = 1;
	inst->adsr.d = 12;
	inst->volume = MAX_VOLUME;
	inst->base_note = MIDDLE_C;
	inst->prog_period = 2;
	inst->cutoff = 2047;
	inst->slide_speed = 0x80;
	inst->vibrato_speed = 0x20;
	inst->vibrato_depth = 0x20;
	inst->vib_shape = MUS_SHAPE_SINE;
	inst->vib_delay = 0;
	
	for (int p = 0 ; p < MUS_PROG_LEN; ++p)
		inst->program[p] = MUS_FX_NOP;
}


void mus_set_fx(MusEngine *mus, MusSong *song)
{
	cyd_lock(mus->cyd, 1);
	for(int f = 0 ; f < CYD_MAX_FX_CHANNELS ; ++f)
	{
		cydfx_set(&mus->cyd->fx[f], &song->fx[f]);
	}
	cyd_lock(mus->cyd, 0);
}


int mus_load_song_file(FILE *f, MusSong *song)
{
	char id[9];
	id[8] = '\0';

	fread(id, 8, sizeof(id[0]), f);
	
	if (strcmp(id, MUS_SONG_SIG) == 0)
	{
		Uint8 version = 0;
		fread(&version, 1, sizeof(version), f);
		
		if (version > MUS_VERSION)
			return 0;
		
		if (version >= 6) 
			fread(&song->num_channels, 1, sizeof(song->num_channels), f);
		else 
		{
			if (version > 3) 
				song->num_channels = 4;
			else 
				song->num_channels = 3;
		}	
		
		fread(&song->time_signature, 1, sizeof(song->time_signature), f);
		fread(&song->num_instruments, 1, sizeof(song->num_instruments), f);
		fread(&song->num_patterns, 1, sizeof(song->num_patterns), f);
		fread(song->num_sequences, 1, sizeof(song->num_sequences[0]) * (int)song->num_channels, f);
		fread(&song->song_length, 1, sizeof(song->song_length), f);
		fread(&song->loop_point, 1, sizeof(song->loop_point), f);
		fread(&song->song_speed, 1, sizeof(song->song_speed), f);
		fread(&song->song_speed2, 1, sizeof(song->song_speed2), f);
		fread(&song->song_rate, 1, sizeof(song->song_rate), f);
		
		if (version > 2) fread(&song->flags, 1, sizeof(song->flags), f);
		else song->flags = 0;
		
		if (version >= 9) fread(&song->multiplex_period, 1, sizeof(song->multiplex_period), f);
		else song->multiplex_period = 3;
		
		/* The file format is little-endian, the following only does something on big-endian machines */
		
		FIX_ENDIAN(song->song_length);
		FIX_ENDIAN(song->loop_point);
		FIX_ENDIAN(song->time_signature);
		FIX_ENDIAN(song->num_patterns);
		FIX_ENDIAN(song->flags);
		
		for (int i = 0 ; i < (int)song->num_channels ; ++i)
			FIX_ENDIAN(song->num_sequences[i]);
		
		Uint8 title_len = 16 + 1; // old length
		
		if (version >= 11)
		{
			fread(&title_len, 1, 1, f);
		}
		
		if (version >= 5) 
		{
			memset(song->title, 0, sizeof(song->title));
			fread(song->title, 1, my_min(sizeof(song->title), title_len), f);
			song->title[sizeof(song->title) - 1] = '\0';
		}
		
		Uint8 n_fx = 0;
		
		if (version >= 10)
			fread(&n_fx, 1, sizeof(n_fx), f);
		else if (song->flags & MUS_ENABLE_REVERB)
			n_fx = 1;
		
		if (n_fx > 0)
		{
			if (version >= 10)
			{
				fread(&song->fx, sizeof(song->fx[0]), n_fx, f);
				
				for (int fx = 0 ; fx < n_fx ; ++fx)
				{
					FIX_ENDIAN(song->fx[fx].flags);
					
					for (int i = 0 ; i < CYDRVB_TAPS ; ++i)
					{
						FIX_ENDIAN(song->fx[fx].rvb.tap[i].gain);
						FIX_ENDIAN(song->fx[fx].rvb.tap[i].delay);
					}
				}
			}
			else
			{
				for (int fx = 0 ; fx < n_fx ; ++fx)
				{
					song->fx[fx].flags = CYDFX_ENABLE_REVERB;
					if (song->flags & MUS_ENABLE_CRUSH) song->fx[fx].flags |= CYDFX_ENABLE_CRUSH;
					
					song->fx[fx].rvb.spread = 0;
				
					for (int i = 0 ; i < CYDRVB_TAPS ; ++i)	
					{
						Sint32 g, d;
						fread(&g, 1, sizeof(g), f);
						fread(&d, 1, sizeof(d), f);
							
						song->fx[fx].rvb.tap[i].gain = g;
						song->fx[fx].rvb.tap[i].delay = d;
						
						FIX_ENDIAN(song->fx[fx].rvb.tap[i].gain);
						FIX_ENDIAN(song->fx[fx].rvb.tap[i].delay);
					}
				}
			}
		}
		
		if (song->instrument == NULL)
			song->instrument = malloc((size_t)song->num_instruments * sizeof(song->instrument[0]));
		
		for (int i = 0 ; i < song->num_instruments; ++i)
		{
			mus_load_instrument_file(version, f, &song->instrument[i]); 
		}
		
		
		for (int i = 0 ; i < song->num_channels ; ++i)
		{
			if (song->num_sequences[i] > 0)
			{
				if (song->sequence[i] == NULL)
					song->sequence[i] = malloc((size_t)song->num_sequences[i] * sizeof(song->sequence[0][0]));
			
				if (version < 8)
				{
					fread(song->sequence[i], song->num_sequences[i], sizeof(song->sequence[i][0]), f);
				}
				else
				{
					for (int s = 0 ; s < song->num_sequences[i] ; ++s)
					{
						fread(&song->sequence[i][s].position, 1, sizeof(song->sequence[i][s].position), f);
						fread(&song->sequence[i][s].pattern, 1, sizeof(song->sequence[i][s].pattern), f);
						fread(&song->sequence[i][s].note_offset, 1, sizeof(song->sequence[i][s].note_offset), f);
					}
				}
				
				for (int s = 0 ; s < song->num_sequences[i] ; ++s)
				{
					FIX_ENDIAN(song->sequence[i][s].position);
					FIX_ENDIAN(song->sequence[i][s].pattern);
				}
			}
		}
		
		if (song->pattern == NULL)
		{
			song->pattern = calloc((size_t)song->num_patterns, sizeof(song->pattern[0]));
			//memset(song->pattern, 0, (size_t)song->num_patterns * sizeof(song->pattern[0]));
		}
		
		for (int i = 0 ; i < song->num_patterns; ++i)
		{
			Uint16 steps;
			fread(&steps, 1, sizeof(song->pattern[i].num_steps), f);
			
			FIX_ENDIAN(steps);
			
			if (song->pattern[i].step == NULL)
				song->pattern[i].step = calloc((size_t)steps, sizeof(song->pattern[i].step[0]));
			else if (steps > song->pattern[i].num_steps)
				song->pattern[i].step = realloc(song->pattern[i].step, (size_t)steps * sizeof(song->pattern[i].step[0]));
				
			song->pattern[i].num_steps = steps;
			
			if (version < 8)
			{
				size_t s = (version < 2) ? sizeof(Uint8)*3 : sizeof(song->pattern[i].step[0]);
				for (int step = 0 ; step < song->pattern[i].num_steps ; ++step)
				{
					fread(&song->pattern[i].step[step], 1, s, f);
					FIX_ENDIAN(song->pattern[i].step[step].command);
				}
			}
			else
			{
				int len = song->pattern[i].num_steps / 2 + (song->pattern[i].num_steps & 1);
				
				Uint8 *packed = malloc(sizeof(Uint8) * len);
				Uint8 *current = packed;
				
				fread(packed, sizeof(Uint8), len, f);
				
				for (int s = 0 ; s < song->pattern[i].num_steps ; ++s)
				{
					Uint8 bits = (s & 1 || s == song->pattern[i].num_steps - 1) ? (*current & 0xf) : (*current >> 4);
					
					if (bits & MUS_PAK_BIT_NOTE)
						fread(&song->pattern[i].step[s].note, 1, sizeof(song->pattern[i].step[s].note), f);
					else
						song->pattern[i].step[s].note = MUS_NOTE_NONE;
						
					if (bits & MUS_PAK_BIT_INST)
						fread(&song->pattern[i].step[s].instrument, 1, sizeof(song->pattern[i].step[s].instrument), f);
					else
						song->pattern[i].step[s].instrument = MUS_NOTE_NO_INSTRUMENT;
						
					if (bits & MUS_PAK_BIT_CTRL)
						fread(&song->pattern[i].step[s].ctrl, 1, sizeof(song->pattern[i].step[s].ctrl), f);
					else
						song->pattern[i].step[s].ctrl = 0;
						
					if (bits & MUS_PAK_BIT_CMD)
						fread(&song->pattern[i].step[s].command, 1, sizeof(song->pattern[i].step[s].command), f);
					else
						song->pattern[i].step[s].command = 0;
						
					FIX_ENDIAN(song->pattern[i].step[s].command);
					
					if (s & 1) ++current;
				}
				
				free(packed);
			}
		}
		
		return 1;
	}
	
	return 0;
}


int mus_load_song(const char *path, MusSong *song)
{
	FILE *f = fopen(path, "rb");
	
	if (f)
	{	
		int r = mus_load_song_file(f, song);
		fclose(f);
		
		return r;
	}
	
	return 0;
}

void mus_free_song(MusSong *song)
{
	free(song->instrument);

	for (int i = 0 ; i < MUS_MAX_CHANNELS; ++i)
	{
		free(song->sequence[i]);
	}
	
	for (int i = 0 ; i < song->num_patterns; ++i)
	{
		free(song->pattern[i].step);
	}
	
	free(song->pattern);
}


void mus_release(MusEngine *mus, int chan)
{	
	cyd_lock(mus->cyd, 1);
	cyd_enable_gate(mus->cyd, &mus->cyd->channel[chan], 0);
	
	mus->song_track[chan].delayed.instrument = NULL;
	
	cyd_lock(mus->cyd, 0);
}
