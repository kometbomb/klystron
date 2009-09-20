#ifndef FONT_H
#define FONT_H

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


#include "tiledescriptor.h"
#include "../util/bundle.h"

#include <stdio.h>

typedef struct
{
	char *charmap;
	TileDescriptor *tiledescriptor;
	SDL_Surface * surface;
	int w, h;
} Font;

int font_load(Font *font, Bundle *b, char *name);
void font_create(Font *font, SDL_Surface *tiles, const int w, const int h, char *charmap);
void font_destroy(Font *font);
void font_write_cursor(Font *font, SDL_Surface *dest, const SDL_Rect *r, Uint16 *cursor, const char * text, ...) __attribute__ ((format (printf, 5, 6)));
void font_write(Font *font, SDL_Surface *dest, const SDL_Rect *r, const char * text, ...) __attribute__ ((format (printf, 4, 5)));

#endif
