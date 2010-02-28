#ifndef EDITOR_H
#define EDITOR_H

#include <stdio.h>

#include "gfx/background.h"
#include "gfx/levelbase.h"

#define MAX_LAYERS 16

typedef struct
{
	Uint8 n_layers;
	Background layer[MAX_LAYERS+2];
	Uint16 n_events;
	LevEvent *event;
} Level;

void level_load(Level *level, FILE *f);
void level_save(Level *level, FILE *f);
void layer_load(Background *layer, FILE *f);
void layer_save(Background *layer, FILE *f);

#endif
