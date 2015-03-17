**libksnd** is a (Win32) library using native system API (no SDL needed) and optimized for size. The library code is around 34 kilobytes uncompressed so it is suitable for small intros and whatnot. Both static and dynamic linking is supported.

Download here: [libksnd.zip](https://dl.dropboxusercontent.com/u/1190319/libksnd.zip)

## Notes ##

  * See [ksnd.h](http://code.google.com/p/klystron/source/browse/trunk/src/lib/ksnd.h) for the API.
  * Link with libksnd.a for DLL use (needs ksnd.dll)
  * Link with libksndstatic.a for static use (no external dlls needed) - note: you need to link with winmm since it uses Windows waveout stuff
  * You can also include the song as data and use `KSND_LoadSongFromMemory()` to load the song
  * For VS users: use the .def file to create a .lib from the DLL in case you need it.

Song loading and playing should be straightforward. See example below:

```
#include "ksnd.h"
#include <windows.h>

// Plays Test.kt from the beginning for 5 seconds at full volume, 5 seconds at half volume, pauses for 1 second
// and after unpausing frees allocated stuff and exits.

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
	KPlayer *player;
	KSong * song;
	
	player = KSND_CreatePlayer(44100);
	
	song = KSND_LoadSong(player, "Test.kt");
	
	KSND_PlaySong(player, song, 0);
	
	Sleep(5000);

	KSND_SetVolume(player, 64); // Half volume, range is 0..128

	Sleep(5000);
	
	KSND_Pause(player, 1);
	
	Sleep(1000);
	
	KSND_Pause(player, 0);
	
	Sleep(10000);
		
	KSND_FreePlayer(player);
	KSND_FreeSong(song);
		
	return 0;
}
```

Static linking:

```
gcc player.c -o player -lksndstatic -lwinmm
```

Dynamic linking:

```
gcc player.c -o player -lksnd 
```

## Using your own mixer routines ##

If you are using your own mixer to play audio data or just want to dump the audio in a file, you can use KSND\_FillBuffer() to make the player fill your own buffer with audio without playing it. Use that buffer any way you like.

You need to create a KPlayer with KSND\_CreatePlayerUnregistered() to bypass the creation of a player thread.

## Compiling the library ##

Here's how to compile it yourself. Note: you can also use "size" in place of "release" to make the library a bit smaller.

  1. Get the klystron repo.
  1. `make CFG=release EXTFLAGS="-DUSESDLMUTEXES -DSTEREOOUTPUT -DUSENATIVEAPIS -DLOWRESWAVETABLE"`
  1. Never mind the warnings.
  1. libksnd.a, ksnd.dll etc. will in the bin.release directory
  1. src/lib has the rest of the files (ksnd.def, ksnd.h)

## Making the lib smaller ##

You can compile libksnd using various compilation options that disable player features you do not need. This helps to shave a few kilobytes off the library size.

E.g. `make CFG=release EXTFLAGS="-DCYD_DISABLE_FX -DUSESDLMUTEXES -DSTEREOOUTPUT -DUSENATIVEAPIS -DLOWRESWAVETABLE"`

| `CYD_DISABLE_BUZZ` | Disable buzzer |
|:-------------------|:---------------|
| `CYD_DISABLE_ENVELOPE` | Disable envelope |
| `CYD_DISABLE_FILTER` | Disable filter |
| `CYD_DISABLE_FX` | Disable FX bus |
| `CYD_DISABLE_INACCURACY` | Disable pitch inaccuracy |
| `CYD_DISABLE_LFSR` | Disable LFSR (Pokey) |
| `CYD_DISABLE_MULTIPLEX` | Disable channel multiplexing |
| `CYD_DISABLE_PWM` | Disable PWM automation |
| `CYD_DISABLE_WAVETABLE` | Disable wavetable |
| `CYD_DISABLE_VIBRATO` | Disable vibrato automation |
| `GENERATE_VIBRATO_TABLES` | Generate lookup tables instead of using Protracker tables |