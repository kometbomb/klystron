/* SDL-equivalent types */

#ifndef CYDTYPES_H
#define CYDTYPES_H

#ifndef USENATIVEAPIS

# include "SDL.h"

typedef SDL_mutex * CydMutex;

#else

# ifdef WIN32

#include <windows.h>

typedef BYTE Uint8;
typedef CHAR Sint8;
typedef WORD Uint16;
typedef SHORT Sint16;
typedef DWORD Uint32;
typedef INT Sint32;
typedef unsigned long long Uint64;
typedef CRITICAL_SECTION CydMutex;

# else

#  error USENATIVEAPIS: Platform not supported

# endif

#endif

#endif
