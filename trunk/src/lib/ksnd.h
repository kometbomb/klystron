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

KSong* KSND_LoadSong(KPlayer* player, const char *path);
KSong* KSND_LoadSongFromMemory(KPlayer* player, void *data, int data_size);
void KSND_FreeSong(KSong *song);
KPlayer* KSND_CreatePlayer(int sample_rate);
void KSND_FreePlayer(KPlayer *player);
void KSND_PlaySong(KPlayer *player, KSong *song);
void KSND_Stop(KPlayer* player);

#ifdef __cplusplus
}
#endif

#endif
