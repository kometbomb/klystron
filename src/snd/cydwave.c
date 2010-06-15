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

Sint32 cyd_wave_get_sample(const CydWavetableEntry *entry, Uint64 wave_acc)
{
	if (entry->data)
	{
		return entry->data[wave_acc / ACC_LENGTH];
	}
	else
		return 0;
}


void cyd_wave_cycle(CydEngine *cyd, CydChannel *chn)
{
	if (chn->wave_entry && (chn->flags & CYD_CHN_ENABLE_WAVE))
	{
		chn->wave_acc += chn->wave_frequency;
		
		if (chn->wave_entry->flags & CYD_WAVE_LOOP)
		{
			if (chn->wave_acc >= (Uint64)chn->wave_entry->loop_end * ACC_LENGTH)
			{
				chn->wave_acc = chn->wave_acc - (Uint64)chn->wave_entry->loop_end * ACC_LENGTH + (Uint64)chn->wave_entry->loop_begin * ACC_LENGTH;
			}
		}
		else
		{
			if (chn->wave_acc >= (Uint64)chn->wave_entry->samples * ACC_LENGTH)
			{
				// stop playback
				chn->wave_entry = NULL;
			}
		}
	}
}
