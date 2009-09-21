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


static void inner_write(Font *font, SDL_Surface *dest, const SDL_Rect *r, Uint16 * cursor, SDL_Rect *bounds, const char * text)
{
	const char *c = text;
	int x = (*cursor & 0xff) * font->h, y = ((*cursor >> 8) & 0xff) * font->h, cr = 0, right = dest->w;
	
	if (r)
	{
		x = x + (cr = r->x);
		y = r->y + y;
		right = r->w + r->x;
	}
	
	int prev_x = x, prev_y = y;
	
	if (bounds && bounds->w == 0)
	{
		bounds->x = x;
		bounds->y = y;
	}
	
	for (;*c;++c)
	{
		if (*c == '\n' || x >= right)
		{
			y += font->h;
			x = cr;
			*cursor &= (Uint16)0xff00;
			*cursor += 0x0100;
			
			prev_y = y;
			
			if (*c == '\n') 
				continue;
		}
		
		if (*c == ' ')
		{
			x += font->w;
			++*cursor;
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
			
			++*cursor;
		}
		
		if (bounds)
		{
			bounds->x = my_min(prev_x, bounds->x);
			bounds->y = my_min(prev_y, bounds->y);
			bounds->w = my_max(x - bounds->x, bounds->w);
			bounds->h = my_max(y - bounds->y + font->h, bounds->h);
		}
		
		prev_x = x;
	}
}

static void font_write_va(Font *font, SDL_Surface *dest, const SDL_Rect *r, Uint16 * cursor, SDL_Rect *bounds, const char * text, va_list va)
{
	int len = vsnprintf(NULL, 0, text, va) + 1;
	char * formatted = malloc(len * sizeof(*formatted));
	vsnprintf(formatted, len, text, va);
	
	inner_write(font, dest, r, cursor, bounds, formatted);
	
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
					strcpy(map, "ABCDEFGHIJKLMNOPQRSTUVWXYZ≈ƒ÷…abcdefghijklmnopqrstuvwxyzÂ‰ˆÈ!\"%&/()=?+-_.:,;`~[]{}*'<>    0123456789\\|^@");
					debug("Charmap not found in font file");
				}
			}
			
			int w, h;
			
			{
				SDL_RWops *rw = SDL_RWFromBundle(&fb, "res.txt");
				char res[10];
				
				if (rw)
				{
					rw->read(rw, res, 1, sizeof(res)-1);
					SDL_FreeRW(rw);
					
					sscanf(res, "%d %d", &w, &h);
				}
				else
				{
					w = 8;
					h = 9;
				}
			}
			
			font_create(font, s, w, h, map);
		
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


void font_write_cursor_args(Font *font, SDL_Surface *dest, const SDL_Rect *r, Uint16 *cursor, SDL_Rect *bounds, const char * text, ...)
{
	va_list va;
	va_start(va, text);
	
	font_write_va(font, dest, r, cursor, bounds, text, va);
	
	va_end(va);
}


void font_write_cursor(Font *font, SDL_Surface *dest, const SDL_Rect *r, Uint16 *cursor, SDL_Rect *bounds, const char * text)
{
	inner_write(font, dest, r, cursor, bounds, text);
}


void font_write(Font *font, SDL_Surface *dest, const SDL_Rect *r, const char * text)
{
	inner_write(font, dest, r, NULL, NULL, text);
}


void font_write_args(Font *font, SDL_Surface *dest, const SDL_Rect *r, const char * text, ...)
{
	Uint16 cursor = 0;
	
	va_list va;
	va_start(va, text);
	
	font_write_va(font, dest, r, &cursor, NULL, text, va);
	
	va_end(va);
}
