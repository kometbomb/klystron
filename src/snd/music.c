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


#include "music.h"
#include <assert.h>
#include <stdio.h>
#include "freqs.h"
#include "macros.h"
#include "rnd.h"


static int mus_trigger_instrument_internal(MusEngine* mus, int chan, MusInstrument *ins, Uint8 note);


static void mus_set_note(MusEngine *mus, int chan, Uint16 note, int update_note)
{
	MusChannel *chn = &mus->channel[chan];
	
	if (update_note) chn->note = note;
	
	Uint16 frequency;
	
	if ((note & 0xff) == 0)
	{
		frequency = frequency_table[(note >> 8) % FREQ_TAB_SIZE];
	}
	else
	{
		Uint16 f1 = frequency_table[(note >> 8) % FREQ_TAB_SIZE];
		Uint16 f2 = frequency_table[((note >> 8) + 1) % FREQ_TAB_SIZE];
		frequency = f1 + ((f2-f1) * (note & 0xff)) / 256;
	}
	
	cyd_set_frequency(mus->cyd, &mus->cyd->channel[chan], frequency);
}


static void mus_set_slide(MusEngine *mus, int chan, Uint16 note)
{
	MusChannel *chn = &mus->channel[chan];
	chn->target_note = note;
	//if (update_note) chn->note = note;
}


void mus_init_engine(MusEngine *mus, CydEngine *cyd)
{
	init_genrand(34);
	memset(mus, 0, sizeof(*mus));
	mus->cyd = cyd;
	mus->volume = 128;
}


static void do_command(MusEngine *mus, int chan, int tick, Uint16 inst)
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
		}
		break;
		
		case MUS_FX_PORTA_DN:
		{
			Uint16 prev = chn->note;
			chn->note -= ((inst & 0xff) << 2);
			if (prev < chn->note) chn->note = 0x0;
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
				
		case MUS_FX_PW_SET:
		{
			mus->song_track[chan].pw = inst & 0xff << 4;
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
				cydchn->volume = mus->song_track[chan].volume * (int)mus->volume / 128;
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
						cydchn->volume = 0;
					}
				}
				break;
				
				case MUS_FX_EXT_RETRIGGER:
				{
					if ((inst & 0xf) > 0 && (tick % (inst & 0xf)) == 0)
					{
						Uint8 prev_vol = mus->song_track[chan].volume;
						Uint16 note = chn->note;
						mus_trigger_instrument_internal(mus, chan, chn->instrument, chn->note);
						mus_set_note(mus, chan, note, 1);
						mus->song_track[chan].volume = prev_vol;
					}
				}
				break;
			}
		}
		break;
		
		case MUS_FX_SET_SPEED:
		{
			mus->song->song_speed = inst & 0xf;
			if ((inst & 0xf0) == 0) mus->song->song_speed2 = mus->song->song_speed;
			else mus->song->song_speed2 = (inst >> 4) & 0xf;
		}
		break;
		
		case MUS_FX_SET_RATE:
		{
			mus->song->song_rate = inst & 0xff;
			if (mus->song->song_rate < 1) mus->song->song_rate = 1;
			cyd_set_callback_rate(mus->cyd, mus->song->song_rate);
		}
		break;
	}
	
	if (tick != 0) return;
	
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
						cydchn->volume = mus->song_track[chan].volume * (int)mus->volume / 128;
					}
				}
				break;
				
				case MUS_FX_EXT_FADE_VOLUME_UP:
				{
					if (!(chn->flags & MUS_CHN_DISABLED))
					{
						mus->song_track[chan].volume += inst & 0xf;
						if (mus->song_track[chan].volume > MAX_VOLUME) mus->song_track[chan].volume = MAX_VOLUME;
						cydchn->volume = mus->song_track[chan].volume * (int)mus->volume / 128;
					}
				}
				break;
				
				case MUS_FX_EXT_PORTA_UP:
				{
					Uint16 prev = chn->note;
					chn->note += ((inst & 0x0f));
					if (prev > chn->note) chn->note = 0xffff;
				}
				break;
				
				case MUS_FX_EXT_PORTA_DN:
				{
					Uint16 prev = chn->note;
					chn->note -= ((inst & 0x0f));
					if (prev < chn->note) chn->note = 0x0;
				}
				break;
			}
		}
		break;
		
		case MUS_FX_SET_PANNING:
		{
			cyd_set_panning(mus->cyd, cydchn, inst & 0xff);
		}
		break;
	
		case MUS_FX_CUTOFF_SET:
		{
			mus->song_track[chan].filter_cutoff = (inst & 0xff) << 3;
			if (mus->song_track[chan].filter_cutoff > 0x7ff) mus->song_track[chan].filter_cutoff = 0x7ff;
			cyd_set_filter_coeffs(mus->cyd, cydchn, mus->song_track[chan].filter_cutoff, chn->instrument->resonance);
		}
		break;
		
		default:
		
		switch (inst & 0x7f00)
		{
			case MUS_FX_PORTA_UP_SEMI:
			{
				Uint16 prev = chn->note;
				chn->note += (inst&0xff) << 8;
				if (prev > chn->note || chn->note >= (FREQ_TAB_SIZE << 8)) chn->note = ((FREQ_TAB_SIZE-1) << 8);
			}
			break;
			
			case MUS_FX_PORTA_DN_SEMI:
			{
				Uint16 prev = chn->note;
				chn->note -= (inst&0xff) << 8;
				if (prev < chn->note) chn->note = 0x0;
			}
			break;
			
			case MUS_FX_ARPEGGIO:
			{
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
				mus->cyd->channel[chan].volume = (chn->flags & MUS_CHN_DISABLED) ? 0 : mus->song_track[chan].volume * (int)mus->volume / 128;
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
		do_command(mus, chan, mus->song_counter, inst);
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
			
			do_command(mus, chan, chn->program_counter, inst);
			
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


static void do_pwm(MusEngine* mus, int chan)
{
	MusChannel *chn = &mus->channel[chan];
	MusInstrument *ins = chn->instrument;

	mus->song_track[chan].pwm_position += ins->pwm_speed;
	mus->cyd->channel[chan].pw = mus->song_track[chan].pw + vibrato_table[((mus->song_track[chan].pwm_position) >> 1) & (VIB_TAB_SIZE - 1)] * ins->pwm_depth / 32;
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
	chn->instrument = ins;
	chn->program_counter = 0;
	chn->program_tick = 0;
	chn->program_loop = 1;
	mus->cyd->channel[chan].flags = ins->cydflags;
	chn->arpeggio_note = 0;
	
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
	
	mus_set_note(mus, chan, (Uint16)note << 8, 1);
	chn->target_note = (Uint16)note << 8;
	chn->current_tick = 0;
	mus->song_track[chan].vibrato_position = 0;
	mus->song_track[chan].slide_speed = 0;
	
	mus->song_track[chan].volume = ins->volume;
	mus->cyd->channel[chan].volume = (chn->flags & MUS_CHN_DISABLED) ? 0 : mus->song_track[chan].volume * (int)mus->volume / 128;
	
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
	
	
	memcpy(&mus->cyd->channel[chan].adsr, &ins->adsr, sizeof(ins->adsr));
	
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
		int u = (chn->program_counter + 1) >= chn->instrument->prog_period;
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
	int vibdep = ins->vibrato_depth;
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
		vib = vibrato_table[((mus->song_track[chan].vibrato_position) >> 1) & (VIB_TAB_SIZE - 1)] * vibdep / 64;
	}
	
	
	do_pwm(mus, chan);
	
	Sint32 note = mus->channel[chan].note + vib + ((Uint16)mus->channel[chan].arpeggio_note << 8);
	
	if (note < 0) note = 0;
	if (note > FREQ_TAB_SIZE << 8) note = (FREQ_TAB_SIZE << 8) - 1;
	
	mus_set_note(mus, chan, note, 0);
}


void mus_advance_tick(void* udata)
{
	MusEngine *mus = udata;
	
	if (mus->song)
	{
		if	(mus->song_counter == 0)
		{
			for (int i = 0 ; i < mus->song->num_channels ; ++i)
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
						else if (note != MUS_NOTE_NONE)
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
								mus_set_note(mus, i, (Uint16)note << 8, 1);
								mus->channel[i].target_note = ((Uint16)note + pinst->base_note - MIDDLE_C) << 8;
							}
							else 
							{
								Uint8 prev_vol = mus->song_track[i].volume;
								mus_trigger_instrument_internal(mus, i, pinst, note);
								mus->channel[i].target_note = ((Uint16)note + pinst->base_note - MIDDLE_C) << 8;
								if (inst == MUS_NOTE_NO_INSTRUMENT)
								{
									mus->song_track[i].volume = prev_vol;
									mus->cyd->channel[i].volume = prev_vol;
								}
							}
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
}


void mus_set_song(MusEngine *mus, MusSong *song, Uint16 position)
{
	cyd_lock(mus->cyd, 1);
	cyd_reset(mus->cyd);
	mus->song = song;
	
	if (song != NULL)
		mus->song_counter = 0;
		
	mus->song_position = position;
	
	for (int i = 0 ; i < MUS_MAX_CHANNELS ; ++i)
	{
		mus->song_track[i].pattern = NULL;
		mus->song_track[i].pattern_step = 0;
		mus->song_track[i].sequence_position = 0;
		mus->song_track[i].last_ctrl = 0;
		mus->song_track[i].delayed.instrument = NULL;
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


void mus_load_instrument(const char *path, MusInstrument *inst)
{
	FILE *f = fopen(path, "rb");
	
	if (f)
	{
		mus_load_instrument_file2(f, inst);
	
		fclose(f);
	}
}


void mus_load_instrument_file(Uint8 version, FILE *f, MusInstrument *inst)
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
	_VER_READ(inst->name, sizeof(inst->name));
	VER_READ(version, 1, 0xff, &inst->cutoff, 0);
	VER_READ(version, 1, 0xff, &inst->resonance, 0);
	VER_READ(version, 1, 0xff, &inst->flttype, 0);
}


void mus_load_instrument_file2(FILE *f, MusInstrument *inst)
{
	char id[9];
				
	id[8] = '\0';

	fread(id, 8, sizeof(id[0]), f);
	
	if (strcmp(id, MUS_INST_SIG) == 0)
	{
		Uint8 version = 0;
		fread(&version, 1, sizeof(version), f);
	
		mus_load_instrument_file(version, f, inst);
	}
	else
	{
		debug("Instrument signature does not match");
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
	
	for (int p = 0 ; p < MUS_PROG_LEN; ++p)
		inst->program[p] = MUS_FX_NOP;
}


void mus_set_reverb(MusEngine *mus, MusSong *song)
{
	cyd_lock(mus->cyd, 1);
	if (song->flags & MUS_ENABLE_REVERB)
	{
		for (int i = 0 ; i < CYDRVB_TAPS ; ++i)
		{
			cydrvb_set_tap(&mus->cyd->rvb, i, song->rvbtap[i].delay, song->rvbtap[i].gain);
		}
		
		mus->cyd->flags |= CYD_ENABLE_REVERB;
	}
	else
	{
		mus->cyd->flags &= ~CYD_ENABLE_REVERB;
	}
	cyd_lock(mus->cyd, 0);
}


void mus_load_song_file(FILE *f, MusSong *song)
{
	char id[9];
	id[8] = '\0';

	fread(id, 8, sizeof(id[0]), f);
	
	if (strcmp(id, MUS_SONG_SIG) == 0)
	{
		Uint8 version = 0;
		fread(&version, 1, sizeof(version), f);
		
		if (version >= 6) fread(&song->num_channels, 1, sizeof(song->num_channels), f);
			else song->num_channels = 4;
		
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
		
		if (version >= 5) fread(song->title, 1, MUS_TITLE_LEN + 1, f);
		
		if (song->flags & MUS_ENABLE_REVERB)
		{
			for (int i = 0 ; i < CYDRVB_TAPS ; ++i)	
			{
				fread(&song->rvbtap[i].gain, 1, sizeof(song->rvbtap[i].gain), f);
				fread(&song->rvbtap[i].delay, 1, sizeof(song->rvbtap[i].delay), f);
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
			
				fread(song->sequence[i], song->num_sequences[i], sizeof(song->sequence[i][0]), f);
			}
		}
		
		if (song->pattern == NULL)
		{
			song->pattern = calloc((size_t)song->num_patterns, sizeof(song->pattern[0]));
			//memset(song->pattern, 0, (size_t)song->num_patterns * sizeof(song->pattern[0]));
		}
		
		for (int i = 0 ; i < song->num_patterns; ++i)
		{
			fread(&song->pattern[i].num_steps, 1, sizeof(song->pattern[i].num_steps), f);
			
			if (song->pattern[i].step == NULL)
				song->pattern[i].step = calloc((size_t)song->pattern[i].num_steps, sizeof(song->pattern[i].step[0]));
			
			size_t s = (version < 2) ? sizeof(Uint8)*3 : sizeof(song->pattern[i].step[0]);
			
			for (int step = 0 ; step < song->pattern[i].num_steps ; ++step)
				fread(&song->pattern[i].step[step], 1, s, f);
		}
	}
}


void mus_load_song(const char *path, MusSong *song)
{
	FILE *f = fopen(path, "rb");
	
	if (f)
	{	
		mus_load_song_file(f, song);
		fclose(f);
	}
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
