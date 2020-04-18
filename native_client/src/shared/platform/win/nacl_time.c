/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime time abstraction layer.
 * This is the host-OS-dependent implementation.
 */

/* Make sure that winmm.lib is added to the linker's input. */
#pragma comment(lib, "winmm.lib")

#include <windows.h>
#include <intrin.h>
#include <mmsystem.h>
#include <sys/timeb.h>
#include <time.h>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/platform/nacl_time.h"
#include "native_client/src/shared/platform/win/nacl_time_types.h"

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/shared/platform/win/xlate_system_error.h"

#define kMillisecondsPerSecond 1000
#define kMicrosecondsPerMillisecond 1000
#define kMicrosecondsPerSecond 1000000
#define kCalibrateTimeoutMs 1000
#define kCoarseMks 10

/*
 * Here's some usefull links related to timing on Windows:
 * http://svn.wildfiregames.com/public/ps/trunk/docs/timing_pitfalls.pdf
 * Rob Arnold (mozilla) blog:
 * http://robarnold.org/measuring-performance/
 * https://bugzilla.mozilla.org/show_bug.cgi?id=563082
 */

/*
 * To get a 1ms resolution time-of-day clock, we have to jump thrugh
 * some hoops.  This approach has the following defect: ntp
 * adjustments will be nullified, until the difference is larger than
 * a recalibration threshold (below), at which point time may jump
 * (until we introduce a mechanism to slowly phase in time changes).
 * A system suspend/resume will generate a clock recalibration with
 * high probability.
 *
 * The algorithm is as follows:
 *
 * At start up, we sample the system time (coarse timer) via
 * GetSystemTimeAsFileTime until it changes.  At the state transition,
 * we record the value of the time-after-boot counter via timeGetTime.
 *
 * At subsequent gettimeofday syscall, we query the system time via
 * GetSystemTimeAsFileTime.  This is converted to an
 * expected/corresponding return value from timeGetTime.  The
 * difference from the actual return value is the time that elapsed
 * since the state transition of the system time counter, and we add
 * that difference (assuming it is smaller than the max-drift
 * threshold) to the system time and return that (minus the Unix
 * epoch, etc), as the result of the gettimeofday syscall.  If the
 * difference is larger than the max-drift threshold, we invoke the
 * calibration function.
 */

struct NaClTimeState g_NaCl_time_state;

static uint32_t const kMaxMillsecondDriftBeforeRecalibration = 60;
static uint32_t const kMaxCalibrationDiff = 4;

/*
 * Translate Windows system time to milliseconds.  Windows System time
 * is in units of 100nS.  Windows time is in 100e-9s == 1e-7s, and we
 * want units of 1e-3s, so ms/wt = 1e-4.
 */
static uint64_t NaClFileTimeToMs(FILETIME *ftp) {
  ULARGE_INTEGER t;
  t.u.HighPart = ftp->dwHighDateTime;
  t.u.LowPart = ftp->dwLowDateTime;
  return t.QuadPart / NACL_100_NANOS_PER_MILLI;
}

#define kCpuInfoPageSize 4  /* Not in bytes, but in sizeof(uint32_t). */

static char* CPU_GetBrandString(void) {
  const int kBaseInfoPage = 0x80000000;
  const int kCpuBrandStringPage1 = 0x80000002;
  const int kCpuBrandStringPage2 = 0x80000003;
  const int kCpuBrandStringPage3 = 0x80000004;

  static int cpu_info[kCpuInfoPageSize * 4] = {0};
  __cpuid(cpu_info, kBaseInfoPage);
  if (cpu_info[0] < kCpuBrandStringPage3)
    return "";

  __cpuid(&cpu_info[0], kCpuBrandStringPage1);
  __cpuid(&cpu_info[kCpuInfoPageSize], kCpuBrandStringPage2);
  __cpuid(&cpu_info[kCpuInfoPageSize * 2], kCpuBrandStringPage3);
  return (char*)cpu_info;
}

static int CPU_GetFamily(void) {
  const int kFeaturesPage = 1;
  int cpu_info[kCpuInfoPageSize];
  __cpuid(cpu_info, kFeaturesPage);
  return (cpu_info[0] >> 8) & 0xf;
}

/*
 * Returns 1 if success, 0 in case of QueryPerformanceCounter is not supported.
 *
 * Calibration is done as described above.
 *
 * To ensure that the calibration is accurate, we interleave sampling
 * the microsecond resolution counter via QueryPerformanceCounter with the 1
 * millisecond resolution system clock via GetSystemTimeAsFileTime.
 * When we see the edge transition where the system clock ticks, we
 * compare the before and after microsecond counter values.  If it is
 * within a calibration threshold (kMaxCalibrationDiff), then we
 * record the instantaneous system time (1 msresolution) and the
 * 64-bit counter value (~0.5 mks reslution).  Since we have a before and
 * after counter value, we interpolate it to get the "average" counter
 * value to associate with the system time.
 */
static int NaClCalibrateWindowsClockQpc(struct NaClTimeState *ntsp) {
  FILETIME  ft_start;
  FILETIME  ft_prev;
  FILETIME  ft_now;
  LARGE_INTEGER  counter_before;
  LARGE_INTEGER  counter_after;
  int64_t  counter_diff;
  int64_t  counter_diff_ms;
  uint64_t end_of_calibrate;
  int sys_time_changed;
  int calibration_success = 0;

  NaClLog(5, "Entered NaClCalibrateWindowsClockQpc\n");

  GetSystemTimeAsFileTime(&ft_start);
  ft_prev = ft_start;
  end_of_calibrate = NaClFileTimeToMs(&ft_start) + kCalibrateTimeoutMs;

  do {
    if (!QueryPerformanceCounter(&counter_before))
      return 0;
    GetSystemTimeAsFileTime(&ft_now);
    if (!QueryPerformanceCounter(&counter_after))
      return 0;

    counter_diff = counter_after.QuadPart - counter_before.QuadPart;
    counter_diff_ms =
        (counter_diff * kMillisecondsPerSecond) / ntsp->qpc_frequency;
    sys_time_changed = (ft_now.dwHighDateTime != ft_prev.dwHighDateTime) ||
        (ft_now.dwLowDateTime != ft_prev.dwLowDateTime);

    if ((counter_diff >= 0) &&
        (counter_diff_ms <= kMaxCalibrationDiff) &&
        sys_time_changed) {
      calibration_success = 1;
      break;
    }
    if (sys_time_changed)
      ft_prev = ft_now;
  } while (NaClFileTimeToMs(&ft_now) < end_of_calibrate);

  ntsp->system_time_start_ms = NaClFileTimeToMs(&ft_now);
  ntsp->qpc_start = counter_before.QuadPart + (counter_diff / 2);
  ntsp->last_qpc = counter_after.QuadPart;

  NaClLog(5,
          "Leaving NaClCalibrateWindowsClockQpc : %d\n",
          calibration_success);
  return calibration_success;
}

/*
 * Calibration is done as described above.
 *
 * To ensure that the calibration is accurate, we interleave sampling
 * the millisecond resolution counter via timeGetTime with the 10-55
 * millisecond resolution system clock via GetSystemTimeAsFileTime.
 * When we see the edge transition where the system clock ticks, we
 * compare the before and after millisecond counter values.  If it is
 * within a calibration threshold (kMaxCalibrationDiff), then we
 * record the instantaneous system time (10-55 msresolution) and the
 * 32-bit counter value (1ms reslution).  Since we have a before and
 * after counter value, we interpolate it to get the "average" counter
 * value to associate with the system time.
 */
static void NaClCalibrateWindowsClockMu(struct NaClTimeState *ntsp) {
  FILETIME  ft_start;
  FILETIME  ft_now;
  DWORD     ms_counter_before;
  DWORD     ms_counter_after;
  uint32_t  ms_counter_diff;

  NaClLog(5, "Entered NaClCalibrateWindowsClockMu\n");
  GetSystemTimeAsFileTime(&ft_start);
  ms_counter_before = timeGetTime();
  for (;;) {
    GetSystemTimeAsFileTime(&ft_now);
    ms_counter_after = timeGetTime();
    ms_counter_diff = ms_counter_after - (uint32_t) ms_counter_before;
    NaClLog(5, "ms_counter_diff %u\n", ms_counter_diff);
    if (ms_counter_diff <= kMaxCalibrationDiff &&
        (ft_now.dwHighDateTime != ft_start.dwHighDateTime ||
         ft_now.dwLowDateTime != ft_start.dwLowDateTime)) {
      break;
    }
    ms_counter_before = ms_counter_after;
  }
  ntsp->system_time_start_ms = NaClFileTimeToMs(&ft_now);
  /*
   * Average the counter values.  Note unsigned computation of
   * ms_counter_diff, so that was mod 2**32 arithmetic, and the
   * addition of half the difference is numerically correct, whereas
   * (ms_counter_before + ms_counter_after)/2 is wrong due to
   * overflow.
   */
  ntsp->ms_counter_start = (DWORD) (ms_counter_before + (ms_counter_diff / 2));

  NaClLog(5, "Leaving NaClCalibrateWindowsClockMu\n");
}

void NaClAllowLowResolutionTimeOfDay(void) {
  g_NaCl_time_state.allow_low_resolution = 1;
}

void NaClTimeInternalInit(struct NaClTimeState *ntsp) {
  TIMECAPS    tc;
  SYSTEMTIME  st;
  FILETIME    ft;
  LARGE_INTEGER qpc_freq;

  /*
   * Maximize timer/Sleep resolution.
   */
  timeGetDevCaps(&tc, sizeof tc);

  if (ntsp->allow_low_resolution) {
    /* Set resolution to max so we don't over-promise. */
    ntsp->wPeriodMin = tc.wPeriodMax;
  } else {
    ntsp->wPeriodMin = tc.wPeriodMin;
    timeBeginPeriod(ntsp->wPeriodMin);
    NaClLog(4, "NaClTimeInternalInit: timeBeginPeriod(%u)\n", ntsp->wPeriodMin);
  }
  ntsp->time_resolution_ns = ntsp->wPeriodMin * NACL_NANOS_PER_MILLI;

  /*
   * Compute Unix epoch start; calibrate high resolution clock.
   */
  st.wYear = 1970;
  st.wMonth = 1;
  st.wDay = 1;
  st.wHour = 0;
  st.wMinute = 0;
  st.wSecond = 0;
  st.wMilliseconds = 0;
  SystemTimeToFileTime(&st, &ft);
  ntsp->epoch_start_ms = NaClFileTimeToMs(&ft);
  NaClLog(4, "Unix epoch start is  %"NACL_PRIu64"ms in Windows epoch time\n",
          ntsp->epoch_start_ms);

  NaClMutexCtor(&ntsp->mu);

  /*
   * We don't actually grab the lock, since the module initializer
   * should be called before going threaded.
   */
  ntsp->can_use_qpc = 0;
  if (!ntsp->allow_low_resolution) {
    ntsp->can_use_qpc = QueryPerformanceFrequency(&qpc_freq);
    /*
     * On Athlon X2 CPUs (e.g. model 15) QueryPerformanceCounter is
     * unreliable.  Fallback to low-res clock.
     */
    if (strstr(CPU_GetBrandString(), "AuthenticAMD") && (CPU_GetFamily() == 15))
        ntsp->can_use_qpc = 0;

    NaClLog(4,
            "CPU_GetBrandString->[%s] ntsp->can_use_qpc=%d\n",
            CPU_GetBrandString(),
            ntsp->can_use_qpc);

    if (ntsp->can_use_qpc) {
      ntsp->qpc_frequency = qpc_freq.QuadPart;
      NaClLog(4, "qpc_frequency = %"NACL_PRId64" (counts/s)\n",
              ntsp->qpc_frequency);
      if (!NaClCalibrateWindowsClockQpc(ntsp))
        ntsp->can_use_qpc = 0;
    }
    if (!ntsp->can_use_qpc)
      NaClCalibrateWindowsClockMu(ntsp);
  }
}

uint64_t NaClTimerResolutionNsInternal(struct NaClTimeState *ntsp) {
  return ntsp->time_resolution_ns;
}

void NaClTimeInternalFini(struct NaClTimeState *ntsp) {
  NaClMutexDtor(&ntsp->mu);
  if (!ntsp->allow_low_resolution)
    timeEndPeriod(ntsp->wPeriodMin);
}

void NaClTimeInit(void) {
  NaClTimeInternalInit(&g_NaCl_time_state);
}

void NaClTimeFini(void) {
  NaClTimeInternalFini(&g_NaCl_time_state);
}

uint64_t NaClTimerResolutionNanoseconds(void) {
  return NaClTimerResolutionNsInternal(&g_NaCl_time_state);
}

int NaClGetTimeOfDayInternQpc(struct nacl_abi_timeval *tv,
                              struct NaClTimeState    *ntsp,
                              int allow_calibration) {
  FILETIME  ft_now;
  int64_t sys_now_mks;
  LARGE_INTEGER qpc;
  int64_t qpc_diff;
  int64_t qpc_diff_mks;
  int64_t qpc_now_mks;
  int64_t drift_mks;
  int64_t drift_ms;

  NaClLog(5, "Entered NaClGetTimeOfDayInternQpc\n");

  NaClXMutexLock(&ntsp->mu);

  GetSystemTimeAsFileTime(&ft_now);
  QueryPerformanceCounter(&qpc);
  sys_now_mks = NaClFileTimeToMs(&ft_now) * kMicrosecondsPerMillisecond;
  NaClLog(5, " sys_now_mks = %"NACL_PRId64" (us)\n", sys_now_mks);
  qpc_diff = qpc.QuadPart - ntsp->qpc_start;
  NaClLog(5, " qpc_diff = %"NACL_PRId64" (counts)\n", qpc_diff);
  /*
   * Coarse qpc_now_mks to 10 microseconds resolution,
   * to match the other platforms and not make a side-channel
   * attack any easier than it needs to be.
   */
  qpc_diff_mks = ((qpc_diff * (kMicrosecondsPerSecond / kCoarseMks)) /
      ntsp->qpc_frequency) * kCoarseMks;
  NaClLog(5, " qpc_diff_mks = %"NACL_PRId64" (us)\n", qpc_diff_mks);

  qpc_now_mks = (ntsp->system_time_start_ms * kMicrosecondsPerMillisecond) +
      qpc_diff_mks;
  NaClLog(5, " system_time_start_ms %"NACL_PRIu64"\n",
          ntsp->system_time_start_ms);
  NaClLog(5, " qpc_now_mks = %"NACL_PRId64" (us)\n", qpc_now_mks);

  if ((qpc_diff < 0) || (qpc.QuadPart < ntsp->last_qpc)) {
    NaClLog(5, " need recalibration\n");
    if (allow_calibration) {
      NaClCalibrateWindowsClockQpc(ntsp);
      NaClXMutexUnlock(&ntsp->mu);
      return NaClGetTimeOfDayInternQpc(tv, ntsp, 0);
    } else {
      NaClLog(5, " ... but using coarse, system time instead.\n");
      /* use GetSystemTimeAsFileTime(), not QPC */
      qpc_now_mks = sys_now_mks;
    }
  }
  ntsp->last_qpc = qpc.QuadPart;
  drift_mks = sys_now_mks - qpc_now_mks;
  if (qpc_now_mks > sys_now_mks)
    drift_mks = qpc_now_mks - sys_now_mks;

  drift_ms = drift_mks / kMicrosecondsPerMillisecond;
  NaClLog(5, " drift_ms = %"NACL_PRId64"\n", drift_ms);

  if (allow_calibration &&
      (drift_ms > kMaxMillsecondDriftBeforeRecalibration)) {
    NaClLog(5, "drift_ms recalibration\n");
    NaClCalibrateWindowsClockQpc(ntsp);
    NaClXMutexUnlock(&ntsp->mu);
    return NaClGetTimeOfDayInternQpc(tv, ntsp, 0);
  }

  NaClXMutexUnlock(&ntsp->mu);

  /* translate to unix time base */
  qpc_now_mks = qpc_now_mks
      - ntsp->epoch_start_ms * kMicrosecondsPerMillisecond;

  tv->nacl_abi_tv_sec =
      (nacl_abi_time_t)(qpc_now_mks / kMicrosecondsPerSecond);
  tv->nacl_abi_tv_usec =
      (nacl_abi_suseconds_t)(qpc_now_mks % kMicrosecondsPerSecond);
  return 0;
}

int NaClGetTimeOfDayIntern(struct nacl_abi_timeval *tv,
                           struct NaClTimeState    *ntsp) {
  FILETIME  ft_now;
  DWORD     ms_counter_now;
  uint64_t  t_ms;
  DWORD     ms_counter_at_ft_now;
  uint32_t  ms_counter_diff;
  uint64_t  unix_time_ms;

  if (ntsp->can_use_qpc)
    return NaClGetTimeOfDayInternQpc(tv, ntsp, 1);

  GetSystemTimeAsFileTime(&ft_now);
  ms_counter_now = timeGetTime();
  t_ms = NaClFileTimeToMs(&ft_now);

  NaClXMutexLock(&ntsp->mu);

  if (!ntsp->allow_low_resolution) {
    NaClLog(5, "ms_counter_now       %"NACL_PRIu32"\n",
            (uint32_t) ms_counter_now);
    NaClLog(5, "t_ms                 %"NACL_PRId64"\n", t_ms);
    NaClLog(5, "system_time_start_ms %"NACL_PRIu64"\n",
            ntsp->system_time_start_ms);

    ms_counter_at_ft_now = (DWORD)
        (ntsp->ms_counter_start +
         (uint32_t) (t_ms - ntsp->system_time_start_ms));

    NaClLog(5, "ms_counter_at_ft_now %"NACL_PRIu32"\n",
            (uint32_t) ms_counter_at_ft_now);

    ms_counter_diff = ms_counter_now - (uint32_t) ms_counter_at_ft_now;

    NaClLog(5, "ms_counter_diff      %"NACL_PRIu32"\n", ms_counter_diff);

    if (ms_counter_diff <= kMaxMillsecondDriftBeforeRecalibration) {
      t_ms = t_ms + ms_counter_diff;
    } else {
      NaClCalibrateWindowsClockMu(ntsp);
      t_ms = ntsp->system_time_start_ms;
    }

    NaClLog(5, "adjusted t_ms =      %"NACL_PRIu64"\n", t_ms);
  }

  unix_time_ms = t_ms - ntsp->epoch_start_ms;

  NaClXMutexUnlock(&ntsp->mu);

  NaClLog(5, "unix_time_ms  =      %"NACL_PRId64"\n", unix_time_ms);
  /*
   * Unix time is measured relative to a different epoch, Jan 1, 1970.
   * See the module initialization for epoch_start_ms.
   */

  tv->nacl_abi_tv_sec = (nacl_abi_time_t) (unix_time_ms / 1000);
  tv->nacl_abi_tv_usec = (nacl_abi_suseconds_t) ((unix_time_ms % 1000) * 1000);
  return 0;
}

int NaClGetTimeOfDay(struct nacl_abi_timeval *tv) {
  return NaClGetTimeOfDayIntern(tv, &g_NaCl_time_state);
}

int NaClNanosleep(struct nacl_abi_timespec const *req,
                  struct nacl_abi_timespec       *rem) {
  DWORD                     sleep_ms;
  uint64_t                  resolution;
  DWORD                     resolution_gap = 0;

  UNREFERENCED_PARAMETER(rem);

  /* round up from ns resolution to ms resolution */
  /* TODO(bsy): report an error or loop if req->tv_sec does not fit in DWORD */
  sleep_ms = ((DWORD) req->tv_sec * NACL_MILLIS_PER_UNIT +
              NACL_UNIT_CONVERT_ROUND(req->tv_nsec, NACL_NANOS_PER_MILLI));

  /* round up to minimum timer resolution */
  resolution = NaClTimerResolutionNanoseconds();
  NaClLog(4, "Resolution %"NACL_PRId64"\n", resolution);
  if (0 != resolution) {
    resolution = NACL_UNIT_CONVERT_ROUND(resolution, NACL_NANOS_PER_MILLI);
    resolution_gap = (DWORD) (sleep_ms % resolution);
    if (0 != resolution_gap) {
      resolution_gap = (DWORD) (resolution - resolution_gap);
    }
  }
  NaClLog(4, "Resolution gap %d\n", resolution_gap);
  sleep_ms += resolution_gap;

  NaClLog(4, "Sleep(%d)\n", sleep_ms);
  Sleep(sleep_ms);

  return 0;
}
