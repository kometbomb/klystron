#include "bundle.h"
#include <stdlib.h>
#include "macros.h"
#include <unistd.h>

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


typedef struct
{
	SDL_RWops *handle;
	BundleFile *file;
	Uint32 pos;
} RWOpsBundle;


int bnd_open(Bundle *bundle, const char * filename)
{
	SDL_RWops *rw = SDL_RWFromFile(filename, "rb");
	
	if (rw)
	{
		if (!bnd_open_RW(bundle, rw))
		{
			SDL_FreeRW(rw);
			return 0;
		}
	
		bundle->close_handle = true;
	
		//SDL_FreeRW(rw);
		return 1;
	}
	else
	{
		warning("Can't open bundle '%s'", filename);
		return 0;
	}
}


int bnd_open_RW(Bundle *bundle, SDL_RWops *ctx)
{
	memset(bundle, 0, sizeof(*bundle));
	
	char sig[5] = { 0 };
	SDL_RWread(ctx, sig, strlen(BND_SIG), sizeof(char));
	
	if (strncmp(sig, BND_SIG, strlen(BND_SIG)) != 0)
	{
		warning("Bundle sig does not match '"BND_SIG"'");
		return 0;
	}
	
	SDL_RWread(ctx, &bundle->flags, 1, sizeof(bundle->flags));
	
	FIX_ENDIAN(bundle->flags);
	
	if (bundle->flags != 0)
		warning("Unsupported bundle mode");
	
	SDL_RWread(ctx, &bundle->n_files, 1, sizeof(bundle->n_files));
	
	FIX_ENDIAN(bundle->n_files);
	
	bundle->file = calloc(sizeof(bundle->file[0]), bundle->n_files);
	
	Uint32 origin = 0;
	
	for (int i = 0 ; i < bundle->n_files ; ++i)
	{
		char name[BND_MAX_NAME_SIZE];
		memset(name, 0, sizeof(name));
		
		char c;
		int l = 0;
		
		do
		{
			SDL_RWread(ctx, &c, 1, 1);
			if (l < BND_MAX_NAME_SIZE-1)
			{
				name[l] = c;
				++l;
			}
			else
				warning("Bundle name too long");
		}
		while (c);
		
		bundle->file[i].name = strdup(name);
		SDL_RWread(ctx, &bundle->file[i].size, 1, sizeof(bundle->file[i].size));
		FIX_ENDIAN(bundle->file[i].size);
		bundle->file[i].offset = origin;
		origin += bundle->file[i].size;
	}
	
	Uint32 header_size = SDL_RWtell(ctx);
	
	for (int i = 0 ; i < bundle->n_files ; ++i)
	{
		bundle->file[i].offset += header_size;
		debug("%s @ %u", bundle->file[i].name, bundle->file[i].offset);
	}
	
	bundle->handle = ctx;
	
	debug("Opened bundle (%d files)", bundle->n_files);
	
	return 1;
}


void bnd_free(Bundle *bundle)
{
	for (int i = 0 ; i < bundle->n_files ; ++i)
	{
		free(bundle->file[i].name);
	}
	
	if (bundle->close_handle) 
	{
		SDL_RWclose(bundle->handle);
		SDL_FreeRW(bundle->handle);
	}
	
	free(bundle->file);
	memset(bundle, 0, sizeof(*bundle));
}


#if SDL_VERSION_ATLEAST(1,3,0)
static long bnd_seek(struct SDL_RWops *context, long offset, int whence)
#else
static int bnd_seek(struct SDL_RWops *context, int offset, int whence)
#endif
{
	RWOpsBundle *b = context->hidden.unknown.data1;
	
	switch (whence)
	{
		case SEEK_SET:
		SDL_RWseek(b->handle, b->file->offset + offset, SEEK_SET);
		break;
	
		case SEEK_CUR:
		SDL_RWseek(b->handle, offset, SEEK_CUR);
		break;
		
		case SEEK_END:
		SDL_RWseek(b->handle, b->file->offset + b->file->size - offset, SEEK_SET);
		break;
	}
	
	if (SDL_RWtell(b->handle) < b->file->offset)
		SDL_RWseek(b->handle, b->file->offset, SEEK_SET);
	if (SDL_RWtell(b->handle) > b->file->offset + b->file->size)
		SDL_RWseek(b->handle, b->file->offset + b->file->size, SEEK_SET);
		
	return b->pos = (SDL_RWtell(b->handle) - b->file->offset);
}


#if SDL_VERSION_ATLEAST(1,3,0)
static size_t bnd_read(struct SDL_RWops *context, void *ptr, size_t size, size_t num)
#else
static int bnd_read(struct SDL_RWops *context, void *ptr, int size, int num)
#endif
{
	RWOpsBundle *b = context->hidden.unknown.data1;
	
	SDL_RWseek(b->handle, b->file->offset + b->pos, SEEK_SET);
	
	if (size * num > b->file->size - (SDL_RWtell(b->handle) - b->file->offset))
		num = (b->file->size - (SDL_RWtell(b->handle) - b->file->offset)) / size;
	
	b->pos += size * num;
	
	return SDL_RWread(b->handle, ptr, size, num);
}


static int bnd_close(struct SDL_RWops *context)
{
	debug("bnd_close");
	RWOpsBundle *b = context->hidden.unknown.data1;
	free(b);
	return 0;
}


SDL_RWops *SDL_RWFromBundle(Bundle *bundle, const char *filename)
{
	SDL_RWops *rwops;
	RWOpsBundle * b = calloc(1, sizeof(RWOpsBundle));
	b->handle = bundle->handle;
	
	for (int i = 0 ; i < bundle->n_files ; ++i)
	{
		if (strcmp(bundle->file[i].name, filename) == 0)
		{
			b->file = &bundle->file[i];
			break;
		}
	}
	
	if (!b->file)
	{
		warning("SDL_RWFromBundle file %s not found", filename);
		free(b);
		return NULL;
	}
		
	rwops = SDL_AllocRW();
	
	if (rwops != NULL) 
	{
		rwops->seek = bnd_seek;
		rwops->read = bnd_read;
		rwops->write = NULL;
		rwops->close = bnd_close;
		rwops->hidden.unknown.data1 = b;
		
		rwops->seek(rwops, 0, SEEK_SET);
	}
	else
	{
		free(b);
	}
	
	return rwops;
}


int bnd_exists(const Bundle *bundle, const char *filename)
{
	for (int i = 0 ; i < bundle->n_files ; ++i)
	{
		if (strcmp(bundle->file[i].name, filename) == 0)
		{
			return 1;
		}
	}
	
	return 0;
}


