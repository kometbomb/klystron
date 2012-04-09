#ifndef KSND_DLL_H
#define KSND_DLL_H

/*

Wrapper library for easy klystron song loading and playing.

*/

#ifdef __cplusplus
extern "C" {
#endif

typedef struct KSong_t KSong;
typedef struct KPlayer_t KPlayer;

#ifdef WIN32 
#ifdef DLLEXPORT
#define KLYSAPI _cdecl __declspec(dllexport)
#else
#define KLYSAPI 
#endif
#else
#define KLYSAPI 
#endif

// Song

KLYSAPI extern KSong* KSND_LoadSong(KPlayer* player, const char *path);
KLYSAPI extern KSong* KSND_LoadSongFromMemory(KPlayer* player, void *data, int data_size);
KLYSAPI extern void KSND_FreeSong(KSong *song);
KLYSAPI extern int KSND_GetSongLength(const KSong *song);

// Player

KLYSAPI extern KPlayer* KSND_CreatePlayer(int sample_rate);
KLYSAPI extern void KSND_FreePlayer(KPlayer *player);
KLYSAPI extern void KSND_PlaySong(KPlayer *player, KSong *song, int start_position);
KLYSAPI extern void KSND_Stop(KPlayer* player);
KLYSAPI extern void KSND_Pause(KPlayer *player, int state);
KLYSAPI extern int KSND_GetPlayPosition(KPlayer* player);
KLYSAPI extern void KSND_SetVolume(KPlayer *player, int volume);

#ifdef __cplusplus
}
#endif

#endif