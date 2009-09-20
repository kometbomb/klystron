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


#include "font.h"
#include "gfx.h"
#include <string.h>

char * strdup(const char *);

void font_create(Font *font, SDL_Surface *tiles, const int w, const int h, char *charmap)
{
	font->tiledescriptor = gfx_build_tiledescriptor(tiles,w,h);
	font->charmap = strdup(charmap);
	font->w = w;
	font->h = h;
}


void font_destroy(Font *font)
{
	free(font->tiledescriptor);
	free(font->charmap);
	if (font->surface) SDL_FreeSurface(font->surface);
}


static const TileDescriptor * findchar(const Font *font, char c)
{
	char *tc;
	const TileDescriptor *tile;
	for (tile = font->tiledescriptor, tc = font->charmap; *tc ; ++tile, ++tc)
		if (*tc == c) return tile;
	debug("Could not find character '%c'", c);
	return NULL;
}


void font_write(Font *font, SDL_Surface *dest, const SDL_Rect *r, const char * text, ...)
{
	va_list va;
	va_start(va, text);
	
	int len = vsnprintf(NULL, 0, text, va) + 1;
	char * formatted = malloc(len * sizeof(*formatted));
	vsnprintf(formatted, len, text, va);
	
	va_end(va);
	
	const char *c = formatted;
	int x = 0, y = 0, cr = 0, right = dest->w;
	
	if (r)
	{
		cr = x = r->x;
		y = r->y;
		right = r->w + r->x;
	}
	
	while (*c)
	{
		if (x >= right) --c;
		
		if (*c == '\n' || x >= right)
		{
			y += font->h;
			x = cr;
		}
		else if (*c == ' ')
		{
			x += font->w;
		}
		else
		{
			const TileDescriptor *tile = findchar(font, *c);
			
			if (tile)
			{
				SDL_Rect rect = { x, y, tile->rect.w, tile->rect.h };
				SDL_BlitSurface(tile->surface, (SDL_Rect*)&tile->rect, dest, &rect);
				
				x += tile->rect.w;
			}
			else
			{
				x += font->w;
			}
		}
		
		++c;
	}
	
	free(formatted);
}


int font_load(Font *font, Bundle *bundle, char *name)
{
	debug("Loading font '%s'",name);

	Bundle fb;
	FILE *f = bnd_locate(bundle, name, 0);
	
	if (f && bnd_open_file(&fb, f, bundle->path))
	{
		SDL_RWops *rw = SDL_RWFromBundle(&fb, "font.bmp");
		if (rw)
		{
			SDL_Surface * s= gfx_load_surface_RW(rw, GFX_KEYED);
			SDL_FreeRW(rw);
			
			char map[1000];
			memset(map, 0, sizeof(map));
			
			{
				SDL_RWops *rw = SDL_RWFromBundle(&fb, "charmap.txt");
				if (rw)
				{
					rw->read(rw, map, 1, sizeof(map)-1);
					SDL_FreeRW(rw);
				}
				else
				{
					strcpy(map, "ABCDEFGHIJKLMNOPQRSTUVWXYZ����abcdefghijklmnopqrstuvwxyz����!\"%&/()=?+-_.:,;`~[]{}*'<>    0123456789\\|^@");
					debug("Charmap not found in font file");
				}
			}
			
			font_create(font, s, 8, 9, map);
		
			font->surface = s;
			
			fclose(f);
			bnd_free(&fb);
			
			return 1;
		}
		else
		{
			fclose(f);
			bnd_free(&fb);
			
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

