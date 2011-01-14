/* Timer class by Adam Bailey

   Use:
      ATimer timer();

   then:
      timer.Init();                    // use QueryPerformanceCounter
   or:
      timer.Init(ATimer::HPC, false);  // ignore measurement overhead
   or:
      timer.Init(ATimer::RDTSC);       // measure with Pentium timestamp counter

   then:
      timer.Delay(uSeconds)
   or:
      timer.StartWatch();
      (code to time goes here)
      timer.StopWatch();
      dSeconds = timer.GetSeconds();
   or:
      timer.SetTimeout(uSeconds);
      timer.StartWatch();
      while(1)
      {
         (code to time out goes here)
         if(timer.IsTimeUp()) break;
      }

*/


#ifndef _ATIMER_H
#define _ATIMER_H

// In order to use experimental features, #define the appropriate macro.
// This macro enables measurement using the Pentium's RDTSC instruction.
// (ReaD TimeStamp Counter)
//#define ATIMER_USE_RDTSC

// This macro enables measurement using the timeGetTime() function.
//#define ATIMER_USE_WINMM


#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// exclude unused stuff from mmsystem.h
#define MMNODRV     
#define MMNOSOUND   
#define MMNOWAVE    
#define MMNOMIDI    
#define MMNOAUX     
#define MMNOMIXER   
//#define MMNOTIMER   
#define MMNOJOY     
#define MMNOMCI     
#define MMNOMMIO    
#define MMNOMMSYSTEM
#include <mmsystem.h>

#ifndef ATIMER_EXPORT
   #ifdef ATIMER_DLL_EXPORTS
      #define ATIMER_EXPORT __declspec(dllexport)
   #else
      #define ATIMER_EXPORT //__declspec(dllimport)
   #endif
#endif

class ATIMER_EXPORT ATimer {
public:
   typedef enum { HPC,
#ifdef ATIMER_USE_WINMM
      WINMM,
#endif
#ifdef ATIMER_USE_RDTSC
      RDTSC,
#endif
#ifdef ATIMER_USE_TICKCOUNT
      TICKCOUNT,
#endif
   } timer_type;
   ATimer(timer_type method = HPC, bool bAdjustInterval = true);
   bool Init(timer_type method = HPC, bool bAdjustInterval = true);
   ULONG GetMinInterval(); // measures delay inherent in the measuring process

   inline void SetInterval(UINT64 interval); // sets the interval to check for timeout, or to wait
   inline void Wait(); // waits for stored interval
   void Wait(UINT64 interval); // waits for this interval

   inline UINT64 GetInterval() { return interval; }
   inline UINT64 GetTicksPerSec() { return ticks_per_sec; }
   void GetCount(UINT64 *count);

   // these are probably the most useful functions
   inline void StartWatch() { GetCount(&start_time); storedCount = 0; }
   void StopWatch();
   double GetSeconds() const;
   inline double elapsed() { StopWatch(); return GetSeconds(); }  // use while still timing
   void Delay(ULONG uSec);
   bool IsTimeUp();
   void SetTimeout(ULONG uSec, bool bStart = true);
   void Pause();
   void Continue();

   static void SDelay(int uSec); // static function

private:
   UINT64 interval;
   UINT64 ticks_per_sec;
   UINT64 start_time;
   UINT64 storedCount; // used by Pause/Continue
   timer_type method;

   bool bAdjustInterval;
   ULONG minInterval; // stored as ULONG rather than UINT64 for efficient calculations

   static ATimer *globalTimer;
   friend class GlobalTimerDestructor;

#ifdef ATIMER_USE_RDTSC
   static UINT64 CPU_freq; // measured once
#endif
};


////// INLINE FUNCTIONS ////////////

inline void ATimer::Wait()
{
   Wait(this->interval);
}

inline bool ATimer::IsTimeUp()
{
   UINT64 now;
   GetCount(&now);
   return (now - start_time >= interval);
}

inline void ATimer::SetTimeout(ULONG uSec, bool bStart /*= true*/)
{
   UINT64 tmpint = uSec * ticks_per_sec / 1000000;
   SetInterval(tmpint);
   if (bStart)
      StartWatch();
}

inline void ATimer::SetInterval(UINT64 interval)
{
   this->interval = interval;
}

inline double ATimer::GetSeconds() const
{
   return (double)(__int64)interval / (double)(__int64)ticks_per_sec;
}



#endif // _ATIMER_H