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
