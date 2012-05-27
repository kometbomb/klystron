#include "ksnd.h"
#include "snd/music.h"
#include "snd/cyd.h"
#include "macros.h"

struct KSong_t 
{
	MusSong song;
	CydWavetableEntry wavetable_entries[CYD_WAVE_MAX_ENTRIES];
};


struct KPlayer_t 
{
	CydEngine cyd;
	MusEngine mus;
};


KLYSAPI KSong* KSND_LoadSong(KPlayer* player, const char *path)
{
	KSong *song = calloc(sizeof(*song), 1);
	
	int i = 0;
	for (i = 0 ; i < CYD_WAVE_MAX_ENTRIES ; ++i)
	{
		cyd_wave_entry_init(&song->wavetable_entries[i], NULL, 0, 0, 0, 0, 0);
	}
	
	if (mus_load_song(path, &song->song, song->wavetable_entries))
	{
		return song;
	}
	else
	{
		free(song);
		return NULL;
	}
}

#ifdef USENATIVEAPIS
static int RWread(struct RWops *context, void *ptr, int size, int maxnum)
{
	const int len = my_min(size * maxnum, context->mem.length - context->mem.ptr);
	memcpy(ptr, context->mem.base + context->mem.ptr, len);
	
	context->mem.ptr += len;
	
	return len;
}


static int RWclose(struct RWops *context)
{
	free(context);
	return 1;
}
#endif


KLYSAPI KSong* KSND_LoadSongFromMemory(KPlayer* player, void *data, int data_size)
{
#ifdef USENATIVEAPIS
	RWops *ops = calloc(sizeof(*ops), 1);
	ops->read = RWread;
	ops->close = RWclose;
	ops->mem.base = data;
	ops->mem.length = data_size;
#else
	RWops *ops = SDL_RWFromMem(data, data_size);
#endif
	
	KSong *song = calloc(sizeof(*song), 1);
	
	int i = 0;
	for (i = 0 ; i < CYD_WAVE_MAX_ENTRIES ; ++i)
	{
		cyd_wave_entry_init(&song->wavetable_entries[i], NULL, 0, 0, 0, 0, 0);
	}
	
	if (mus_load_song_RW(ops, &song->song, song->wavetable_entries))
	{
		return song;
	}
	else
	{
		free(song);
#ifdef USENATIVEAPIS
		RWclose(ops);
#else
		SDL_RWclose(ops);
#endif
		return NULL;
	}
}


KLYSAPI void KSND_FreeSong(KSong *song)
{
	int i = 0;
	for (i = 0 ; i < CYD_WAVE_MAX_ENTRIES ; ++i)
	{
		cyd_wave_entry_init(&song->wavetable_entries[i], NULL, 0, 0, 0, 0, 0);
	}

	mus_free_song(&song->song);
	free(song);
}


KLYSAPI int KSND_GetSongLength(const KSong *song)
{
	return song->song.song_length;
}


KLYSAPI KPlayer* KSND_CreatePlayer(int sample_rate)
{
	KPlayer *player = malloc(sizeof(*player));
	
	cyd_init(&player->cyd, sample_rate, 1);
	mus_init_engine(&player->mus, &player->cyd);
	
	// Each song has its own wavetable array so let's free this
	free(player->cyd.wavetable_entries); 
	player->cyd.wavetable_entries = NULL;
	
	cyd_register(&player->cyd);
	
	return player;
}


KLYSAPI void KSND_FreePlayer(KPlayer *player)
{
	KSND_Stop(player);
	cyd_unregister(&player->cyd);
	cyd_deinit(&player->cyd);
	free(player);
}


KLYSAPI void KSND_PlaySong(KPlayer *player, KSong *song, int start_position)
{
	player->cyd.wavetable_entries = song->wavetable_entries;
	cyd_set_callback(&player->cyd, mus_advance_tick, &player->mus, song->song.song_rate);
	mus_set_fx(&player->mus, &song->song);
	
	if (song->song.num_channels > player->cyd.n_channels)
		cyd_reserve_channels(&player->cyd, song->song.num_channels);
	
	mus_set_song(&player->mus, &song->song, start_position);
}


KLYSAPI void KSND_Stop(KPlayer *player)
{
	mus_set_song(&player->mus, NULL, 0);
	cyd_set_callback(&player->cyd, NULL, NULL, 1);
	player->cyd.wavetable_entries = NULL;
}


KLYSAPI void KSND_Pause(KPlayer *player, int state)
{
	cyd_pause(&player->cyd, state);
}


KLYSAPI int KSND_GetPlayPosition(KPlayer *player)
{
	int song_position = 0;
	
	mus_poll_status(&player->mus, &song_position, NULL, NULL, NULL, NULL, NULL);
	
	return song_position;
}


KLYSAPI void KSND_SetVolume(KPlayer *player, int volume)
{
	cyd_lock(&player->cyd, 1);
	player->mus.volume = volume;
	cyd_lock(&player->cyd, 0);
}


KLYSAPI void KSND_GetVUMeters(KPlayer *player, int *dest, int n_channels)
{
	int temp[MUS_MAX_CHANNELS];
	mus_poll_status(&player->mus, NULL, NULL, NULL, NULL, dest, NULL);
	memcpy(dest, temp, sizeof(dest[0]) * n_channels);
}
