#pragma once

#include "SDL.h"

#define SEPARATOR_HEIGHT 6
#define ELEMENT_MARGIN 4

typedef struct
{
	const SDL_Rect position;
	
	/*
	This is a combined drawing and mouse event handler.
	*/
	
	void (*handler)(SDL_Surface *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);
	void *param;
	/*
	When clicked the focus set to the following or if the param equals -1 it will leave as it is
	*/
	int focus;
} View;

void draw_view(SDL_Surface *dest, const View* views, const SDL_Event *event);

void adjust_rect(SDL_Rect *rect, int margin);
void copy_rect(SDL_Rect *dest, const SDL_Rect *src);
void update_rect(const SDL_Rect *parent, SDL_Rect *rect);
void clip_rect(SDL_Rect *rect, const SDL_Rect *limits);
