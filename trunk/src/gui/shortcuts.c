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


#include "shortcuts.h"

void do_shortcuts(SDL_KeyboardEvent *e, const KeyShortcut *shortcuts)
{
	for (int i = 0 ; shortcuts[i].action ; ++i)
	{
		if (e->keysym.sym == shortcuts[i].key
			&& (!(e->keysym.mod & KMOD_SHIFT) == !(shortcuts[i].mod & KMOD_SHIFT))
			&& (!(e->keysym.mod & KMOD_CTRL) == !(shortcuts[i].mod & KMOD_CTRL))
			&& (!(e->keysym.mod & KMOD_ALT) == !(shortcuts[i].mod & KMOD_ALT))
		)
		{
			shortcuts[i].action((void*)shortcuts[i].p1, (void*)shortcuts[i].p2, (void*)shortcuts[i].p3);
			e->keysym.sym = 0;
			break;
		}
	}
}
