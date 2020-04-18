#include "Platform.h"

#include <stdio.h>

void testRDTSC ( void )
{
  int64_t temp = rdtsc();

  printf("%d",(int)temp);
}

#if defined(_MSC_VER)

#include <windows.h>

void SetAffinity ( int cpu )
{
  SetProcessAffinityMask(GetCurrentProcess(),cpu);
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
}

#else

#include <sched.h>

void SetAffinity ( int /*cpu*/ )
{
#if !defined(__CYGWIN__) && !defined(__APPLE__)
  cpu_set_t mask;
    
  CPU_ZERO(&mask);
    
  CPU_SET(2,&mask);
    
  if( sched_setaffinity(0,sizeof(mask),&mask) == -1)
  {
    printf("WARNING: Could not set CPU affinity\n");
  }
#endif
}

#endif
