#ifndef MACROS_H
#define MACROS_H

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


#include <stdio.h>

#define SQR(x) ((x)*(x))
#define my_min(a,b) (((a)<(b))?(a):(b))
#define my_max(a,b) (((a)>(b))?(a):(b))
#define my_lock(s) do { if (SDL_MUSTLOCK(s)) SDL_LockSurface(s); } while(0)
#define my_unlock(s) do { if (SDL_MUSTLOCK(s)) SDL_UnlockSurface(s); } while(0)
#define VER(file_version, first_version, last_version, block)\
	if ((((Uint16)file_version) >= ((Uint16)first_version)) && (((Uint16)file_version) <= ((Uint16)last_version)))\
	{\
		block;\
	} 
	
#define VER_READ(file_version, first_version, last_version, var, size) VER(file_version, first_version, last_version, fread(var, !size ? sizeof(*var) : size, 1, f));
#define _VER_READ(x, size) VER_READ(version, 0, MUS_VERSION, x, size)
#define _VER_WRITE(x, size) fwrite(x, !size ? sizeof(*x) : size, 1, f)

#ifdef DEBUG
# define debug(...) do { printf("[DEBUG] "); printf(__VA_ARGS__); printf("\n"); } while(0)
#else
# define debug(...) do {} while(0)
#endif

#define warning(...) do { fputs("[WARNING] ", stderr); fprintf(stderr, __VA_ARGS__); fputs("\n", stderr); } while(0)
#define fatal(...) do { fputs("[FATAL] ", stderr); fprintf(stderr, __VA_ARGS__); fputs("\n", stderr); } while(0)

#endif
