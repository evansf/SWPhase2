// call to Intel fine grained timer.  define _LOCALTIMER in the debug build environment
#ifdef _LOCALTIMER

#include "windows.h"
#undef min
#undef max
#define STARTTIMER(__name) static double __name;LARGE_INTEGER __name##_LT0; LARGE_INTEGER __name##_L_Timing_ClocksPerSec; QueryPerformanceFrequency(&__name##_L_Timing_ClocksPerSec); __name = 0.0; QueryPerformanceCounter(&__name##_LT0);
#define STOPTIMER(__name) LARGE_INTEGER __name##_LT1; QueryPerformanceCounter(&__name##_LT1); __name = (double)(__name##_LT1.LowPart-__name##_LT0.LowPart)/__name##_L_Timing_ClocksPerSec.LowPart;
#define READTIMER(__name) LARGE_INTEGER __name##_LT1; QueryPerformanceCounter(&__name##_LT1); __name = (double)(__name##_LT1.LowPart-__name##_LT0.LowPart)/__name##_L_Timing_ClocksPerSec.LowPart;

#else	// LOCALTIMER Not defined
#define STARTTIMER(__name) static double __name = 0.0;
#define STOPTIMER(__name) __name = 0.0;
#define READTIMER(__name)
#endif	// _LOCALTIMER


