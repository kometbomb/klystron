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

#include "cydfx.h"
#include "cyd.h"

#define KRUSH ~0x3f

#ifdef STEREOOUTPUT
void cydfx_output(CydFx *fx, Sint32 fx_l, Sint32 fx_r, Sint32 *left, Sint32 *right)
{
	*left = fx_l;
	*right = fx_r;
#else
Sint32 cydfx_output(CydFx *fx, Sint32 fx_input)
{
	Sint32 v = fx_input;
#endif
	if (fx->flags & CYDFX_ENABLE_REVERB)
	{
#ifdef STEREOOUTPUT
		cydrvb_cycle(&fx->rvb, fx_l, fx_l);
		cydrvb_output(&fx->rvb, &fx_l, &fx_r);
		*left += fx_l;
		*right += fx_r;
#else
		cydrvb_cycle(&fx->rvb, fx_input);
		v = cydrvb_output(&fx->rvb);
#endif
	}
	
	if (fx->flags & CYDFX_ENABLE_CRUSH)
	{
#ifdef STEREOOUTPUT
		*left = *left & KRUSH;
		*right = *right & KRUSH;
#else
		v = v & KRUSH;
#endif
	}
	
#ifndef STEREOOUTPUT
	return v;
#endif
}


void cydfx_init(CydFx *fx, int rate)
{
	cydrvb_init(&fx->rvb, rate);
}


void cydfx_deinit(CydFx *fx)
{
	cydrvb_deinit(&fx->rvb);
}


void cydfx_set(CydFx *fx, const CydFxSerialized *ser)
{
	cydrvb_set_stereo_spread(&fx->rvb, ser->rvb.spread);
	
	for (int i = 0 ; i < CYDRVB_TAPS ; ++i)
	{
		cydrvb_set_tap(&fx->rvb, i, ser->rvb.tap[i].delay, ser->rvb.tap[i].gain);
	}
	
	fx->flags = ser->flags;
}
