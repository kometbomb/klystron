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


#include "gfx.h"
#include <math.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// gives the optimizer an opportunity to vectorize the second loop

#define vechelper(width, block)\
	{\
		const int w = width & 0xffffff00;\
		int x;\
		for (x = 0 ; x < w ; ++x)\
		{\
			block;\
		}\
		for ( ; x < width ; ++x)\
		{\
			block;\
		}\
	}

SDL_Surface* gfx_load_surface(const char* filename, const int flags)
{
	FILE *f = fopen(filename, "rb");
	if (f)
	{
		SDL_RWops * rw = SDL_RWFromFP(f, 1);
		SDL_Surface * s = gfx_load_surface_RW(rw, flags);
		return s;
	}
	else
	{
		warning("Loading surface '%s' failed", filename);
		return NULL;
	}
}


SDL_Surface* gfx_load_surface_RW(SDL_RWops *rw, const int flags)
{
	SDL_Surface* loaded = SDL_LoadBMP_RW(rw, 1);
	
	if (!loaded) 
	{
		warning("Loading surface failed");
		return NULL;
	}
	
	if (flags & GFX_KEYED)
	{
		SDL_Surface* optimal = NULL;
		SDL_SetColorKey(loaded, SDL_RLEACCEL | SDL_SRCCOLORKEY,
			SDL_MapRGB(loaded->format, 255, 0, 255));
		optimal = SDL_DisplayFormat(loaded);
		
		if (!optimal)
		{
			warning("Conversion failed %dx%d", loaded->w, loaded->h);
			return loaded;
		}
		
		if (optimal && (flags & GFX_TRANS_HALF))
		{
			SDL_SetAlpha(optimal, SDL_RLEACCEL | SDL_SRCALPHA, 128);  
		}
		
		SDL_FreeSurface(loaded);
		
		return optimal;
	}
	else if (flags & GFX_ALPHA)
	{
		SDL_Surface* optimal = NULL;
		SDL_SetAlpha(loaded, SDL_RLEACCEL | SDL_SRCALPHA, (flags & GFX_TRANS_HALF)?128:SDL_ALPHA_OPAQUE);  
		optimal = SDL_DisplayFormatAlpha(loaded);
		
		if (!optimal)
		{
			warning("Conversion failed %dx%d", loaded->w, loaded->h);
			return loaded;
		}
		
		SDL_FreeSurface(loaded);
		return optimal;
	}
	else
	{
		return loaded;
	}
	
}


void gfx_blit_2x(SDL_Surface *dest, SDL_Surface *src)
{
	
	for (int y = 0 ; y < src->h ; y ++)
	{
		unsigned int *dptr1 = dest->pixels+dest->pitch*(y*2);
		unsigned int *dptr2 = dest->pixels+dest->pitch*(y*2+1);
		unsigned int *sptr = src->pixels+src->pitch*y;
		for (int x = src->w ; x != 0  ; --x)
		{
			*(dptr1++) = *sptr;
			*(dptr1++) = *sptr;
			*(dptr2++) = *sptr;
			*(dptr2++) = *(sptr++);
		}
	}
}


void gfx_blit_3x(SDL_Surface *dest, SDL_Surface *src)
{
	
	for (int y = 0 ; y < src->h ; y ++)
	{
		unsigned int *dptr1 = dest->pixels+dest->pitch*(y*3);
		unsigned int *dptr2 = dest->pixels+dest->pitch*(y*3+1);
		unsigned int *dptr3 = dest->pixels+dest->pitch*(y*3+2);
		unsigned int *sptr = src->pixels+src->pitch*y;
		for (int x = src->w ; x != 0  ; --x)
		{
			*(dptr1++) = *sptr;
			*(dptr1++) = *sptr;
			*(dptr1++) = *sptr;
			*(dptr2++) = *sptr;
			*(dptr2++) = *sptr;
			*(dptr2++) = *sptr;
			*(dptr3++) = *sptr;
			*(dptr3++) = *sptr;
			*(dptr3++) = *(sptr++);
		}
	}
}


void gfx_blit_4x(SDL_Surface *dest, SDL_Surface *src)
{
	
	for (int y = 0 ; y < src->h ; y ++)
	{
		unsigned int *dptr1 = dest->pixels+dest->pitch*(y*4);
		unsigned int *dptr2 = dest->pixels+dest->pitch*(y*4+1);
		unsigned int *dptr3 = dest->pixels+dest->pitch*(y*4+2);
		unsigned int *dptr4 = dest->pixels+dest->pitch*(y*4+3);
		unsigned int *sptr = src->pixels+src->pitch*y;
		for (int x = src->w ; x != 0  ; --x)
		{
			*(dptr1++) = *sptr;
			*(dptr1++) = *sptr;
			*(dptr1++) = *sptr;
			*(dptr1++) = *sptr;
			*(dptr2++) = *sptr;
			*(dptr2++) = *sptr;
			*(dptr2++) = *sptr;
			*(dptr2++) = *sptr;
			*(dptr3++) = *sptr;
			*(dptr3++) = *sptr;
			*(dptr3++) = *sptr;
			*(dptr3++) = *sptr;
			*(dptr4++) = *sptr;
			*(dptr4++) = *sptr;
			*(dptr4++) = *sptr;
			*(dptr4++) = *(sptr++);
		}
	}
}


void gfx_raster(SDL_Surface *dest, const SDL_Rect *rect, const Uint32 *colors, int len)
{
	for (int y = rect ? rect->y : 0, i = 0 ; y < (rect ? rect->h+rect->y : dest->h) && i < len ; ++i, ++y)
	{
		SDL_Rect r = {rect ? rect->x : 0, y, rect ? rect->w : dest->w, 1};
		SDL_FillRect(dest, &r, colors[i]);
	}
}


void gfx_generate_raster(Uint32 *dest, const Uint32 from, const Uint32 to, int len)
{
	for (int i = 0 ; i < len ; ++i)
	{
		int from_r = ((Uint8*)&from)[0];
		int from_g = ((Uint8*)&from)[1];
		int from_b = ((Uint8*)&from)[2];
		int to_r = ((Uint8*)&to)[0];
		int to_g = ((Uint8*)&to)[1];
		int to_b = ((Uint8*)&to)[2];
		
		*(dest++) = ((from_r + (to_r-from_r) * i / len) & 0xff)
					| (((from_g + (to_g-from_g) * i / len) & 0xff) << 8)
					| (((from_b + (to_b-from_b) * i / len) & 0xff) << 16)
					;
	}
}


static int has_pixels(TileDescriptor *desc)
{
	my_lock(desc->surface);
	
	int result = 0;
	
	for (int y = 0 ; y < desc->rect.h ; ++y)
	{
		Uint8 *p = (Uint8 *)desc->surface->pixels + ((int)desc->rect.y + y) * desc->surface->pitch + (int)desc->rect.x * desc->surface->format->BytesPerPixel;
		
		for (int x = 0 ; x < desc->rect.w ; ++x)
		{
			//printf("%08x", *(Uint32*)p);
			if ((*((Uint32*)p)&0xffffff) != desc->surface->format->colorkey)
			{
				++result;
			}
			
			p+=desc->surface->format->BytesPerPixel;
		}
	}
	
	my_unlock(desc->surface);
	
	return result;
}


TileDescriptor *gfx_build_tiledescriptor(SDL_Surface *tiles, const int cellwidth, const int cellheight) 
{
	TileDescriptor *descriptor = calloc(sizeof(*descriptor), (tiles->w/cellwidth)*(tiles->h/cellheight));
	for (int i = 0 ; i < (tiles->w/cellwidth)*(tiles->h/cellheight) ; ++i) 
	{
		descriptor[i].rect.x = i*cellwidth;
		descriptor[i].rect.y = 0; 
		
		while (descriptor[i].rect.x >= tiles->w)
		{
			descriptor[i].rect.y += cellheight;
			descriptor[i].rect.x -= tiles->w;
		}
		
		descriptor[i].rect.w = cellwidth;
		descriptor[i].rect.h = cellheight;
		descriptor[i].surface = tiles;
		
		int pixels = has_pixels(&descriptor[i]);
		
		if (pixels == CELLSIZE*CELLSIZE)
			descriptor[i].flags = TILE_COL_NORMAL;
		else if (pixels > 0)
			descriptor[i].flags = TILE_COL_PIXEL;
		else descriptor[i].flags = TILE_COL_DISABLE;
	}
	
	return descriptor;
}


static void plot(SDL_Surface *dest, int x0, int y0, const Uint32 color)
{
	Uint32 *ptr = dest->pixels + x0 * 4 + y0 * dest->pitch;
	*ptr = color;
}


#define SWAP(x,y) {int t = x; x = y; y=t;}

static inline int delta(int e, int x, int y)
{
	if (y == 0) return 0;
	return e * x / y;
}

void gfx_line(SDL_Surface *dest, int x0, int y0, int x1, int y1, Uint32 color)
{
	if (x0 < 0)
	{
		y0 = y0 + (0 - x0) * (y1-y0)/(x1-x0);
		x0 = 0;
	}

	if (x0 >= dest->w)
	{
		y0 = y0 + delta((dest->w-1 - x0), (y1-y0),(x1-x0));
		x0 = dest->w-1;
	}
	
	if (x1 < 0)
	{
		y1 = y1 + delta((0 - x1), (y1-y0),(x1-x0));
		x1 = 0;
	}

	if (x1 >= dest->w)
	{
		y1 = y1 + delta((dest->w-1 - x1), (y1-y0),(x1-x0));
		x1 = dest->w-1;
	}
	
	if (y0 < 0)
	{
		x0 = x0 + delta((0 - y0), (x1-x0),(y1-y0));
		y0 = 0;
	}

	if (y0 >= dest->h)
	{
		x0 = x0 + delta((dest->h-1 - y0), (x1-x0),(y1-y0));
		y0 = dest->h-1;
	}
	
	if (y1 < 0)
	{
		x1 = x1 + delta((0 - y1), (x1-x0),(y1-y0));
		y1 = 0;
	}

	if (y1 >= dest->h)
	{
		x1 = x1 + delta((dest->h-1 - y1), (x1-x0),(y1-y0));
		y1 = dest->h-1;
	}
	
	gfx_line_unclipped(dest, x0, y0, x1, y1, color);
}

void gfx_line_unclipped(SDL_Surface *dest, int x0, int y0, int x1, int y1, Uint32 color)
{
	my_lock(dest);
	int Dx = x1 - x0; 
	int Dy = y1 - y0;
	const int steep = (abs(Dy) >= abs(Dx));
	if (steep) {
	   SWAP(x0, y0);
	   SWAP(x1, y1);
	   // recompute Dx, Dy after swap
	   Dx = x1 - x0;
	   Dy = y1 - y0;
	}
	int xstep = 1;
	if (Dx < 0) {
	   xstep = -1;
	   Dx = -Dx;
	}
	int ystep = 1;
	if (Dy < 0) {
	   ystep = -1;		
	   Dy = -Dy; 
	}
	int TwoDy = 2*Dy; 
	int TwoDyTwoDx = TwoDy - 2*Dx; // 2*Dy - 2*Dx
	int E = TwoDy - Dx; //2*Dy - Dx
	int y = y0;
	int xDraw, yDraw;	
	for (int x = x0; x != x1; x += xstep) {		
	   if (steep) {			
		   xDraw = y;
		   yDraw = x;
	   } else {			
		   xDraw = x;
		   yDraw = y;
	   }
	   // plot
	   plot(dest, xDraw, yDraw, color);
	   // next
	   if (E > 0) {
		   E += TwoDyTwoDx; //E += 2*Dy - 2*Dx;
		   y = y + ystep;
	   } else {
		   E += TwoDy; //E += 2*Dy;
	   }
	}
	my_unlock(dest);
}


static void scale2x(SDL_Surface *src, SDL_Surface *dst)
{
	int looph, loopw;
	
	Uint8* srcpix = (Uint8*)src->pixels;
	Uint8* dstpix = (Uint8*)dst->pixels;

	const int srcpitch = src->pitch;
	const int dstpitch = dst->pitch;
	const int width = src->w;
	const int height = src->h;

	Uint32 *E0, *E1, *E2, *E3;
	Uint32 B, D, E, F, H;
	for(looph = 0; looph < height; ++looph)
	{
		E0 = (Uint32*)(dstpix + looph*2*dstpitch + 0*2*4);
		E1 = (Uint32*)(dstpix + looph*2*dstpitch + (0*2+1)*4);
		E2 = (Uint32*)(dstpix + (looph*2+1)*dstpitch + 0*2*4);
		E3 = (Uint32*)(dstpix + (looph*2+1)*dstpitch + (0*2+1)*4);
	
		for(loopw = 0; loopw < width; ++ loopw)
		{
			B = *(Uint32*)(srcpix + (my_max(0,looph-1)*srcpitch) + (4*loopw));
			D = *(Uint32*)(srcpix + (looph*srcpitch) + (4*my_max(0,loopw-1)));
			E = *(Uint32*)(srcpix + (looph*srcpitch) + (4*loopw));
			F = *(Uint32*)(srcpix + (looph*srcpitch) + (4*my_min(width-1,loopw+1)));
			H = *(Uint32*)(srcpix + (my_min(height-1,looph+1)*srcpitch) + (4*loopw));
			
			*E0 = D == B && B != F && D != H ? D : E;
			*E1 = B == F && B != D && F != H ? F : E;
			*E2 = D == H && D != B && H != F ? D : E;
			*E3 = H == F && D != H && B != F ? F : E;

			E0 += 2;
			E1 += 2;
			E2 += 2;
			E3 += 2;
		}
	}

}


static void scale3x(SDL_Surface *src, SDL_Surface *dst)
{
	int looph, loopw;
	
	Uint8* srcpix = (Uint8*)src->pixels;
	Uint8* dstpix = (Uint8*)dst->pixels;

	const int srcpitch = src->pitch;
	const int dstpitch = dst->pitch;
	const int width = src->w;
	const int height = src->h;

	Uint32 *E0, *E1, *E2, *E3, *E4, *E5, *E6, *E7, *E8, A, B, C, D, E, F, G, H, I;
	for(looph = 0; looph < height; ++looph)
	{
		E0 = (Uint32*)(dstpix + looph*3*dstpitch + 0*3*4);
		E1 = (Uint32*)(dstpix + looph*3*dstpitch + (0*3+1)*4);
		E2 = (Uint32*)(dstpix + looph*3*dstpitch + (0*3+2)*4);
		E3 = (Uint32*)(dstpix + (looph*3+1)*dstpitch + 0*3*4);
		E4 = (Uint32*)(dstpix + (looph*3+1)*dstpitch + (0*3+1)*4);
		E5 = (Uint32*)(dstpix + (looph*3+1)*dstpitch + (0*3+2)*4);
		E6 = (Uint32*)(dstpix + (looph*3+2)*dstpitch + 0*3*4);
		E7 = (Uint32*)(dstpix + (looph*3+2)*dstpitch + (0*3+1)*4);
		E8 = (Uint32*)(dstpix + (looph*3+2)*dstpitch + (0*3+2)*4);
	
		for(loopw = 0; loopw < width; ++ loopw)
		{
				A = *(Uint32*)(srcpix + (my_max(0,looph-1)*srcpitch) + (4*my_max(0,loopw-1)));
				B = *(Uint32*)(srcpix + (my_max(0,looph-1)*srcpitch) + (4*loopw));
				C = *(Uint32*)(srcpix + (my_max(0,looph-1)*srcpitch) + (4*my_min(width-1,loopw+1)));
				D = *(Uint32*)(srcpix + (looph*srcpitch) + (4*my_max(0,loopw-1)));
				E = *(Uint32*)(srcpix + (looph*srcpitch) + (4*loopw));
				F = *(Uint32*)(srcpix + (looph*srcpitch) + (4*my_min(width-1,loopw+1)));
				G = *(Uint32*)(srcpix + (my_min(height-1,looph+1)*srcpitch) + (4*my_max(0,loopw-1)));
				H = *(Uint32*)(srcpix + (my_min(height-1,looph+1)*srcpitch) + (4*loopw));
				I = *(Uint32*)(srcpix + (my_min(height-1,looph+1)*srcpitch) + (4*my_min(width-1,loopw+1)));
			
			if (B != H && D != F) {
				*E0 = D == B ? D : E;
				*E1 = (D == B && E != C) || (B == F && E != A) ? B : E;
				*E2 = B == F ? F : E;
				*E3 = (D == B && E != G) || (D == H && E != A) ? D : E;
				*E4 = E;
				*E5 = (B == F && E != I) || (H == F && E != C) ? F : E;
				*E6 = D == H ? D : E;
				*E7 = (D == H && E != I) || (H == F && E != G) ? H : E;
				*E8 = H == F ? F : E;
			} else {
				*E0 = E;
				*E1 = E;
				*E2 = E;
				*E3 = E;
				*E4 = E;
				*E5 = E;
				*E6 = E;
				*E7 = E;
				*E8 = E;
			}


			E0 += 3;
			E1 += 3;
			E2 += 3;
			E3 += 3;
			E4 += 3;
			E5 += 3;
			E6 += 3;
			E7 += 3;
			E8 += 3;
		}
	}
}


void gfx_blit_2x_resample(SDL_Surface *dest, SDL_Surface *src)
{
	scale2x(src,dest);
}

void gfx_blit_3x_resample(SDL_Surface *dest, SDL_Surface *src)
{
	scale3x(src,dest);
}


void gfx_circle(SDL_Surface *dest, const int xc, const int yc, const int r, const Uint32 color)
{
	const int h = my_min(yc+r+1, dest->h);
	for (int y=my_max(0,yc-r); y<h; ++y)
	{
		const int w = (int)(sqrt(SQR(r) - SQR(y - yc)));
		SDL_Rect r = {xc - w, y, w*2, 1};
		SDL_FillRect(dest, &r, color);
	}
}

void gfx_circle_inverted(SDL_Surface *dest, const int xc, const int yc, const int r, const Uint32 color)
{
	const int h = my_min(yc+r+1, dest->h);
	for (int y=my_max(0,yc-r); y<h; ++y)
	{
		const int w = (int)(sqrt(SQR(r) - SQR(y - yc)));
		
		{
			SDL_Rect rect = {xc - r, y, r - w, 1};
			SDL_FillRect(dest, &rect, color);
		}
		
		{
			SDL_Rect rect = {xc + w, y, r - w, 1};
			SDL_FillRect(dest, &rect, color);
		}
	}
}


static void gfx_domain_set_framerate(GfxDomain *d)
{
#ifdef WIN32
	QueryPerformanceFrequency((LARGE_INTEGER*)&d->clock_resolution);
	QueryPerformanceCounter((LARGE_INTEGER*)&d->last_ticks);
	d->frame_time = d->clock_resolution / d->fps;
#else
	d->clock_resolution = 1000;
	d->last_ticks = SDL_GetTicks();
	d->frame_time = 1000 / d->fps;
#endif	
}


void gfx_domain_update(GfxDomain *domain)
{
	debug("Setting screen mode (scale = %d%s)", domain->scale, domain->fullscreen ? ", fullscreen" : "");
	
	domain->screen = SDL_SetVideoMode(domain->screen_w * domain->scale, domain->screen_h * domain->scale, 32, (domain->fullscreen ? SDL_FULLSCREEN : 0) | SDL_HWSURFACE | SDL_DOUBLEBUF | domain->flags);
	
	if (!domain->screen)
	{
		fatal("%s",  SDL_GetError());
		exit(1);
	}
		
	if (domain->scale > 1) 
	{
		domain->buf = SDL_CreateRGBSurface(SDL_SWSURFACE, domain->screen_w, domain->screen_h, 32, 0, 0, 0, 0);
		
		if (!domain->buf)
		{
			fatal("%s",  SDL_GetError());
			exit(1);
		}
		
		if (domain->scale == 4 && domain->scale_type == GFX_SCALE_SMOOTH)
		{
			domain->buf2 = SDL_CreateRGBSurface(SDL_SWSURFACE, domain->screen_w*2, domain->screen_h*2, 32, 0, 0, 0, 0);
			
			if (!domain->buf2)
			{
				fatal("%s",  SDL_GetError());
				exit(1);
			}
		}
		else
		{
			if (domain->buf2) SDL_FreeSurface(domain->buf2);
			domain->buf2 = NULL;
		}
	}
	else
	{
		if (domain->buf) SDL_FreeSurface(domain->buf);
		domain->buf = NULL;
		if (domain->buf2) SDL_FreeSurface(domain->buf2);
		domain->buf2 = NULL;
	}
	
	gfx_domain_set_framerate(domain);
}


void gfx_domain_flip(GfxDomain *domain)
{
	if (domain->scale > 1)
	{
		if (domain->scale_type == GFX_SCALE_FAST)
		{
			my_lock(domain->screen);
			my_lock(domain->buf);
			switch (domain->scale)
			{
				default: break;
				case 2: 
					gfx_blit_2x(domain->screen, domain->buf); 
				break;
				case 3: 
					gfx_blit_3x(domain->screen, domain->buf); 
				break;
				case 4: 
					gfx_blit_4x(domain->screen, domain->buf); 
				break;
			}
			my_unlock(domain->screen);
			my_unlock(domain->buf);
		}
		else
		{
			my_lock(domain->screen);
			my_lock(domain->buf);
			switch (domain->scale)
			{
				default: break;
				case 2: 
					gfx_blit_2x_resample(domain->screen, domain->buf); 
				break;
				case 3: 
					gfx_blit_3x_resample(domain->screen, domain->buf); 
				break;
				case 4: 
					my_lock(domain->buf2);
					gfx_blit_2x_resample(domain->buf2, domain->buf); 
					gfx_blit_2x_resample(domain->screen, domain->buf2);
					my_unlock(domain->buf2);
				break;
			}
			my_unlock(domain->screen);
			my_unlock(domain->buf);
		}
	}
	
	SDL_Flip(domain->screen);
}


SDL_Surface * gfx_domain_get_surface(GfxDomain *domain)
{
	if (domain->scale > 1)
		return domain->buf;
	else
		return domain->screen;
}


void gfx_domain_free(GfxDomain *domain)
{
	if (domain->buf) SDL_FreeSurface(domain->buf);
	if (domain->buf2) SDL_FreeSurface(domain->buf2);
	
	free (domain);
	
}


GfxDomain * gfx_create_domain()
{
	GfxDomain *d = malloc(sizeof(GfxDomain));
	d->screen = d->buf = d->buf2 = NULL;
	d->screen_w = 320;
	d->screen_h = 320;
	d->scale = 1;
	d->scale_type = GFX_SCALE_FAST;
	d->fullscreen = 0;
	d->fps = 50;
	d->flags = 0;
	
	gfx_domain_set_framerate(d);
	
	return d;
}

int gfx_domain_is_next_frame(GfxDomain *domain)
{
#ifdef WIN32
	Uint64 ticks;
	QueryPerformanceCounter((LARGE_INTEGER*)&ticks);
#else
	Uint32 ticks = SDL_GetTicks();
#endif	
	
	FramerateTimer delta = ticks - domain->last_ticks;

	if (delta < domain->frame_time)
		return 0;
		
	FramerateTimer frames = delta * domain->fps / domain->clock_resolution;
		
	domain->last_ticks += frames * domain->frame_time;
	
	return frames;
}
