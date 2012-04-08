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

#include "cydwave.h"
#include "cyddefs.h"
#include "macros.h"

#ifndef LOWRESWAVETABLE
typedef Sint64 WaveAccSigned;
#else
typedef Sint32 WaveAccSigned;
#endif

static Sint32 cyd_wave_get_sample_no_interpolation(const CydWavetableEntry *entry, CydWaveAcc wave_acc, int direction)
{
#ifndef CYD_DISABLE_WAVETABLE
	if (entry->data)
	{	
		return entry->data[wave_acc / WAVETABLE_RESOLUTION];
	}
	else
		return 0;
#else
	return 0;
#endif // CYD_DISABLE_WAVETABLE
}


static Sint32 cyd_wave_get_sample_linear(const CydWavetableEntry *entry, CydWaveAcc wave_acc, int direction)
{
#ifndef CYD_DISABLE_WAVETABLE
	if (entry->data)
	{	
		if (direction == 0) 
		{
			int a = wave_acc / WAVETABLE_RESOLUTION;
			int b = a + 1;
			
			if ((entry->flags & CYD_WAVE_LOOP) && b >= entry->loop_end)
			{
				if (!(entry->flags & CYD_WAVE_PINGPONG))
					b = b - entry->loop_end + entry->loop_begin;
				else
					b = entry->loop_end - (b - entry->loop_end);
			}
			
			if (b >= entry->samples)
				return entry->data[a];
			else
				return entry->data[a] + (entry->data[b] - entry->data[a]) * (wave_acc % WAVETABLE_RESOLUTION) / WAVETABLE_RESOLUTION;
		}
		else
		{
			int a = wave_acc / WAVETABLE_RESOLUTION;
			int b = a - 1;
			
			if ((entry->flags & CYD_WAVE_LOOP) && b < (Sint32)entry->loop_begin)
			{
				if (!(entry->flags & CYD_WAVE_PINGPONG))
					b = b - entry->loop_begin + entry->loop_end;
				else
					b = entry->loop_begin - (b - entry->loop_begin);
			}
			
			if (b < 0)
				return entry->data[a];
			else
				return entry->data[a] + (entry->data[b] - entry->data[a]) * (WAVETABLE_RESOLUTION - (wave_acc % WAVETABLE_RESOLUTION)) / WAVETABLE_RESOLUTION;
		}
	}
	else
		return 0;
		
#else 	
	return 0;
#endif // CYD_DISABLE_WAVETABLE
}


Sint32 cyd_wave_get_sample(const CydWavetableEntry *entry, CydWaveAcc wave_acc, int direction)
{
#ifndef CYD_DISABLE_WAVETABLE

	if (entry->flags & CYD_WAVE_NO_INTERPOLATION)
	{
		return cyd_wave_get_sample_no_interpolation(entry, wave_acc, direction);
	}
	else
	{
		return cyd_wave_get_sample_linear(entry, wave_acc, direction);
	}
	
#else
	return 0;
#endif // CYD_DISABLE_WAVETABLE
}


void cyd_wave_cycle(CydEngine *cyd, CydChannel *chn)
{
#ifndef CYD_DISABLE_WAVETABLE

	if (chn->wave_entry && (chn->flags & CYD_CHN_ENABLE_WAVE))
	{
		if (chn->wave_direction == 0)
		{
			chn->wave_acc += chn->wave_frequency;
			
			if ((chn->wave_entry->flags & CYD_WAVE_LOOP) && chn->wave_entry->loop_end != chn->wave_entry->loop_begin)
			{
				if (chn->wave_acc >= (Uint64)chn->wave_entry->loop_end * WAVETABLE_RESOLUTION)
				{
					if (chn->wave_entry->flags & CYD_WAVE_PINGPONG) 
					{
						chn->wave_acc = (Uint64)chn->wave_entry->loop_end * WAVETABLE_RESOLUTION - (chn->wave_acc - (Uint64)chn->wave_entry->loop_end * WAVETABLE_RESOLUTION);
						chn->wave_direction = 1;
					}
					else
					{
						chn->wave_acc = chn->wave_acc - (Uint64)chn->wave_entry->loop_end * WAVETABLE_RESOLUTION + (Uint64)chn->wave_entry->loop_begin * WAVETABLE_RESOLUTION;
					}
				}
			}
			else
			{
				if (chn->wave_acc >= (Uint64)chn->wave_entry->samples * WAVETABLE_RESOLUTION)
				{
					// stop playback
					chn->wave_entry = NULL;
				}
			}
		}
		else
		{
			chn->wave_acc -= chn->wave_frequency;
			
			if ((chn->wave_entry->flags & CYD_WAVE_LOOP) && chn->wave_entry->loop_end != chn->wave_entry->loop_begin)
			{
				if ((WaveAccSigned)chn->wave_acc < (WaveAccSigned)chn->wave_entry->loop_begin * WAVETABLE_RESOLUTION)
				{
					if (chn->wave_entry->flags & CYD_WAVE_PINGPONG) 
					{
						chn->wave_acc = (WaveAccSigned)chn->wave_entry->loop_begin * WAVETABLE_RESOLUTION - ((WaveAccSigned)chn->wave_acc - (WaveAccSigned)chn->wave_entry->loop_begin * WAVETABLE_RESOLUTION);
						chn->wave_direction = 0;
					}
					else
					{
						chn->wave_acc = chn->wave_acc - (Uint64)chn->wave_entry->loop_begin * WAVETABLE_RESOLUTION + (Uint64)chn->wave_entry->loop_end * WAVETABLE_RESOLUTION;
					}
				}
			}
			else
			{
				if ((WaveAccSigned)chn->wave_acc < 0)
				{
					// stop playback
					chn->wave_entry = NULL;
				}
			}
		}
	}
	
#endif // CYD_DISABLE_WAVETABLE
}
