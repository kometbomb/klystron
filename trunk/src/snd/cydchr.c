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

#include "cydchr.h"
#include <math.h>

void cydchr_output(CydChorus *chr, Sint32 in_l, Sint32 in_r, Sint32 *out_l, Sint32 *out_r)
{
	++chr->pos_l;
	if (chr->pos_l >= chr->lut_size)
		chr->pos_l = 0;

	++chr->pos_r;
	if (chr->pos_r >= chr->lut_size)
		chr->pos_r = 0;
		
	++chr->pos_buf;
	if (chr->pos_buf >= chr->buf_size)
		chr->pos_buf = 0;
		
	chr->buffer[chr->pos_buf] = in_r;
	chr->buffer[chr->pos_buf + chr->buf_size] = in_r;
	
	if (chr->lut_size)
		*out_l = chr->buffer[(chr->pos_buf - chr->lut[chr->pos_l] + chr->buf_size)];
	else
		*out_l = in_l;
		
	*out_r = chr->buffer[(chr->pos_buf - chr->lut[chr->pos_r] + chr->buf_size)];
}


void cydchr_set(CydChorus *chr, int rate, int min_delay, int max_delay, int stereo_separation)
{
	if (rate)
	{
		int old = chr->lut_size;
		chr->lut_size = chr->sample_rate * 4 * 10 / (10 + (rate - 1));
		
		chr->pos_l = 0;
		chr->pos_r = (stereo_separation * chr->lut_size / 2 / 64) % chr->lut_size;
		
		if (old == chr->lut_size && min_delay == chr->min_delay && chr->max_delay == max_delay) return;
		
		chr->min_delay = min_delay;
		chr->max_delay = max_delay;
		
		for (int i = 0 ; i < chr->lut_size ; ++i)
			chr->lut[i] = (int)(((sin((double)i / chr->lut_size * M_PI * 2) * 0.5 + 0.5) * (max_delay - min_delay) + min_delay) * chr->sample_rate / 10000) % chr->lut_size;
	}
	else
	{
		chr->pos_l = 0;
		chr->pos_r = 0;
		chr->lut_size = 0;
		chr->lut[0] = chr->sample_rate * min_delay / 10000;
	}
}


void cydchr_init(CydChorus *chr, int sample_rate)
{	
	memset(chr, 0, sizeof(*chr));
	chr->sample_rate = sample_rate;
	chr->buf_size = sample_rate * CYDCHR_SIZE / 1000;
	chr->buffer = calloc(chr->buf_size, sizeof(chr->buffer[0]) * 2);
	chr->lut = calloc(sample_rate * 4, sizeof(chr->buffer[0]));
	chr->lut_size = 0;
}


void cydchr_deinit(CydChorus *chr)
{
	free(chr->buffer);
	free(chr->lut);
}

