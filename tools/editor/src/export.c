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

#include "export.h"
#include "gfx/levelbase.h"
#include "gui/toolutil.h"
 
#define export(ptr, f) { fwrite(ptr, 1, sizeof(*ptr) ,f); }
#define export_opcode(opcode, repeat, f) { LevOpCode op = { opcode, repeat }; export(&op, f); }

extern Font font;
extern GfxDomain *domain;
extern SDL_Surface *gfx;

void level_export(Level *level)
{
	FILE *f = open_dialog("wb", "Export level", "exp", domain, gfx, &font, &font);
	for (int i = 0 ;  i < level->n_layers ; ++i)
	{
		if (level->layer[i].w > 0 && level->layer[i].h > 0)
		{
			export_opcode(LOP_LAYER, 1, f);
			LevLayer header;
			header.flags = level->layer[i].flags;
			header.w = level->layer[i].w;
			header.h = level->layer[i].h;
			header.prx_mlt_x = level->layer[i].prx_mlt_x;
			header.prx_mlt_y = level->layer[i].prx_mlt_y;
			header.off_x = level->layer[i].off_x;
			header.off_y = level->layer[i].off_y;
			export(&header, f);
			//export_opcode(LOP_TILE, level->layer[i].w * level->layer[i].h, f);
			
			int * lut = calloc(level->layer[i].h*level->layer[i].w+1, sizeof(int));
			int lutp = 0;
			int prev = level->layer[i].data[0].tile;
			int count = 0;
			
			for (int x = 0 ; x < level->layer[i].h*level->layer[i].w ; ++x)
			{
				if (x != level->layer[i].h*level->layer[i].w-1 && prev == level->layer[i].data[x].tile)
				{
					++count;
				}
				else
				{
					lut[lutp] = count;
					++lutp;
					count = 1;
					prev = level->layer[i].data[x].tile;
				}
			}
			
			prev = lut[0];
			count = 1;
			lutp = 0;
			LevOpCode * lut2 = calloc(level->layer[i].h*level->layer[i].w+1, sizeof(LevOpCode));
			
			for (int x = 0 ; x == 0 || lut[x-1] ; ++x)
			{
				if (lut[x] != 0 && (prev > 1) == (lut[x] > 1))
				{
					++count;
				}
				else
				{
					lut2[lutp].opcode = lut[x-1] > 1 ? LOP_REP_TILE : LOP_TILE;
					lut2[lutp].repeat = count;
					++lutp;
					count = 1;
					prev = lut[x];
				}
			}
			
			lut2[lutp].opcode = LOP_END;
			
			BgCell *dptr = level->layer[i].data;
			int c = level->layer[i].w * level->layer[i].h;
			for (int x = 0 ; lut2[x].opcode != LOP_END ; ++x)
			{
				export_opcode(lut2[x].opcode, lut2[x].repeat, f);
				for (int r = 0 ; r < lut2[x].repeat ; ++r)
				{
					switch (lut2[x].opcode)
					{
						case LOP_TILE:
						{
							LevTile tile = { dptr->tile };
							export(&tile, f);
							++dptr;
							--c;
						}
						break;
						
						case LOP_REP_TILE:
						{
							LevRepTile tile = { 0, dptr->tile };
							
							
							int count = 0;
							while (dptr->tile == tile.type && c > 0) { ++dptr; --c; ++count; };
							tile.repeat = count;
							export(&tile, f);
						}
						break;
					}
				}
			}
			
			free(lut);
			free(lut2);
		}
	}
	if (level->n_events > 0)
	{
		export_opcode(LOP_EVENT, level->n_events, f);
		for (int i = 0 ;  i < level->n_events ; ++i)
		{
			export(&level->event[i], f);
		}
	}
	export_opcode(LOP_END, 0, f);
	fclose(f);
}
