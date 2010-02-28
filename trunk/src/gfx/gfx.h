#ifndef GFX_H
#define GFX_H

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


#include "SDL.h"
#include "SDL_rwops.h"
#include "tiledescriptor.h"

enum
{
	GFX_KEYED = 1,
	GFX_ALPHA = 2,
	GFX_TRANS_HALF = 4
};

typedef enum
{
	GFX_SCALE_FAST,
	GFX_SCALE_SMOOTH
} GfxScaleType;

#ifdef WIN32
typedef Uint64 FramerateTimer;
#else
typedef Uint32 FramerateTimer;
#endif

typedef struct
{
	SDL_Surface *screen, *buf, *buf2;
	int screen_w, screen_h;
	int scale, fullscreen, fps;
	GfxScaleType scale_type;
	FramerateTimer last_ticks, frame_time, clock_resolution;
} GfxDomain;

SDL_Surface* gfx_load_surface(const char* filename, const int flags);
SDL_Surface* gfx_load_surface_RW(SDL_RWops *rw, const int flags);
void gfx_blit_2x(SDL_Surface *dest, SDL_Surface *src);
void gfx_blit_2x_resample(SDL_Surface *dest, SDL_Surface *src);
void gfx_blit_3x(SDL_Surface *dest, SDL_Surface *src);
void gfx_blit_3x_resample(SDL_Surface *dest, SDL_Surface *src);
void gfx_blit_4x(SDL_Surface *dest, SDL_Surface *src);
void gfx_raster(SDL_Surface *dest, const SDL_Rect *rect, const Uint32 *colors, int len);
void gfx_generate_raster(Uint32 *dest, const Uint32 from, const Uint32 to, int len);
TileDescriptor *gfx_build_tiledescriptor(SDL_Surface *tiles, const int cellwidth, const int cellheight) ;
void gfx_line_unclipped(SDL_Surface *dest, int x0, int y0, int x1, int y1, Uint32 color);
void gfx_line(SDL_Surface *dest, int x0, int y0, int x1, int y1, Uint32 color);
void gfx_circle_inverted(SDL_Surface *dest, const int xc, const int yc, const int r, const Uint32 color);
void gfx_circle(SDL_Surface *dest, const int xc, const int yc, const int r, const Uint32 color);

GfxDomain * gfx_create_domain();
void gfx_domain_update(GfxDomain *domain);
SDL_Surface *gfx_domain_get_surface(GfxDomain *domain);
void gfx_domain_flip(GfxDomain *domain);
void gfx_domain_free(GfxDomain *domain);
int gfx_domain_is_next_frame(GfxDomain *domain);

#endif
