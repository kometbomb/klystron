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

#include "cyd.h"
#include "macros.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef ENABLEAUDIODUMP
#include <time.h>
#endif

#include "cyddefs.h"
#include "cydwave.h"
#include "freqs.h"

#ifndef USENATIVEAPIS

# include "SDL_mixer.h"

#else

# ifdef WIN32
# endif

#endif


#define envspd(cyd,slope) (slope!=0?((0xff0000 / ((slope) * (slope) * 256)) * CYD_BASE_FREQ / cyd->sample_rate):0xff0000)


static Sint32 inline fastrnd(Uint32 fastrnd_rndi)
{
	return ((fastrnd_rndi & 0x400000) >> 11) |
		((fastrnd_rndi & 0x100000) >> 10) |
		((fastrnd_rndi & 0x010000) >> 7) |
		((fastrnd_rndi & 0x002000) >> 5) |
		((fastrnd_rndi & 0x000800) >> 4) |
		((fastrnd_rndi & 0x000080) >> 1) |
		((fastrnd_rndi & 0x000010) << 1) |
		((fastrnd_rndi & 0x000004) << 2);
}


static void cyd_init_channel(CydEngine *cyd, CydChannel *chn)
{
	memset(chn, 0, sizeof(*chn));
	chn->random = RANDOM_SEED;
	chn->pw = 0x400;
	cyd_set_filter_coeffs(cyd, chn, 2047, 0);
#ifdef STEREOOUTPUT
	cyd_set_panning(cyd, chn, CYD_PAN_CENTER);
#endif
}


static void cyd_init_log_tables(CydEngine *cyd)
{
	for (int i = 0 ; i < LUT_SIZE ; ++i)
	{
		cyd->lookup_table[i] = i * (i/2) / ((LUT_SIZE*LUT_SIZE / 65536)/2);
	}
	
	for (int i = 0 ; i < YM_LUT_SIZE ; ++i)
	{
		cyd->lookup_table_ym[i] = (Uint32)32767 * (Uint32)(i+1) * (Uint32)(i+1) * (Uint32)(i+1) / (Uint32)(YM_LUT_SIZE * YM_LUT_SIZE * YM_LUT_SIZE);
	}
	
	cyd->lookup_table_ym[0] = 0;
}


void cyd_reset_wavetable(CydEngine *cyd)
{
	memset(cyd->wavetable_entries, 0, sizeof(cyd->wavetable_entries[0]) * CYD_WAVE_MAX_ENTRIES);

	for (int i = 0 ; i < CYD_WAVE_MAX_ENTRIES ; ++i)
	{
		cyd_wave_entry_init(&cyd->wavetable_entries[i], NULL, 0, 0, 0, 0, 0);
	}
}


void cyd_init(CydEngine *cyd, Uint16 sample_rate, int channels)
{
	memset(cyd, 0, sizeof(*cyd));
	cyd->sample_rate = sample_rate;
	cyd->lookup_table = malloc(sizeof(*cyd->lookup_table) * LUT_SIZE);
	cyd->lookup_table_ym = malloc(sizeof(*cyd->lookup_table) * YM_LUT_SIZE);
	cyd->n_channels = channels;
	
	if (cyd->n_channels > CYD_MAX_CHANNELS)
		cyd->n_channels = CYD_MAX_CHANNELS;
	
	cyd->channel = calloc(sizeof(*cyd->channel), cyd->n_channels);
	
	
#ifndef USENATIVEAPIS

# ifdef USESDLMUTEXES
	cyd->mutex = SDL_CreateMutex();
# endif

#else

# ifdef WIN32
	InitializeCriticalSection(&cyd->mutex);
# endif

#endif
	
	cyd_init_log_tables(cyd);
	
	for (int i = 0 ; i < CYD_MAX_FX_CHANNELS ; ++i)
		cydfx_init(&cyd->fx[i], sample_rate);
	
	cyd->wavetable_entries = calloc(sizeof(cyd->wavetable_entries[0]), CYD_WAVE_MAX_ENTRIES);
	
	cyd_reset_wavetable(cyd);
	
	cyd_reset(cyd);
}


void cyd_deinit(CydEngine *cyd)
{
	if (cyd->lookup_table)
	{
		free(cyd->lookup_table);
		cyd->lookup_table = NULL;
	}
	
	if (cyd->lookup_table_ym)	
	{
		free(cyd->lookup_table_ym);
		cyd->lookup_table_ym = NULL;
	}

	if (cyd->channel)
	{
		free(cyd->channel);
		cyd->channel = NULL;
	}
	
	for (int i = 0 ; i < CYD_MAX_FX_CHANNELS ; ++i)
		cydfx_deinit(&cyd->fx[i]);
	
#ifndef USENATIVEAPIS

# ifdef USESDLMUTEXES
	if (cyd->mutex)
		SDL_DestroyMutex(cyd->mutex);
	cyd->mutex = NULL;
# endif	

#else

# ifdef WIN32
	DeleteCriticalSection(&cyd->mutex);
# endif

#endif

#ifdef ENABLEAUDIODUMP
	if (cyd->dump) fclose(cyd->dump);
	cyd->dump = NULL;
#endif

	if (cyd->wavetable_entries)
	{
		for (int i = 0 ; i < CYD_WAVE_MAX_ENTRIES ; ++i)
			cyd_wave_entry_deinit(&cyd->wavetable_entries[i]);
			
		free(cyd->wavetable_entries);
		cyd->wavetable_entries = NULL;
	}
}


void cyd_reset(CydEngine *cyd)
{
	for (int i = 0 ; i < cyd->n_channels ; ++i)
	{
		cyd_init_channel(cyd, &cyd->channel[i]);
		cyd->channel[i].sync_source = i;
	}
}


static inline void cyd_cycle_adsr(CydEngine *eng, CydChannel *chn)
{
	if (!(chn->flags & CYD_CHN_ENABLE_YM_ENV))
	{
		// SID style ADSR envelope
	
		switch (chn->envelope_state)
		{
			case SUSTAIN:
			case DONE: return; break;
			
			case ATTACK:
			
			chn->envelope += chn->env_speed;
			
			if (chn->envelope >= 0xff0000) 
			{
				chn->envelope_state = DECAY;
				chn->envelope=0xff0000;
				chn->env_speed = envspd(eng, chn->adsr.d);
			}
			
			break;
			
			case DECAY:
			
				if (chn->envelope > ((Uint32)chn->adsr.s << 19) + chn->env_speed)
					chn->envelope -= chn->env_speed;
				else
				{
					chn->envelope = (Uint32)chn->adsr.s << 19;
					chn->envelope_state = (chn->adsr.s == 0) ? RELEASE : SUSTAIN;
					chn->env_speed = envspd(eng, chn->adsr.r);;
				}
			
			break;
			
			case RELEASE:
			if (chn->envelope > chn->env_speed)
			{
				chn->envelope -= chn->env_speed;
			}
			else 
			{
				chn->envelope_state = DONE;
				if ((chn->flags & (CYD_CHN_ENABLE_WAVE|CYD_CHN_WAVE_OVERRIDE_ENV)) != (CYD_CHN_ENABLE_WAVE|CYD_CHN_WAVE_OVERRIDE_ENV)) chn->flags &= ~CYD_CHN_ENABLE_GATE;
				chn->envelope = 0;
			}
			break;
		}
	}
	else
	{
		// YM2149 style envelope HOLD is not processed
	
		switch (chn->envelope_state)
		{
			case ATTACK:
			
				chn->envelope += chn->env_speed;
				
				if (chn->envelope >= YM_LENGTH) 
				{
					if (chn->ym_env_shape & CYD_YM_ENV_ALT)
					{
						chn->envelope = YM_LENGTH - (chn->envelope- YM_LENGTH);
						chn->envelope_state = DECAY;
					}
					else
					{
						chn->envelope &= YM_LENGTH - 1;
						chn->envelope_state = ATTACK;
					}
				}
			
			break;
			
			case DECAY:
			
				if (chn->envelope >= chn->env_speed) 
					chn->envelope -= chn->env_speed;
				else
				{
					if (chn->ym_env_shape & CYD_YM_ENV_ALT)
					{
						chn->envelope = (Uint32)chn->env_speed - chn->envelope;
						chn->envelope_state = ATTACK;
					}
					else
					{
						chn->envelope -= chn->env_speed;
						chn->envelope &= YM_LENGTH - 1;
						chn->envelope_state = DECAY;		
					}
				}
			
			break;
			
			case RELEASE:
				chn->envelope_state = DONE;
				if ((chn->flags & (CYD_CHN_ENABLE_WAVE|CYD_CHN_WAVE_OVERRIDE_ENV)) != (CYD_CHN_ENABLE_WAVE|CYD_CHN_WAVE_OVERRIDE_ENV)) chn->flags &= ~CYD_CHN_ENABLE_GATE;
				chn->envelope = 0;
			break;
			
			default: break;
		}
	}
}



static void cyd_cycle_channel(CydEngine *cyd, CydChannel *chn)
{
	cyd_cycle_adsr(cyd, chn);
	
	Uint32 prev_acc = chn->accumulator;
	chn->accumulator = (chn->accumulator + (Uint32)chn->frequency);
	chn->sync_bit = chn->accumulator & ACC_LENGTH;
	chn->accumulator &= ACC_LENGTH - 1;
	
	cyd_wave_cycle(cyd, chn);
	
	if ((prev_acc & (ACC_LENGTH/32)) != (chn->accumulator & (ACC_LENGTH/32)))
	{
		Uint32 bit0 = ((chn->random >> 22) ^ (chn->random >> 17)) & 0x1;
		chn->random <<= 1;
		chn->random &= 0x7fffff;
		chn->random |= bit0;
	}
}


static void cyd_sync_channel(CydEngine *cyd, CydChannel *chn)
{
	if (chn->flags & CYD_CHN_ENABLE_SYNC && cyd->channel[chn->sync_source].sync_bit)
	{
		chn->accumulator = 0;
		chn->wave_acc = 0;
		chn->wave_direction = 0;
		chn->random = RANDOM_SEED;
	}
}


static inline Uint32 cyd_pulse(Uint32 acc, Uint32 pw) 
{
	return (((acc >> (ACC_BITS - OUTPUT_BITS - 1)) >= pw ? 0x0fff : 0));
}


static inline Uint32 cyd_saw(Uint32 acc) 
{
	return (acc >> (ACC_BITS - OUTPUT_BITS - 1)) & 0xfff;
}


static inline Uint32 cyd_triangle(Uint32 acc)
{
	return ((((acc & (ACC_LENGTH / 2)) ? ~acc : acc) >> (ACC_BITS - OUTPUT_BITS - 2)) & 0x1fff);
}


static inline Uint32 cyd_noise(Uint32 acc) 
{
	return fastrnd(acc);
}


static Sint16 cyd_output_channel(CydEngine *cyd, CydChannel *chn)
{
	Sint32 v = 0;
	switch (chn->flags & WAVEFORMS & ~CYD_CHN_ENABLE_WAVE)
	{
		case CYD_CHN_ENABLE_PULSE:
		v = cyd_pulse(chn->accumulator, chn->pw);
		break;
		
		case CYD_CHN_ENABLE_SAW:
		v = cyd_saw(chn->accumulator);
		break;
		
		case CYD_CHN_ENABLE_TRIANGLE:
		v = cyd_triangle(chn->accumulator);
		break;
		
		case CYD_CHN_ENABLE_NOISE:
		v = cyd_noise(chn->random);
		break;
		
		case CYD_CHN_ENABLE_TRIANGLE|CYD_CHN_ENABLE_PULSE:
		v = cyd_pulse(chn->accumulator, chn->pw) & cyd_triangle(chn->accumulator);
		break;
		
		case CYD_CHN_ENABLE_SAW|CYD_CHN_ENABLE_PULSE:
		v = cyd_saw(chn->accumulator) & cyd_pulse(chn->accumulator, chn->pw);
		break;
		
		case CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_PULSE:
		v = cyd_noise(chn->random) & cyd_pulse(chn->accumulator, chn->pw);
		break;
		
		case CYD_CHN_ENABLE_TRIANGLE|CYD_CHN_ENABLE_SAW:
		v = cyd_triangle(chn->accumulator) & cyd_saw(chn->accumulator);
		break;
		
		case CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_SAW:
		v = cyd_noise(chn->random) & cyd_saw(chn->accumulator);
		break;
		
		case CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_TRIANGLE:
		v = cyd_noise(chn->random) & cyd_triangle(chn->accumulator);
		break;
		
		case CYD_CHN_ENABLE_TRIANGLE|CYD_CHN_ENABLE_PULSE|CYD_CHN_ENABLE_SAW:
		v = cyd_pulse(chn->accumulator, chn->pw) & cyd_triangle(chn->accumulator) & cyd_saw(chn->accumulator);
		break;
		
		case CYD_CHN_ENABLE_TRIANGLE|CYD_CHN_ENABLE_PULSE|CYD_CHN_ENABLE_NOISE:
		v = cyd_pulse(chn->accumulator, chn->pw) & cyd_triangle(chn->accumulator) & cyd_noise(chn->random);
		break;
		
		case CYD_CHN_ENABLE_TRIANGLE|CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_SAW:
		v = cyd_saw(chn->accumulator) & cyd_triangle(chn->accumulator) & cyd_noise(chn->random);
		break;
		
		case CYD_CHN_ENABLE_PULSE|CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_SAW:
		v = cyd_pulse(chn->accumulator, chn->pw) & cyd_saw(chn->accumulator) & cyd_noise(chn->random);
		break;
		
		case CYD_CHN_ENABLE_NOISE|CYD_CHN_ENABLE_SAW|CYD_CHN_ENABLE_PULSE|CYD_CHN_ENABLE_TRIANGLE:
		v = cyd_saw(chn->accumulator) & cyd_pulse(chn->accumulator, chn->pw) & cyd_triangle(chn->accumulator) & cyd_noise(chn->random);
		break;
		
		default:
		return 0;
		break;
	}
	
	return v - 0x800;
}


Sint32 cyd_env_output(CydEngine *cyd, CydChannel *chn, Sint32 input)
{
	if (chn->flags & CYD_CHN_ENABLE_YM_ENV)
	{
		int idx = chn->envelope * (Uint32)YM_LUT_SIZE / YM_LENGTH;
		return input * cyd->lookup_table_ym[idx] / 32768 * (Sint32)(chn->volume) / MAX_VOLUME;
	}
	else
	{
		if (chn->envelope_state == ATTACK)
			return (input * ((Sint32)chn->envelope / 0x10000) / 256) * (Sint32)(chn->volume) / MAX_VOLUME;
		else
			return (input * (cyd->lookup_table[(chn->envelope / (65536*256 / LUT_SIZE) ) & (LUT_SIZE - 1)]) / 65536) * (Sint32)(chn->volume) / MAX_VOLUME;
	}
}


#ifdef STEREOOUTPUT
static void cyd_output(CydEngine *cyd, Sint32 *left, Sint32 *right)
#else
static Sint32 cyd_output(CydEngine *cyd)
#endif
{
#ifdef STEREOOUTPUT
	*left = *right = 0;
	Sint32 fx_l[CYD_MAX_FX_CHANNELS] = {0}, fx_r[CYD_MAX_FX_CHANNELS] = {0};
#else
	Sint32 v = 0, fx_input[CYD_MAX_FX_CHANNELS] = {0};
#endif
	Sint32 s[CYD_MAX_CHANNELS];
	
	for (int i = 0 ; i < cyd->n_channels ; ++i)
	{
		s[i] = (Sint32)cyd_output_channel(cyd, &cyd->channel[i]);
		
		if ((cyd->channel[i].flags & CYD_CHN_ENABLE_WAVE) && cyd->channel[i].wave_entry && !(cyd->channel[i].flags & CYD_CHN_WAVE_OVERRIDE_ENV))
			s[i] += cyd_wave_get_sample(cyd->channel[i].wave_entry, cyd->channel[i].wave_acc, cyd->channel[i].wave_direction);
	}
	
	for (int i = 0 ; i < cyd->n_channels ; ++i)
	{
		CydChannel *chn = &cyd->channel[i];
		Sint32 o = 0;
		if (chn->flags & CYD_CHN_ENABLE_GATE)
		{
			if (chn->flags & CYD_CHN_ENABLE_RING_MODULATION)
			{
				o = cyd_env_output(cyd, chn, s[i] * (s[chn->ring_mod] + 0x800) / 0x1000);
			}
			else
			{
				o = cyd_env_output(cyd, chn, s[i]);
			}
			
			if ((cyd->channel[i].flags & CYD_CHN_ENABLE_WAVE) && cyd->channel[i].wave_entry && (cyd->channel[i].flags & CYD_CHN_WAVE_OVERRIDE_ENV))
			{
				o += cyd_wave_get_sample(cyd->channel[i].wave_entry, cyd->channel[i].wave_acc, cyd->channel[i].wave_direction) * (Sint32)(chn->volume) / MAX_VOLUME;
			}
			
			if (chn->flags & CYD_CHN_ENABLE_FILTER) 
			{
				cydflt_cycle(&chn->flt, o);
				switch (chn->flttype)
				{
					case FLT_BP: o = cydflt_output_bp(&chn->flt); break;
					default: case FLT_LP: o = cydflt_output_lp(&chn->flt); break;
					case FLT_HP: o = cydflt_output_hp(&chn->flt); break;
				}
			}
			
#ifdef STEREOOUTPUT
			Sint32 ol = o * chn->gain_left / CYD_STEREO_GAIN, or = o * chn->gain_right / CYD_STEREO_GAIN;
#endif		

			if (chn->flags & CYD_CHN_ENABLE_FX)
			{
#ifdef STEREOOUTPUT
				fx_l[chn->fx_bus] += ol;
				fx_r[chn->fx_bus] += or;
#else
				fx_input[chn->fx_bus] += o;
#endif
			}
			else
			{
#ifdef STEREOOUTPUT
				*left += ol;
				*right += or;
#else
				v += o;
#endif		
			}
		}
	}
	
	for (int i = 0 ; i < CYD_MAX_FX_CHANNELS ; ++i)
	{
#ifdef STEREOOUTPUT
		Sint32 l, r;
		cydfx_output(&cyd->fx[i], fx_l[i], fx_r[i], &l, &r);
		*left += l;
		*right += r;
#else
		v += cydfx_output(&cyd->fx[i], fx_input[i]);
#endif
	}
	
#ifndef STEREOOUTPUT
	return v;
#endif
}


static void cyd_cycle(CydEngine *cyd)
{
	for (int i = 0 ; i < cyd->n_channels ; ++i)
	{
		cyd_cycle_channel(cyd, &cyd->channel[i]);
	}
	
	for (int i = 0 ; i < cyd->n_channels ; ++i)
	{
		cyd_sync_channel(cyd, &cyd->channel[i]);
	}
}


void cyd_output_buffer(int chan, void *_stream, int len, void *udata)
{
	CydEngine *cyd = udata;
	Sint16 * stream = _stream;
	cyd->samples_output = 0;
	
	for (int i = 0 ; i < len ; i += sizeof(Sint16), ++stream, ++cyd->samples_output)
	{
	
#ifndef USENATIVEAPIS

#ifndef USESDLMUTEXES
#ifdef DEBUG
		Uint32 waittime = SDL_GetTicks();
#endif
		while (cyd->lock_request) 
		{
#ifdef DEBUG
			if (SDL_GetTicks() - waittime > 5000)
			{
				warning("Deadlock from cyd_output_buffer");
				waittime = SDL_GetTicks();
			}
#endif
			SDL_Delay(1);
		}
#endif

#endif
	
		if (cyd->flags & CYD_PAUSED) continue;
		
		cyd_lock(cyd, 1);
		
		if (cyd->callback && cyd->callback_counter-- == 0)
		{
			cyd->callback_counter = cyd->callback_period-1;
			if (!cyd->callback(cyd->callback_parameter))
			{
				cyd_lock(cyd, 0);
				return;
			}
		}
		
#ifdef STEREOOUTPUT
		Sint32 output, left, right;
		cyd_output(cyd, &left, &right);
		output = (left + right) / 2;
#else
		output = cyd_output(cyd);
#endif

		Sint32 o = (Sint32)*(Sint16*)stream + output * PRE_GAIN;
		
		if (o < -32768) o = -32768;
		else if (o > 32767) o = 32767;
		
		*(Sint16*)stream = o;
		
		cyd_cycle(cyd);

		cyd_lock(cyd, 0);
	}
	
#ifdef ENABLEAUDIODUMP
	if (cyd->dump) fwrite(_stream, len, 1, cyd->dump);
#endif	
}


void cyd_output_buffer_stereo(int chan, void *_stream, int len, void *udata)
{
	CydEngine *cyd = udata;
	Sint16 *stream = _stream;
	cyd->samples_output = 0;
	cyd->flags &= ~CYD_CLIPPING;
	
	for (int i = 0 ; i < len ; i += sizeof(Sint16)*2, stream += 2, ++cyd->samples_output)
	{
#ifndef USENATIVEAPIS
	
#ifndef USESDLMUTEXES
#ifdef DEBUG
		Uint32 waittime = SDL_GetTicks();
#endif
		while (cyd->lock_request) 
		{
#ifdef DEBUG
			if (SDL_GetTicks() - waittime > 5000)
			{
				warning("Deadlock from cyd_output_buffer");
				waittime = SDL_GetTicks();
			}
#endif
			SDL_Delay(1);
		}
#endif

#endif
	
		if (cyd->flags & CYD_PAUSED) continue;
		
		cyd_lock(cyd, 1);
		
		if (cyd->callback && cyd->callback_counter-- == 0)
		{
			cyd->callback_counter = cyd->callback_period-1;
			cyd->callback(cyd->callback_parameter);
		}

		Sint32 left, right;
#ifdef STEREOOUTPUT
		cyd_output(cyd, &left, &right);
#else
		left = right = cyd_output(cyd);
#endif

		Sint32 o1 = (Sint32)*(Sint16*)stream + left * PRE_GAIN;
		
		if (o1 < -32768) 
		{
			o1 = -32768;
			cyd->flags |= CYD_CLIPPING;
		}
		else if (o1 > 32767) 
		{
			o1 = 32767;
			cyd->flags |= CYD_CLIPPING;
		}
		
		*(Sint16*)stream = o1;
		
		Sint32 o2 = (Sint32)*((Sint16*)stream + 1) + right * PRE_GAIN;
		
		if (o2 < -32768) 
		{
			o2 = -32768;
			cyd->flags |= CYD_CLIPPING;
		}
		else if (o2 > 32767) 
		{
			o2 = 32767;
			cyd->flags |= CYD_CLIPPING;
		}
		
		*((Sint16*)stream + 1) = o2;
		
		cyd_cycle(cyd);

		cyd_lock(cyd, 0);
	}
	
#ifdef ENABLEAUDIODUMP
	if (cyd->dump) fwrite(_stream, len, 1, cyd->dump);
#endif	
}


void cyd_set_frequency(CydEngine *cyd, CydChannel *chn, Uint16 frequency)
{
	chn->frequency = (Uint64)ACC_LENGTH/16 * (Uint64)frequency / (Uint64)cyd->sample_rate;
}


void cyd_set_wavetable_frequency(CydEngine *cyd, CydChannel *chn, Uint16 frequency)
{	
	if (chn->wave_entry)
		chn->wave_frequency = (Uint64)WAVETABLE_RESOLUTION * (Uint64)chn->wave_entry->sample_rate / (Uint64)cyd->sample_rate * (Uint64)frequency / (Uint64)get_freq(chn->wave_entry->base_note);
}


void cyd_set_env_frequency(CydEngine *cyd, CydChannel *chn, Uint16 frequency)
{
	chn->env_speed = (Uint64)YM_LENGTH/16 * (Uint64)frequency / (Uint64)cyd->sample_rate;
}


void cyd_set_env_shape(CydChannel *chn, Uint8 shape)
{
	chn->ym_env_shape = shape;
	if (shape & CYD_YM_ENV_ATT)
	{
		chn->envelope = 0;
		chn->envelope_state = ATTACK;
	}
	else
	{
		chn->envelope = YM_LENGTH;
		chn->envelope_state = DECAY;
	}
}


void cyd_enable_gate(CydEngine *cyd, CydChannel *chn, Uint8 enable)
{
	if (enable)
	{
		if (!(chn->flags & CYD_CHN_ENABLE_YM_ENV))
		{
			chn->envelope_state = ATTACK;
			chn->envelope = 0x0;
			chn->env_speed = envspd(cyd, chn->adsr.a);
			cyd_cycle_adsr(cyd, chn);
		}
		
		if (chn->flags & CYD_CHN_ENABLE_KEY_SYNC)
		{
			chn->accumulator = 0;
		}
		
		chn->flags |= CYD_CHN_ENABLE_GATE;
	}
	else
	{
		chn->flags &= ~CYD_CHN_WAVE_OVERRIDE_ENV;
		chn->envelope_state = RELEASE;
		chn->env_speed = envspd(cyd, chn->adsr.r);
	}
}


void cyd_set_waveform(CydChannel *chn, Uint32 wave)
{
	chn->flags = (chn->flags & (~WAVEFORMS)) | (wave & WAVEFORMS);
}


void cyd_set_callback(CydEngine *cyd, int (*callback)(void*), void*param, Uint16 period)
{
	cyd_lock(cyd, 1);
	
	cyd->callback_parameter = param;
	cyd->callback = callback;
	cyd->callback_period = cyd->sample_rate / period;
	cyd->callback_counter = cyd->callback_counter % cyd->callback_period;
	
	cyd_lock(cyd, 0);
}


void cyd_set_callback_rate(CydEngine *cyd, Uint16 period)
{
	cyd_lock(cyd, 1);
	
	cyd->callback_period = cyd->sample_rate / period;
	cyd->callback_counter = cyd->callback_counter % cyd->callback_period;
	
	cyd_lock(cyd, 0);
}


void cyd_pause(CydEngine *cyd, Uint8 enable)
{
	cyd_lock(cyd, 1);
	
	if (enable)
		cyd->flags |= CYD_PAUSED;
	else
		cyd->flags &= ~CYD_PAUSED;
	
	cyd_lock(cyd, 0);
}


int cyd_register(CydEngine * cyd)
{
#ifndef USENATIVEAPIS
	int frequency, channels;
	Uint16 format;
	if (Mix_QuerySpec(&frequency, &format, &channels))
	{
		switch (format)
		{
			case AUDIO_S16SYS:
			break;
			
			default:
			return 0;
			break;
		}
	
		switch (channels)
		{
			case 1: if (!Mix_RegisterEffect(MIX_CHANNEL_POST, cyd_output_buffer, NULL, cyd)) return 0; break;
			case 2: if (!Mix_RegisterEffect(MIX_CHANNEL_POST, cyd_output_buffer_stereo, NULL, cyd)) return 0; break;
			default: return 0; break;
		}
		
		return 1;
	}
	else return 0;
#else
	return 0;
#endif
}


int cyd_unregister(CydEngine * cyd)
{
#ifndef USENATIVEAPIS
	int frequency, channels;
	Uint16 format;
	if (Mix_QuerySpec(&frequency, &format, &channels))
	{
		switch (channels)
		{
			case 1: if (!Mix_UnregisterEffect(MIX_CHANNEL_POST, cyd_output_buffer)) return 0; break;
			case 2: if (!Mix_UnregisterEffect(MIX_CHANNEL_POST, cyd_output_buffer_stereo)) return 0; break;
			default: return 0; break;
		}
		
		cyd_lock(cyd, 1);
		cyd_lock(cyd, 0);
		
		return 1;
	}
	else return 0;
#else
	return 0;
#endif
}


void cyd_set_filter_coeffs(CydEngine * cyd, CydChannel *chn, Uint16 cutoff, Uint8 resonance)
{
	static const Uint16 resonance_table[] = {10, 512, 1300, 1950};
	cydflt_set_coeff(&chn->flt, cutoff, resonance_table[resonance & 3]);
}


void cyd_lock(CydEngine *cyd, Uint8 enable)
{
	if (cyd->flags & CYD_SINGLE_THREAD) return; // For export, mainly

#ifndef USENATIVEAPIS

#ifndef USESDLMUTEXES
	if (enable)
	{
#ifdef DEBUG
		Uint32 waittime = SDL_GetTicks();
#endif
		cyd->lock_request = 1;
		while (cyd->lock_locked )
		{
#ifdef DEBUG
			if (SDL_GetTicks() - waittime > 5000)
			{
				warning("Deadlock from cyd_lock");
				waittime = SDL_GetTicks();
			}
#endif
			SDL_Delay(1);
		}
	}
	else
	{
		cyd->lock_request = 0;
		while (cyd->lock_locked)
		{
			SDL_Delay(1);
		}
	}	
#else	
	if (enable)
	{
		SDL_mutexP(cyd->mutex);
	}
	else
	{
		SDL_mutexV(cyd->mutex);
	}
#endif
#else

# ifdef WIN32
	if (enable)
	{
		EnterCriticalSection(&cyd->mutex);
	}
	else
	{
		LeaveCriticalSection(&cyd->mutex);
	}
# endif

#endif
}

#ifdef ENABLEAUDIODUMP
void cyd_enable_audio_dump(CydEngine *cyd)
{
	cyd_lock(cyd, 1);
	
	char fn[100];
	sprintf(fn, "cyd-dump-%d-%u.raw", cyd->sample_rate, (Uint32)time(NULL));
	
	cyd->dump = fopen(fn, "wb");
	
	cyd_lock(cyd, 0);
}

void cyd_disable_audio_dump(CydEngine *cyd)
{
	if (cyd->dump) fclose(cyd->dump);
	cyd->dump = NULL;
}
#endif

#ifdef STEREOOUTPUT
void cyd_set_panning(CydEngine *cyd, CydChannel *chn, Uint8 panning)
{
	if (chn->panning == panning) return;
	
	chn->panning = my_min(CYD_PAN_RIGHT, my_max(CYD_PAN_LEFT, panning));
	float a = M_PI / 2 * (float)(chn->panning - CYD_PAN_LEFT) / (CYD_PAN_RIGHT - CYD_PAN_LEFT);
	chn->gain_left = cos(a) * CYD_STEREO_GAIN;
	chn->gain_right = sin(a) * CYD_STEREO_GAIN;
}
#endif


void cyd_set_wave_entry(CydChannel *chn, const CydWavetableEntry * entry)
{
	chn->wave_entry = entry;
	chn->wave_acc = 0;
	chn->wave_frequency = 0;
	chn->wave_direction = 0;
}


void cyd_set_wavetable_offset(CydChannel *chn, Uint16 offset /* 0..0x1000 = 0-100% */)
{
	if (chn->wave_entry)
	{
		chn->wave_acc = (Uint64)offset * WAVETABLE_RESOLUTION * chn->wave_entry->samples / 0x1000;
	}
}
