/*
Copyright (c) 2009-2011 Tero Lindeman (kometbomb)

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

#include <assert.h>
#include "gfx.h"
#include <math.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef USESDL_IMAGE
#include "SDL_image.h"
#endif

#ifdef USEOPENGL
#include <GL/glext.h>
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

int * gfx_build_collision_mask(SDL_Surface *s)
{
	int * mask = malloc(sizeof(int) * s->w * s->h);
	
#if SDL_VERSION_ATLEAST(1,3,0)
	Uint32 key;
	SDL_GetColorKey(s, &key);
#else	
	const Uint32 key = s->format->colorkey;
#endif
	
	my_lock(s);
	for (int y = 0 ; y < s->h ; ++y)
	{
		Uint8 *pa = (Uint8 *)s->pixels + y * s->pitch;
		
		for (int x = 0 ; x < s->w ; ++x)
		{
			int p = 0;
			
			switch (s->format->BytesPerPixel) 
			{
				default:
				case 1: p = ((*((Uint8*)pa))) != key; break;
				case 2: p = ((*((Uint16*)pa))&0xffff) != key; break;
				case 3: p = ((*((Uint32*)pa))&0xffffff) != key; break;
				case 4: p = ((*((Uint32*)pa))&0xffffff) != key; break;
			}
			
			mask[x + y * s->w] = p;
			pa += s->format->BytesPerPixel;
		}
	}
	my_unlock(s);
	
	return mask;
}
	
	
GfxSurface* gfx_load_surface(const char* filename, const int flags)
{
	FILE *f = fopen(filename, "rb");
	if (f)
	{
		SDL_RWops * rw = SDL_RWFromFP(f, 1);
		GfxSurface * s = gfx_load_surface_RW(rw, flags);
		return s;
	}
	else
	{
		warning("Opening image '%s' failed", filename);
		return NULL;
	}
}


GfxSurface* gfx_load_surface_RW(SDL_RWops *rw, const int flags)
{
#ifdef USESDL_IMAGE
	SDL_Surface* loaded = IMG_Load_RW(rw, 1);
#else
	SDL_Surface* loaded = SDL_LoadBMP_RW(rw, 1);
#endif
	
	if (!loaded) 
	{
		warning("Loading surface failed: %s", SDL_GetError());
		return NULL;
	}
	
	GfxSurface *gs = calloc(1, sizeof(GfxSurface));
	
	if (flags & GFX_KEYED)
	{
		SDL_Surface* optimal = NULL;
		Uint32 c = SDL_MapRGB(loaded->format, 255, 0, 255);
		Uint8 r, g, b;
		SDL_GetRGB(c, loaded->format, &r, &g, &b);
		
		if (r == 255 && g == 0 && b == 255)
			SDL_SetColorKey(loaded, SDL_RLEACCEL | SDL_SRCCOLORKEY, c);
			
		optimal = SDL_DisplayFormat(loaded);
		
		if (!optimal)
		{
			warning("Conversion failed %dx%d", loaded->w, loaded->h);
			gs->surface = loaded;
			goto out;
		}
		
		if (optimal && (flags & GFX_TRANS_HALF))
		{
			SDL_SetAlpha(optimal, SDL_RLEACCEL | SDL_SRCALPHA, 128);  
		}
		
		SDL_FreeSurface(loaded);
		
		gs->surface = optimal;
	}
	else if (flags & GFX_ALPHA)
	{
		SDL_Surface* optimal = NULL;
		SDL_SetAlpha(loaded, SDL_RLEACCEL | SDL_SRCALPHA, (flags & GFX_TRANS_HALF)?128:SDL_ALPHA_OPAQUE);  
		optimal = SDL_DisplayFormatAlpha(loaded);
		
		if (!optimal)
		{
			warning("Conversion failed %dx%d", loaded->w, loaded->h);
			gs->surface = loaded;
			goto out;
		}
		
		SDL_FreeSurface(loaded);
		
		gs->surface = optimal;
	}
	else
	{
		SDL_Surface* optimal = SDL_DisplayFormatAlpha(loaded);
		
		if (!optimal)
		{
			warning("Conversion failed %dx%d", loaded->w, loaded->h);
			gs->surface = loaded;
			goto out;
		}
		
		SDL_FreeSurface(loaded);
	
		gs->surface = optimal;
	}
	out:
	
	if (flags & GFX_COL_MASK) gs->mask = gfx_build_collision_mask(gs->surface);
	
	gs->flags = flags;
	
	return gs;
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
#if SDL_VERSION_ATLEAST(1,3,0)
	Uint32 key;
	SDL_GetColorKey(desc->surface->surface, &key);
#else	
	const Uint32 key = desc->surface->surface->format->colorkey;
#endif

	my_lock(desc->surface->surface);
	
	int result = 0;
	
	for (int y = 0 ; y < desc->rect.h ; ++y)
	{
		Uint8 *p = (Uint8 *)desc->surface->surface->pixels + ((int)desc->rect.y + y) * desc->surface->surface->pitch + (int)desc->rect.x * desc->surface->surface->format->BytesPerPixel;
		
		for (int x = 0 ; x < desc->rect.w ; ++x)
		{
			//printf("%08x", *(Uint32*)p);
			if ((*((Uint32*)p)&0xffffff) != key)
			{
				++result;
			}
			
			p+=desc->surface->surface->format->BytesPerPixel;
		}
	}
	
	my_unlock(desc->surface->surface);
	
	return result;
}


TileDescriptor *gfx_build_tiledescriptor(GfxSurface *tiles, const int cellwidth, const int cellheight) 
{
	TileDescriptor *descriptor = calloc(sizeof(*descriptor), (tiles->surface->w/cellwidth)*(tiles->surface->h/cellheight));
	for (int i = 0 ; i < (tiles->surface->w/cellwidth)*(tiles->surface->h/cellheight) ; ++i) 
	{
		descriptor[i].rect.x = i*cellwidth;
		descriptor[i].rect.y = 0; 
		
		while (descriptor[i].rect.x >= tiles->surface->w)
		{
			descriptor[i].rect.y += cellheight;
			descriptor[i].rect.x -= tiles->surface->w;
		}
		
		descriptor[i].rect.w = cellwidth;
		descriptor[i].rect.h = cellheight;
		descriptor[i].surface = tiles;
		
		int pixels = has_pixels(&descriptor[i]);
		
		if (pixels == CELLSIZE*CELLSIZE || !(tiles->flags & GFX_COL_MASK))
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
	// Liang-Barsky
	
	// TODO: remove float math overkill
	
	float p, q;
	float u1 = 0.0, u2 = 1.0;
	float r;

	float dx = x1 - x0;
	float dy = y1 - y0;

	float ptab[] = {-dx, dx, -dy, dy};
	float qtab[] = {x0, dest->w - 1 - x0, y0, dest->h - 1 - y0};

	for (int i = 0; i < 4; ++i)
	{
		p = ptab[i];
		q = qtab[i];

		if (p == 0 && q < 0)
		{
			return;
		}

		r = q / p;

		if (p < 0)
		{
			u1 = my_max(u1, r);
		}

		if (p > 0)
		{
			u2 = my_min(u2, r);
		}

		if (u1 > u2)
		{
			return;
		}
	}
	
	if (u2 < 1)
	{
		x1 = x0 + u2 * dx;
		y1 = y0 + u2 * dy;
	}
	
	if (u1 > 0)
	{
		x0 = x0 + u1 * dx;
		y0 = y0 + u1 * dy;
	}
		
	gfx_line_unclipped(dest, x0, y0, x1, y1, color);
}


void gfx_line_unclipped(SDL_Surface *dest, int x0, int y0, int x1, int y1, Uint32 color)
{
	assert(x0 >= 0 && x0 <= dest->w && y0 >= 0 && y0 <= dest->h);
	
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
	for (int x = x0; ; x += xstep) {		
	   if (steep) {			
		   xDraw = y;
		   yDraw = x;
	   } else {			
		   xDraw = x;
		   yDraw = y;
	   }
	   // plot
	   plot(dest, xDraw, yDraw, color);
	   
	   if (x == x1) break;
	   
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
			
			if (B != H && D != F) 
			{
				*E0 = D == B ? D : E;
				*E1 = B == F ? F : E;
				*E2 = D == H ? D : E;
				*E3 = H == F ? F : E;
			} 
			else 
			{
				*E0 = E;
				*E1 = E;
				*E2 = E;
				*E3 = E;
			}


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
	
#ifdef USEOPENGL
	const int flags = SDL_OPENGL;
#else
	int flags = 0;
#endif
	
	domain->screen = SDL_SetVideoMode(domain->screen_w * domain->scale, domain->screen_h * domain->scale, 32, (domain->fullscreen ? SDL_FULLSCREEN : 0) | SDL_HWSURFACE | SDL_DOUBLEBUF | domain->flags | flags);
	
	if (!domain->screen)
	{
		fatal("SDL_SetVideoMode failed: %s",  SDL_GetError());
		exit(1);
	}

#ifdef USEOPENGL	
	if (domain->opengl_screen) SDL_FreeSurface(domain->opengl_screen);
	
	if (domain->scale > 1 && domain->scale_type == GFX_SCALE_SMOOTH)
		domain->opengl_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, domain->screen_w * domain->scale, domain->screen_h * domain->scale, 32, 0xff0000, 0x00ff00, 0x0000ff, 0);
	else
		domain->opengl_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, domain->screen_w, domain->screen_h, 32, 0xff0000, 0x00ff00, 0x0000ff, 0);

	if (!domain->opengl_screen)
	{
		fatal("OGL texture surface init failed: %s",  SDL_GetError());
		exit(1);
	}
	
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	
	if (domain->texture) glDeleteTextures(1, &domain->texture);
	glGenTextures(1, &domain->texture);
	glBindTexture(GL_TEXTURE_2D, domain->texture);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glEnable(GL_TEXTURE_2D);
	glViewport(0, 0, domain->screen_w * domain->scale, domain->screen_h * domain->scale);

	// Select The Projection Matrix
	glMatrixMode(GL_PROJECTION);

	// Reset The Projection Matrix
	glLoadIdentity();

	// Select The Modelview Matrix
	glMatrixMode(GL_MODELVIEW);

	// Reset The Modelview Matrix
	glLoadIdentity();
#endif

	if (domain->buf) SDL_FreeSurface(domain->buf);
	domain->buf = NULL;
	if (domain->buf2) SDL_FreeSurface(domain->buf2);
	domain->buf2 = NULL;
		
#ifndef USEOPENGL
	if (domain->scale > 1) 
#else
	if (domain->scale > 1 && domain->scale_type == GFX_SCALE_SMOOTH) 
#endif
	{
		SDL_Surface *temp = SDL_CreateRGBSurface(SDL_SWSURFACE, domain->screen_w, domain->screen_h, 32, 0, 0, 0, 0);
		domain->buf = SDL_ConvertSurface(temp, domain->screen->format, SDL_SWSURFACE);
		SDL_FreeSurface(temp);
		
		if (!domain->buf)
		{
			fatal("%s",  SDL_GetError());
			exit(1);
		}
		
		if (domain->scale == 4 && domain->scale_type == GFX_SCALE_SMOOTH)
		{
			SDL_Surface *temp = SDL_CreateRGBSurface(SDL_SWSURFACE, domain->screen_w * 2, domain->screen_h * 2, 32, 0, 0, 0, 0);
			domain->buf2 = SDL_ConvertSurface(temp, domain->screen->format, SDL_SWSURFACE);
			SDL_FreeSurface(temp);
			
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
	
	gfx_domain_set_framerate(domain);
}


void gfx_domain_flip(GfxDomain *domain)
{
#ifdef USEOPENGL	
	SDL_Surface *screen = domain->opengl_screen;
#else
	SDL_Surface *screen = domain->screen;
#endif

	if (domain->scale > 1)
	{
		if (domain->scale_type == GFX_SCALE_FAST)
		{
#ifndef USEOPENGL
			// opengl nearest does the same thing
			my_lock(screen);
			my_lock(domain->buf);
			switch (domain->scale)
			{
				default: break;
				case 2: 
					gfx_blit_2x(screen, domain->buf); 
				break;
				case 3: 
					gfx_blit_3x(screen, domain->buf); 
				break;
				case 4: 
					gfx_blit_4x(screen, domain->buf); 
				break;
			}
			my_unlock(screen);
			my_unlock(domain->buf);
#endif
		}
		else
		{
			my_lock(screen);
			my_lock(domain->buf);
			switch (domain->scale)
			{
				default: break;
				case 2: 
					gfx_blit_2x_resample(screen, domain->buf); 
				break;
				case 3: 
					gfx_blit_3x_resample(screen, domain->buf); 
				break;
				case 4: 
					my_lock(domain->buf2);
					gfx_blit_2x_resample(domain->buf2, domain->buf); 
					gfx_blit_2x_resample(screen, domain->buf2);
					my_unlock(domain->buf2);
				break;
			}
			my_unlock(screen);
			my_unlock(domain->buf);
		}
	}
	
#ifdef USEOPENGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screen->w, screen->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, screen->pixels);
	glBegin(GL_QUADS);
	glTexCoord2d(0.0,0.0); glVertex2d(-1.0,1.0);
	glTexCoord2d(1.0,0.0); glVertex2d(1.0,1.0);
	glTexCoord2d(1.0,1.0); glVertex2d(1.0,-1.0);
	glTexCoord2d(0.0,1.0); glVertex2d(-1.0,-1.0);
	glEnd();
	SDL_GL_SwapBuffers();
#else
	SDL_Flip(domain->screen);
#endif
}


SDL_Surface * gfx_domain_get_surface(GfxDomain *domain)
{
	if (domain->scale > 1)
	{
#ifndef USEOPENGL
		return domain->buf;
#else
		// opengl nearest is used for fast scaling (no resample)
		// smooth scaling is still done in software (TODO: shader)
		if (domain->scale_type == GFX_SCALE_FAST)
			return domain->opengl_screen;
		else
			return domain->buf;
#endif
	}
	else
#ifdef USEOPENGL	
		return domain->opengl_screen;
#else
		return domain->screen;
#endif
}



void gfx_domain_free(GfxDomain *domain)
{
	if (domain->buf) SDL_FreeSurface(domain->buf);
	if (domain->buf2) SDL_FreeSurface(domain->buf2);
	
#ifdef USEOPENGL
	if (domain->opengl_screen) SDL_FreeSurface(domain->opengl_screen);
	if (domain->texture) glDeleteTextures(1, &domain->texture);
#endif
	
	free(domain);
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
	
#ifdef USEOPENGL
	d->texture = 0;
	d->opengl_screen = NULL;
#endif
	
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


void gfx_free_surface(GfxSurface *surface)
{
	if (surface->surface) SDL_FreeSurface(surface->surface);
	if (surface->mask) free(surface->mask);
	free(surface);
}


void gfx_clear(GfxDomain *domain, Uint32 color)
{
	SDL_FillRect(gfx_domain_get_surface(domain), 0, color);
}


void gfx_blit(GfxSurface *_src, SDL_Rect *_src_rect, GfxDomain *domain, SDL_Rect *_dest_rect)
{
	SDL_Surface *dest = gfx_domain_get_surface(domain);
	SDL_Surface *src = _src->surface;
	SDL_Rect dest_temp = { 0, 0, dest->w, dest->h }, src_temp = { 0, 0, src->w, src->h };
	SDL_Rect *src_rect = _src_rect ? _src_rect : &src_temp;
	SDL_Rect *dest_rect = _dest_rect ? _dest_rect : &dest_temp;

	int w = dest_rect->w, h = dest_rect->h;
	
	/*if (dest_rect->w == src_rect->w && dest_rect->h == src_rect->h)
	{
		SDL_BlitSurface(src, src_rect, dest, dest_rect);
		return;
	}*/
	
	if (dest_rect->w == 0 || dest_rect->h == 0)
		return;
	
	assert(src->format->BytesPerPixel == 4 && dest->format->BytesPerPixel == 4);
	
	if (dest_rect->x < 0)
	{
		src_rect->x -= dest_rect->x * (int)src_rect->w / w;
		dest_rect->w = my_max(0, (int)dest_rect->w + dest_rect->x);
		dest_rect->x = 0;
	}
	
	if (dest_rect->y < 0)
	{
		src_rect->y -= dest_rect->y * (int)src_rect->h / h;
		dest_rect->h = my_max(0, (int)dest_rect->h + dest_rect->y);
		dest_rect->y = 0;
	}
	
	if (dest_rect->x + dest_rect->w >= dest->w)
	{
		dest_rect->w = my_max(0, (int)dest->w - dest_rect->x);
	}
	
	if (dest_rect->y + dest_rect->h >= dest->h)
	{
		dest_rect->h = my_max(0, (int)dest->h - dest_rect->y);
	}
	
	if (dest_rect->w == 0 || dest_rect->h == 0)
		return;
	
	int xspd = 256 * src_rect->w / w;
	int yspd = 256 * src_rect->h / h;
	
	my_lock(dest);
	my_lock(src);
	
	for (int dy = dest_rect->y, sy = 256 * (int)src_rect->y ; dy < dest_rect->h + dest_rect->y ; ++dy, sy += yspd)
	{
		Uint32 * dptr = (Uint32*)((Uint8*)dest->pixels + dy * dest->pitch) + dest_rect->x;
		Uint32 * sptr = (Uint32*)((Uint8*)src->pixels + sy/256 * src->pitch);
		
		for (int dx = dest_rect->w, sx = 256 * (int)src_rect->x ; dx > 0 ; --dx, sx += xspd)
		{
			if (sptr[sx / 256] != 0xff00ff) *dptr = sptr[sx / 256];
			++dptr;
		}
	}
	
	my_unlock(dest);
	my_unlock(src);	
}
