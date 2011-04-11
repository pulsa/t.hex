#include "precomp.h"
#define ATIMER_EXPORT
#include "atimer.h"

#include <stdio.h>

#ifdef ATIMER_USE_RDTSC
UINT64 ATimer::CPU_freq = 0;
#endif

ATimer *ATimer::globalTimer = NULL;

// Make sure the global timer object gets deleted
class GlobalTimerDestructor
{
public:
   ~GlobalTimerDestructor()
   {
      if(ATimer::globalTimer)
         delete ATimer::globalTimer;
      ATimer::globalTimer = NULL;
   }
};
static GlobalTimerDestructor gtd; // When this gets deleted, so will globalTimer.

ATimer::ATimer(timer_type method /*= HPC*/, bool bAdjustInterval /*= true*/)
{
   Init(method, bAdjustInterval);
}

bool ATimer::Init(timer_type method, bool bAdjustInterval /*= true*/)
{
   interval = 0;
   ticks_per_sec = 0;
   start_time = 0;
   bAdjustInterval = false;
   minInterval = 0;
   storedCount = 0;

   this->method = method;

#ifdef WIN32
   switch(method)
   {
   case HPC:
      if(!QueryPerformanceFrequency((LARGE_INTEGER*)&ticks_per_sec) || ticks_per_sec == 0)
         return false;
      break;
#ifdef ATIMER_USE_WINMM
   case WINMM:
      {
      TIMECAPS tc;
      timeGetDevCaps(&tc, sizeof(tc));
      timeBeginPeriod(tc.wPeriodMin);
      ticks_per_sec = 1000;
      break;
      }
#endif
#ifdef ATIMER_USE_RDTSC
   case RDTSC:
      if(CPU_freq == 0)
      {
         // calibrate using Windows timer
         UINT64 i1, diff;
         ATimer timer;
         timer.Init();
         StartWatch();
         timer.Wait(timer.GetTicksPerSec());
         StopWatch();

         diff = interval;

         timer.GetCount(&i1);
         StartWatch();
         timer.GetCount(&i1);
         timer.GetCount(&i1);
         StopWatch();

         CPU_freq = diff + interval;
      }
      ticks_per_sec = CPU_freq;
      break;
#endif
#ifdef ATIMER_USE_TICKCOUNT
   case TICKCOUNT:
       ticks_per_sec = 1000;
       break;
#endif
   default:
      return false;
   }
#else // WIN32
    ticks_per_sec = 1000000000;  // struct timespec measures in nanoseconds
#endif
   this->bAdjustInterval = bAdjustInterval;
   if(bAdjustInterval)
      minInterval = GetMinInterval();
   StartWatch();  // I use most of my timers as stopwatches, so let's get the count right away.
   return true;
}

void ATimer::GetCount(UINT64 *count)
{
#ifdef WIN32
#if defined(ATIMER_USE_RDTSC) || defined(ATIMER_USE_WINMM) || defined(ATIMER_USE_TICKCOUNT)
   switch(method)
   {
   case HPC:
      QueryPerformanceCounter((LARGE_INTEGER*)count);
      break;

#ifdef ATIMER_USE_RDTSC
   case RDTSC:
      _asm {
         mov ecx, [count]
         rdtsc
         mov [dword ptr ecx], eax
         mov [dword ptr ecx+4], edx
      }
      break;
#endif
#ifdef ATIMER_USE_WINMM
   case WINMM:
      *count = timeGetTime();
      break;
#endif
#ifdef ATIMER_USE_TICKCOUNT
   case TICKCOUNT:
      *count = GetTickCount();
      break;
#endif
   default:
      *count = 0;
   }
#else
   // This is always available.
   QueryPerformanceCounter((LARGE_INTEGER*)count);
#endif
#else // WIN32
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    *count = ts.tv_sec * 1000000000 + ts.tv_nsec;
#endif
}

void ATimer::StopWatch()
{
   UINT64 stop_time;

   GetCount(&stop_time);

   // minInterval is set to zero if we're not using it
   interval = stop_time - start_time - minInterval + storedCount;
}

void ATimer::Wait(UINT64 interval)
{
   UINT64 stop_time, now;

   GetCount(&now);
   stop_time = now + interval - (minInterval >> 1);
   while(now < stop_time)
      GetCount(&now);
}

ULONG ATimer::GetMinInterval()
{
   //UINT64 temp;
   StartWatch();
   //GetCount(&temp);
   StopWatch();
   return (ULONG)interval;
}


void ATimer::Delay(ULONG uSec)
{
#ifdef WIN32
   UINT64 now, stop_time;
   UINT64 delay = uSec * ticks_per_sec / 1000000;
   const UINT64 minSleep = ticks_per_sec >> 4; // 1/16 second, or 62.5 ms

   GetCount(&now);
   stop_time = now + delay - (minInterval >> 1);

   while(now + minSleep < stop_time)
   {
      Sleep(1); // wait a minimum amount of time
      GetCount(&now);
   }

   while(now < stop_time)
      GetCount(&now);
#else  // WIN32
    usleep(uSec);
#endif
}

void ATimer::Pause()
{
   StopWatch();
   storedCount = interval;
}

void ATimer::Continue()
{
   GetCount(&start_time);
}

void ATimer::SDelay(int uSec)
{
   if(globalTimer == NULL)
      globalTimer = new ATimer(HPC, 0);
   globalTimer->Delay(uSec);
}
