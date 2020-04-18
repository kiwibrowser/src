/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/shared/platform/nacl_threads.h"
#include "native_client/src/shared/platform/nacl_time.h"

#include "native_client/src/trusted/service_runtime/nacl_all_modules.h"


/*
 * This test ensures that NaClCondVarTimedWaitAbsolute and
 * NaClCondVarTimedWaitRelative actually pauses the calling thread --
 * and returns a time out -- for the requested time.  The actual
 * elapsed time is measured and should be greater than or equal to the
 * requested timeout time (due to scheduler delays), and less than or
 * equal to a fuzz factor multiple of the requested time.  The latter
 * is not guarantee by any OS, since the scheduler could delay a
 * process/thread arbitrarily, but is a heuristic sanity check since
 * no normal system would delay a thread for too long.
 *
 * If the timeout does not occur or would take a very long time, this
 * test will just block.  Unfortunately, we don't have a different
 * OS-abstracted mechanisms to do a timeout to test the timeout
 * mechanism....  It would be possible to use threads (NaClThread
 * abstraction) and have another thread use OS-specific usleep / Sleep
 * functions to impose an upper bound on the sleep duration, but that
 * would depend on NaClThread interacting properly with usleep / Sleep
 * -- which is not something that we explicitly require in the
 * contract.
 *
 * WARNING: windows only does millisecond granularity, so do not use
 * parameters that would require highe precision.
 */

int     gVerbosity = 0;

static int const kDefaultSleepStartMicroSeconds =  5000;
static int const kDefaultSleepEndMicroSeconds   = 50000;
static int const kDefaultSleepIncrMicroSeconds  =  5000;

/*
 * These fuzz factors / constant is needed only for windows, due to
 * the translation of absolute time to relative time, and due to the
 * fact that a short relative time wait will return immediately.
 *
 * TODO(gregoryd): determine why TimedWaitRel returns immediately, and
 * control for it so that timing code based on condition wait timeouts
 * can be reliable.
 */
static double   gSchedulerMinFuzzFactor = 1.0;
static uint64_t gSchedulerMinFuzzConst = 20000;  /* usec, so 20mS */
static double   gSchedulerMaxFuzzFactor = 64.0;
static uint64_t gSchedulerMaxMin = 20000;  /* usec, so 20mS */

/*
 * a large max factor value may be needed on busy build machines,
 * where the scheduler may do nasty things for us.
 */

static int const kMicroXinX  = 1000 * 1000;
static int const kNanoXinX   = 1000 * 1000 * 1000;
#if NACL_WINDOWS
static int const kMicroXinMilliX = 1000;
#endif
static int const kNanoXinMicroX = 1000;


struct AlarmerState {
  uint64_t          sleep_usec;
  uint64_t          cond_timeout_usec;
  int               abort_on_wake;
  struct NaClMutex  mu;
  struct NaClThread thr;
};

void AlarmerStateCtor(struct AlarmerState *sp,
                      uint64_t            sleep_usec,
                      uint64_t            cond_timeout_usec) {
  NaClXMutexCtor(&sp->mu);
  sp->abort_on_wake = 1;
  sp->sleep_usec = sleep_usec;
  sp->cond_timeout_usec = cond_timeout_usec;
}

void AlarmerStateDtor(struct AlarmerState *sp) {
  NaClMutexDtor(&sp->mu);
}

void AlarmerDisable(struct AlarmerState *sp) {
  NaClXMutexLock(&sp->mu);
  sp->abort_on_wake = 0;
  NaClXMutexUnlock(&sp->mu);
}

static void PrintFailureSuggestions(void) {
    printf("If this test is running on a very busy machine,\n"
           " try using the -F flag to increase the timeout\n"
           " (this is a multiplier of actual requested timeout time)\n"
           " or -C to increase the minium value for maximum elapsed\n"
           " time permitted\n"
           "Current fuzz factor %g\n",
           gSchedulerMaxFuzzFactor);
}

void WINAPI Alarmer(void *state) {
  struct AlarmerState *sp = (struct AlarmerState *) state;
  int                 should_abort;
  uint64_t            cond_timeout_usec;

#if NACL_WINDOWS
  int millisec = (int) ((sp->sleep_usec + kMicroXinMilliX - 1)
                        / kMicroXinMilliX);
#endif
  if (gVerbosity) {
    printf("Alarmer %p: alarm to go off in"
           " %"NACL_PRId64".%06"NACL_PRId64" seconds\n",
           (void *) sp,
           sp->sleep_usec / kMicroXinX, sp->sleep_usec % kMicroXinX);
    fflush(NULL);
  }
#if NACL_WINDOWS
  Sleep(millisec);
#else
  usleep((useconds_t) sp->sleep_usec);
#endif
  if (gVerbosity) {
    printf("Alarmer %p: woke up\n", (void *) sp);
  }
  NaClXMutexLock(&sp->mu);
  should_abort = sp->abort_on_wake;
  cond_timeout_usec = sp->cond_timeout_usec;
  NaClXMutexUnlock(&sp->mu);
  if (should_abort) {
    printf("Alarmer %p: woke up after %"NACL_PRId64".%06"NACL_PRId64" seconds"
           " without condition\n",
           (void *) sp,
           sp->sleep_usec / kMicroXinX,
           sp->sleep_usec % kMicroXinX);
    printf("condvar timeout was %"NACL_PRId64".%06"NACL_PRId64" sec\n",
           cond_timeout_usec / kMicroXinX,
           cond_timeout_usec % kMicroXinX);
    printf("Alarmer %p:  variable timing out.\n", (void *) sp);
    PrintFailureSuggestions();
    printf("Alarmer %p: aborting all threads\n", (void *) sp);
    exit(1);
  }
  if (gVerbosity) {
    printf("Alarmer %p: cond var ok\n", (void *) sp);
  }
  AlarmerStateDtor(sp);
  /* leaks memory associated with thread state, if any. */
  free(state);
}

struct AlarmerState *SpawnAlarmer(uint64_t  timeout_usec,
                                  uint64_t  cond_timeout_usec) {
  struct AlarmerState *sp = (struct AlarmerState *) malloc(sizeof *sp);

  if (NULL == sp) return sp;
  AlarmerStateCtor(sp, timeout_usec, cond_timeout_usec);
  if (!NaClThreadCtor(&sp->thr, Alarmer, (void *) sp, 4096)) {
    AlarmerStateDtor(sp);
    free(sp);
    return 0;
  }
  return sp;
}


struct Timer {
  struct nacl_abi_timeval base;
};


void TimerStart(struct Timer *self) {
  (void) NaClGetTimeOfDay(&self->base);
}

void TimerCheckElapsed(struct Timer             *self,
                       uint64_t                 cond_timeout_usec,
                       uint64_t                 min_elapsed_usec,
                       uint64_t                 max_elapsed_usec) {
  struct nacl_abi_timeval now;
  uint64_t                elapsed_usec;
  int                     failed;

  (void) NaClGetTimeOfDay(&now);
  elapsed_usec = kMicroXinX * (now.nacl_abi_tv_sec - self->base.nacl_abi_tv_sec)
      + now.nacl_abi_tv_usec - self->base.nacl_abi_tv_usec;

  /*
   * NB: De Morgan's thm on ASSERT args so that there will be some output
   * on failure.
   */
  failed = (min_elapsed_usec > elapsed_usec
            || elapsed_usec > max_elapsed_usec);
  if (gVerbosity || failed) {
    printf("condvar timeout was %"NACL_PRId64".%06"NACL_PRId64" sec\n",
           cond_timeout_usec / kMicroXinX,
           cond_timeout_usec % kMicroXinX);
    printf("min elapsed %"NACL_PRId64".%06"NACL_PRId64" sec\n",
           min_elapsed_usec / kMicroXinX,
           min_elapsed_usec % kMicroXinX);
    printf("max elapsed %"NACL_PRId64".%06"NACL_PRId64" sec\n",
           max_elapsed_usec / kMicroXinX,
           max_elapsed_usec % kMicroXinX);
    printf("actual elapsed %"NACL_PRId64".%06"NACL_PRId64" sec\n",
           elapsed_usec / kMicroXinX,
           elapsed_usec % kMicroXinX);
    if (failed) {
      PrintFailureSuggestions();
    }
  }
  ASSERT(min_elapsed_usec <= elapsed_usec);
  ASSERT(elapsed_usec <= max_elapsed_usec);
}


void FunctorDelays(void                     (*fn)(void *),
                   void                     *arg,
                   uint64_t                 cond_timeout_usec,
                   uint64_t                 min_elapsed_usec,
                   uint64_t                 max_elapsed_usec,
                   int                      use_alarmer) {
  struct Timer t;
  struct AlarmerState     *asp;

  asp = NULL;
  if (use_alarmer) {
    asp = SpawnAlarmer(max_elapsed_usec, cond_timeout_usec);
    ASSERT_MSG(NULL != asp, "Could not create alarmer thread!");
  }
  TimerStart(&t);
  (*fn)(arg);
  if (NULL != asp) {
    AlarmerDisable(asp);
  }
  TimerCheckElapsed(&t, cond_timeout_usec, min_elapsed_usec, max_elapsed_usec);
}

struct NaClMutex        gMu;
struct NaClCondVar      gCv;

void TestInit(void) {
  NaClXMutexCtor(&gMu);
  NaClXCondVarCtor(&gCv);
}

void TestFini(void) {
  NaClMutexDtor(&gMu);
  NaClCondVarDtor(&gCv);
}

struct TestFunctorArg {
  uint64_t  sleep_usec;
};

void TestRelWait(void *arg) {
  uint64_t                  sleep_usec;
  struct nacl_abi_timespec  t;

  sleep_usec = ((struct TestFunctorArg *) arg)->sleep_usec;
  t.tv_sec = (nacl_abi_time_t) (sleep_usec / kMicroXinX);
  t.tv_nsec = (long int) (kNanoXinMicroX * (sleep_usec % kMicroXinX));
  if (gVerbosity > 1) {
    printf("TestRelWait: locking\n");
  }
  NaClXMutexLock(&gMu);
  if (gVerbosity > 1) {
    printf("TestRelWait: waiting\n");
  }
  NaClXCondVarTimedWaitRelative(&gCv, &gMu, &t);
  if (gVerbosity > 1) {
    printf("TestRelWait: unlocking\n");
  }
  NaClXMutexUnlock(&gMu);
}

void TestAbsWait(void *arg) {
  uint64_t                  sleep_usec;
  struct nacl_abi_timeval   now;
  struct nacl_abi_timespec  t;

  sleep_usec = ((struct TestFunctorArg *) arg)->sleep_usec;
  (void) NaClGetTimeOfDay(&now);
  t.tv_sec = (nacl_abi_time_t) (now.nacl_abi_tv_sec + sleep_usec / kMicroXinX);
  t.tv_nsec = (long int) (kNanoXinMicroX * (now.nacl_abi_tv_usec
                                            + (sleep_usec % kMicroXinX)));
  while (t.tv_nsec > kNanoXinX) {
    t.tv_nsec -= kNanoXinX;
    ++t.tv_sec;
  }
  if (gVerbosity > 1) {
    printf("TestAbsWait: locking\n");
  }
  NaClXMutexLock(&gMu);
  if (gVerbosity > 1) {
    printf("TestAbsWait: waiting\n");
  }
  NaClXCondVarTimedWaitAbsolute(&gCv, &gMu, &t);
  if (gVerbosity > 1) {
    printf("TestAbsWait: unlocking\n");
  }
  NaClXMutexUnlock(&gMu);
}


int main(int argc,
         char **argv) {
  int                     opt;
  uint64_t                sleep_start_usec = kDefaultSleepStartMicroSeconds;
  uint64_t                sleep_end_usec = kDefaultSleepEndMicroSeconds;
  uint64_t                sleep_incr_usec = kDefaultSleepIncrMicroSeconds;
  uint64_t                sleep_usec;
  uint64_t                min_sleep;
  uint64_t                max_sleep;
  uint64_t                total_usec;
  char                    *nacl_verbosity = getenv("NACLVERBOSITY");
  struct TestFunctorArg   arg;

  int                     alarmer = 1;

  static void             (*const test_tbl[])(void *) = {
    TestAbsWait, TestRelWait,
  };

  size_t                  ix;

  while (EOF != (opt = getopt(argc, argv,"aAc:C:e:f:F:i:s:v"))) {
    switch (opt) {
      case 'a':
        alarmer = 1;
        break;
      case 'A':
        alarmer = 0;
        break;
      case 'c':
        gSchedulerMinFuzzConst = strtoul(optarg, (char **) 0, 0);
        break;
      case 'C':
        gSchedulerMaxMin = strtoul(optarg, (char **) 0, 0);
        break;
      case 'e':
        sleep_end_usec = strtoul(optarg, (char **) 0, 0);
        break;
      case 'f':
        gSchedulerMinFuzzFactor = atof(optarg);
        break;
      case 'F':
        gSchedulerMaxFuzzFactor = atof(optarg);
        break;
      case 'i':
        sleep_incr_usec = strtoul(optarg, (char **) 0, 0);
        break;
      case 's':
        sleep_start_usec = strtoul(optarg, (char **) 0, 0);
        break;
      case 'v':
        ++gVerbosity;
        break;
      default:
        fprintf(stderr,
                "Usage: nacl_sync_cond_test [flags]\n"
                " where flags specify the range of condition variable\n"
                " wait times:\n"
                " -a use an alarm thread to detect significant oversleep\n"
                " -A do not use an alarm thread\n"
                " -s start microseconds\n"
                " -e end microseconds\n"
                " -i increment microseconds\n"
                " -f scheduler fuzz factor (double; for min elapsed time)\n"
                " -F scheduler fuzz factor (double; max elapsed time)\n"
                " -c scheduler fuzz constant (int, uS)\n"
                "    allow actual elapsed time to be this much less\n"
                "    than requested.\n"
                " -C scheduler fuzz constant (int, uS)\n"
                "    max allowed elapsed time is at least this\n"
                );
        return 1;
    }
  }

  NaClAllModulesInit();
  NaClLogSetVerbosity((NULL == nacl_verbosity)
                      ? 0 : strtol(nacl_verbosity, (char **) 0, 0));

  TestInit();
  ASSERT_MSG(sleep_end_usec > sleep_start_usec,
             "nonsensical start/end times");
  ASSERT_MSG(0 < sleep_incr_usec,
             "nonsensical increment time");

  /*
   * sum of [m..n) by k is (m+n)(n-m)/k/2, and we run Rel and Abs, so
   * the expected time is twice the sum
   */
  total_usec = (sleep_end_usec + sleep_start_usec)
      * (sleep_end_usec - sleep_start_usec) / sleep_incr_usec;

  printf("Test should take approximately %"NACL_PRId64
         ".%06"NACL_PRId64" seconds"
         " on an unloaded machine.\n",
         total_usec / kMicroXinX, total_usec % kMicroXinX);
  for (sleep_usec = sleep_start_usec;
       sleep_usec < sleep_end_usec;
       sleep_usec += sleep_incr_usec) {
    if (gVerbosity) {
      printf("testing wait limit of %"NACL_PRId64
             " microseconds.\n", sleep_usec);
    }
    arg.sleep_usec = sleep_usec;
    min_sleep = (int64_t) (gSchedulerMinFuzzFactor * sleep_usec);
    if (min_sleep < gSchedulerMinFuzzConst) {
      min_sleep = 0;
    } else {
      min_sleep -= gSchedulerMinFuzzConst;
    }
    max_sleep = (uint64_t) (gSchedulerMaxFuzzFactor * sleep_usec);
    if (max_sleep < gSchedulerMaxMin) {
      max_sleep = gSchedulerMaxMin;
    }

    for (ix = 0; ix < NACL_ARRAY_SIZE(test_tbl); ++ix) {
      FunctorDelays(test_tbl[ix], &arg, sleep_usec, min_sleep, max_sleep,
                    alarmer);
    }
  }
  TestFini();

  NaClAllModulesFini();
  printf("PASS\n");
  return 0;
}
