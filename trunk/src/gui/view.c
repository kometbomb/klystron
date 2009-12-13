#include "view.h"

void update_rect(const SDL_Rect *parent, SDL_Rect *rect)
{
	rect->x += rect->w + ELEMENT_MARGIN;
	
	if (rect->x + rect->w - ELEMENT_MARGIN >= parent->x + parent->w)
	{
		rect->x = parent->x;
		rect->y += rect->h;
	}
}


void adjust_rect(SDL_Rect *rect, int margin)
{
	rect->x += margin;
	rect->y += margin;
	rect->w -= margin * 2;
	rect->h -= margin * 2;
}


void copy_rect(SDL_Rect *dest, const SDL_Rect *src)
{
	memcpy(dest, src, sizeof(*dest));
}


void clip_rect(SDL_Rect *rect, const SDL_Rect *limits)
{
	if (rect->x < limits->x) { rect->w -= limits->x - rect->x; rect->x = limits->x; }
	if (rect->y < limits->y) { rect->h -= limits->y - rect->y; rect->y = limits->y; }
	if (rect->w + rect->x > limits->w + limits->x) { rect->w = limits->w + limits->x - rect->x; }
	if (rect->h + rect->y > limits->h + limits->y) { rect->h = limits->h + limits->y - rect->y; }
}
