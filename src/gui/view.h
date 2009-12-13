#pragma once

#include "SDL.h"

#define SEPARATOR_HEIGHT 6
#define ELEMENT_MARGIN 4

void adjust_rect(SDL_Rect *rect, int margin);
void copy_rect(SDL_Rect *dest, const SDL_Rect *src);
void update_rect(const SDL_Rect *parent, SDL_Rect *rect);
void clip_rect(SDL_Rect *rect, const SDL_Rect *limits);
