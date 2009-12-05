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


#include "levelbase.h"

#define ptr_read(dest, ptr) fread(&dest, sizeof(dest), 1, data);

int lev_load(Background *bg, int *n_layers, FILE* data, int (*interpret_event)(void *, const LevEvent *), void* pdata)
{
	int current_layer = -1;
	BgCell * cell = NULL;
	
	while (1)
	{
		Uint8 x = fgetc(data);
		
		if (x == LOP_END) break;
		
		ungetc(x, data);
	
		LevOpCode opcode;
		ptr_read(opcode, data);
		
		FIX_ENDIAN(opcode.repeat);
		
		for (int di = 0 ; di < opcode.repeat ; ++di)
		{
			switch (opcode.opcode)
			{
				default:
				fatal("Unknown opcode %d\n", opcode.opcode);
				return 0;
				break;
				
				case LOP_EVENT:
				{
					LevEvent event = {0};
					ptr_read(event, data);
					
					FIX_ENDIAN(event.x);
					FIX_ENDIAN(event.y);
					FIX_ENDIAN(event.w);
					FIX_ENDIAN(event.h);
					
					for (int i = 0 ; i < EV_PARAMS ; ++i)
					{
						FIX_ENDIAN(event.param[i]);
					}
					
					if (!interpret_event(pdata, &event)) 
					{
						warning("Unknown event %d\n", event.param[0]);
					}
				}
				break;
				
				case LOP_LAYER:
				{
					++current_layer;
					
					if (current_layer >= *n_layers)
					{
						fatal("Too many layers");
						return 0;
					}
					
					LevLayer header;
	
					ptr_read(header, data);
					
					FIX_ENDIAN(header.flags);
					FIX_ENDIAN(header.w);
					FIX_ENDIAN(header.h);
					FIX_ENDIAN(header.prx_mlt_x);
					FIX_ENDIAN(header.prx_mlt_y);
					FIX_ENDIAN(header.off_x);
					FIX_ENDIAN(header.off_y);
					
					bg[current_layer].flags = header.flags;
					bg[current_layer].w = header.w;
					bg[current_layer].h = header.h;
					bg[current_layer].prx_mlt_x = header.prx_mlt_x;
					bg[current_layer].prx_mlt_y = header.prx_mlt_y;
					bg[current_layer].off_x = header.off_x;
					bg[current_layer].off_y = header.off_y;
					bg[current_layer].data = calloc(sizeof(*bg[current_layer].data), header.w * header.h);
					cell = bg[current_layer].data;
				}
				break;
				
				case LOP_TILE:
				{
					LevTile tile = {0};
					
					ptr_read(tile, data);
					
					FIX_ENDIAN(tile.type);
					
					cell->tile = tile.type;
					
					++cell;
				}
				break;
				
				case LOP_REP_TILE:
				{
					LevRepTile tile = {0};
					
					ptr_read(tile, data);
					
					FIX_ENDIAN(tile.type);
					FIX_ENDIAN(tile.repeat);
					
					for (int i = 0 ; i < tile.repeat ; ++i)
					{
						cell->tile = tile.type;
						++cell;
					}
				}
				break;
			}
		}
	}
	
	*n_layers = current_layer+1;
	
	return 1;
}

void lev_unload(Background *bg, int n_layers)
{
	for (int i = 0 ; i < n_layers ; ++i)
	{
		free(bg[i].data);
		bg[i].data = NULL;
	}
}
