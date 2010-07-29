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
	FILE *handle;
	BundleFile *file;
} RWOpsBundle;


int bnd_open(Bundle *bundle, const char * filename)
{
	FILE *f = fopen(filename, "rb");
	
	if (f)
	{
		if (!bnd_open_file(bundle, f, filename))
		{
			fclose(f);
			return 0;
		}
	
		fclose(f);
		return 1;
	}
	else
	{
		warning("Can't open bundle '%s'", filename);
		return 0;
	}
}


int bnd_open_file(Bundle *bundle, FILE *f, const char * filename)
{
	memset(bundle, 0, sizeof(*bundle));
	
	bundle->path = strdup(filename);
	
	Uint32 origin = ftell(f);
	Uint32 _origin = origin;
	
	char sig[5] = { 0 };
	fread(sig, strlen(BND_SIG), sizeof(char), f);
	
	if (strncmp(sig, BND_SIG, strlen(BND_SIG)) != 0)
	{
		warning("Bundle sig does not match '"BND_SIG"'");
		return 0;
	}
	
	fread(&bundle->flags, 1, sizeof(bundle->flags), f);
	
	FIX_ENDIAN(bundle->flags);
	
	if (bundle->flags != 0)
		warning("Unsupported bundle mode");
	
	fread(&bundle->n_files, 1, sizeof(bundle->n_files), f);
	
	FIX_ENDIAN(bundle->n_files);
	
	bundle->file = calloc(sizeof(bundle->file[0]), bundle->n_files);
	
	for (int i = 0 ; i < bundle->n_files ; ++i)
	{
		char name[BND_MAX_NAME_SIZE];
		memset(name, 0, sizeof(name));
		
		char c;
		int l = 0;
		
		do
		{
			c = fgetc(f);
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
		fread(&bundle->file[i].size, 1, sizeof(bundle->file[i].size), f);
		FIX_ENDIAN(bundle->file[i].size);
		bundle->file[i].offset = origin;
		origin += bundle->file[i].size;
	}
	
	Uint32 header_size = ftell(f) - _origin;
	
	for (int i = 0 ; i < bundle->n_files ; ++i)
	{
		bundle->file[i].offset += header_size;
	}
	
	debug("Opened bundle '%s', %d files", filename, bundle->n_files);
	
	return 1;
}


void bnd_free(Bundle *bundle)
{
	for (int i = 0 ; i < bundle->n_files ; ++i)
	{
		free(bundle->file[i].name);
	}
	
	if (bundle->handle) 
	{
		fclose(bundle->handle);
	}
	
	free(bundle->path);
	free(bundle->file);
	memset(bundle, 0, sizeof(*bundle));
}


FILE *bnd_locate(Bundle *bundle, const char *filename, int static_handle)
{
	for (int i = 0 ; i < bundle->n_files ; ++i)
	{
		if (strcmp(bundle->file[i].name, filename) == 0)
		{
			FILE *h = fopen(bundle->path, "rb");
			
			if (static_handle)
			{
				if (bundle->handle)
					fclose(bundle->handle);
					
				bundle->handle = h;
			}
			
			if (h)
			{
				debug("Opening '%s' @ %u", filename, bundle->file[i].offset);
				fseek(h, bundle->file[i].offset, SEEK_SET);
			}
			else
			{
				warning("Could not reopen bundle '%s'", bundle->path);
			}
			
			return h;
		}
	}
	
	warning("File '%s' not found in bundle", filename);
	
	return NULL;
}


static int bnd_seek(struct SDL_RWops *context, int offset, int whence)
{
	RWOpsBundle *b = context->hidden.unknown.data1;
	
	switch (whence)
	{
		case SEEK_SET:
		fseek(b->handle, b->file->offset + offset, SEEK_SET);
		break;
	
		case SEEK_CUR:
		fseek(b->handle, offset, SEEK_CUR);
		break;
		
		case SEEK_END:
		fseek(b->handle, b->file->offset + b->file->size - offset, SEEK_SET);
		break;
	}
	
	if (ftell(b->handle) < b->file->offset)
		fseek(b->handle, b->file->offset, SEEK_SET);
	if (ftell(b->handle) > b->file->offset + b->file->size)
		fseek(b->handle, b->file->offset + b->file->size, SEEK_SET);
		
	return ftell(b->handle) - b->file->offset;
}


static int bnd_read(struct SDL_RWops *context, void *ptr, int size, int num)
{
	RWOpsBundle *b = context->hidden.unknown.data1;
	
	if (size * num > b->file->size - (ftell(b->handle) - b->file->offset))
		num = (b->file->size - (ftell(b->handle) - b->file->offset)) / size;
	
	fread(ptr, size, num, b->handle);
	
	return num;
}


static int bnd_close(struct SDL_RWops *context)
{
	debug("bnd_close");
	RWOpsBundle *b = context->hidden.unknown.data1;
	fclose(b->handle);
	free(b);
	SDL_FreeRW(context);
	return 0;
}


SDL_RWops *SDL_RWFromBundle(Bundle *bundle, const char *filename)
{
	SDL_RWops *rwops;
	
	FILE *f = bnd_locate(bundle, filename, 0);
	
	if (!f)
	{
		warning("SDL_RWFromBundle failed to open file");
		return NULL;
	}
	
	RWOpsBundle * b = calloc(1, sizeof(RWOpsBundle));
	b->handle = f;
	
	for (int i = 0 ; i < bundle->n_files ; ++i)
	{
		if (strcmp(bundle->file[i].name, filename) == 0)
		{
			b->file = &bundle->file[i];
			break;
		}
	}
	
	rwops = SDL_AllocRW();
	
	if (rwops != NULL) 
	{
		rwops->seek = bnd_seek;
		rwops->read = bnd_read;
		rwops->write = NULL;
		rwops->close = bnd_close;
		rwops->hidden.unknown.data1 = b;
	}
	else
	{
		fclose(f);
		free(b);
	}
	
	return(rwops);
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
