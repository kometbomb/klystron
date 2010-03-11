#ifndef CYDFX_H
#define CYDFX_H

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

#include "cydrvb.h"

typedef struct
{
	Uint32 flags;
	CydReverb rvb;
} CydFx;

#ifdef STEREOOUTPUT
void cydfx_output(CydFx *fx, Sint32 fx_l, Sint32 fx_r, Sint32 *left, Sint32 *right);
#else
Sint32 cydfx_output(CydFx *fx, Sint32 fx_input);
#endif
void cydfx_init(CydFx *fx, int rate);
void cydfx_deinit(CydFx *fx);

enum
{
	CYDFX_ENABLE_REVERB = 1,
	CYDFX_ENABLE_CRUSH = 2
};

#endif
