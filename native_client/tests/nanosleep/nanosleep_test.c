/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "native_client/src/trusted/service_runtime/include/machine/_types.h"

/*
 * Newlib's time.h not working right: getting the nanosleep
 * declaration from the newlib time.h file requires defining
 * _POSIX_TIMERS, but we don't implement any of the clock_settime,
 * clock_gettime, clock_getres, timer_create, timer_delete,
 * timer_settime, timer_gettime, and timer_getoverrun functions.  In
 * glibc's time.h, these function declarations are obtained by
 * ensuring _POSIX_C_SOURCE >= 199309L.
 *
 * Sigh.
 */

#define NANOS_PER_MICRO   (1000)
#define MICROS_PER_MILLI  (1000)
#define NANOS_PER_MILLI   (NANOS_PER_MICRO * MICROS_PER_MILLI)
#define MICROS_PER_UNIT   (1000 * 1000)

/*
 * We don't convert to floating point, so a precondition is that
 * tv_usec is within range, i.e., the timeval has been normalized.
 */
void PrintTimeval(FILE *iob, struct timeval const *tv) {
  fprintf(iob, "%"PRId64".%06ld", (int64_t) tv->tv_sec, tv->tv_usec);
}

/* timespec fields should have been ts_nsec etc */
void PrintTimespec(FILE *iob, struct timespec const *ts) {
  fprintf(iob, "%"PRId64".%09ld", (uint64_t) ts->tv_sec, ts->tv_nsec);
}

void NormalizeTimeval(struct timeval *tv) {
  int first = 1;
  while (tv->tv_usec < 0) {
    tv->tv_usec += MICROS_PER_UNIT;
    tv->tv_sec -= 1;
    if (!first) {
      fprintf(stderr, "NormalizedTimeval: usec too small, 2x normalize!\n");
      PrintTimeval(stderr, tv); putc('\n', stderr);
    }
    first = 0;
  }
  first = 1;
  while (tv->tv_usec >= MICROS_PER_UNIT) {
    tv->tv_usec -= MICROS_PER_UNIT;
    tv->tv_sec += 1;
    if (!first) {
      fprintf(stderr, "NormalizedTimeval: usec too large, 2x normalize!\n");
      PrintTimeval(stderr, tv); putc('\n', stderr);
    }
    first = 0;
  }
}

/*
 * Returns failure count.  t_suspend should not be shorter than 1us,
 * since elapsed time measurement cannot possibly be any finer in
 * granularity.  In practice, 1ms is probably the best we can hope for
 * in timer resolution, so even if nanosleep suspends for 1us, the
 * gettimeofday resolution may cause a false failure report.
 */
int TestNanoSleep(struct timespec *t_suspend,
                  uint64_t        slop_ms) {
  struct timespec t_remain;
  struct timeval  t_start;
  int             rv;
  struct timeval  t_end;
  struct timeval  t_elapsed;

  printf("%40s: ", "Requesting nanosleep duration");
  PrintTimespec(stdout, t_suspend);
  printf(" seconds\n");
  t_remain = *t_suspend;
  /*
   * BUG: ntp or other time adjustments can mess up timing.
   * BUG: time-of-day clock resolution may be not be fine enough to
   * measure nanosleep duration.
   */
  if (-1 == gettimeofday(&t_start, NULL)) {
    printf("gettimeofday for start time failed\n");
    return 1;
  }
  while (-1 == (rv = nanosleep(&t_remain, &t_remain)) &&
         EINTR == errno) {
  }
  if (-1 == rv) {
    printf("nanosleep failed, errno = %d\n", errno);
    return 1;
  }
  if (-1 == gettimeofday(&t_end, NULL)) {
    printf("gettimeofday for end time failed\n");
    return 1;
  }

  /*
   * We add a microsecond in case of rounding/synchronization issues
   * between the nanosleep/scheduler clock and the time-of-day clock,
   * where the time-of-day clock doesn't _quite_ get incremented in
   * time even though the entire nanosleep duration had passed.
   * (We've seen this occur on the mac.)
   */

  t_elapsed.tv_sec = t_end.tv_sec - t_start.tv_sec;
  t_elapsed.tv_usec = t_end.tv_usec - t_start.tv_usec + 1;

  NormalizeTimeval(&t_elapsed);
  printf("%40s: ", "Actual nanosleep duration");
  PrintTimeval(stdout, &t_elapsed);
  printf(" seconds\n");

  /*
   * On WinXP, Sleep(num_ms) sometimes -- though rarely -- return
   * earlier than it is supposed to.  This may be due to gettimeofday
   * issues when running on VMs, rather than actual insomnia.  In any
   * case, We permit adding in some slop here to the elapsed time so
   * that we can ignore the sporadic random test failures that would
   * occur.
   */
  t_elapsed.tv_usec += slop_ms * MICROS_PER_MILLI;
  NormalizeTimeval(&t_elapsed);
  printf("%40s: ", "Slop adjusted duration");
  PrintTimeval(stdout, &t_elapsed);
  printf(" seconds\n");

  if (t_elapsed.tv_sec < t_suspend->tv_sec ||
      (t_elapsed.tv_sec == t_suspend->tv_sec &&
       (NANOS_PER_MICRO * t_elapsed.tv_usec < t_suspend->tv_nsec))) {
    printf("Elapsed time too short!\n");
    printf("Error\n");
    return 1;
  } else {
    printf("OK\n");
    return 0;
  }
}

int main(int argc, char **argv) {
  int                     num_errors = 0;
  int                     ix;
  int                     opt;
  uint64_t                slop_ms = 0;

  static struct timespec  t_suspend[] = {
    { 0,   1 * NANOS_PER_MILLI, },
    { 0,   2 * NANOS_PER_MILLI, },
    { 0,   5 * NANOS_PER_MILLI, },
    { 0,  10 * NANOS_PER_MILLI, },
    { 0,  25 * NANOS_PER_MILLI, },
    { 0,  50 * NANOS_PER_MILLI, },
    { 0, 100 * NANOS_PER_MILLI, },
    { 0, 250 * NANOS_PER_MILLI, },
    { 0, 500 * NANOS_PER_MILLI, },
    { 1,   0 * NANOS_PER_MILLI, },
    { 1, 500 * NANOS_PER_MILLI, },
  };

  while (EOF != (opt = getopt(argc, argv, "s:"))) {
    switch (opt) {
      case 's':
        slop_ms = strtoull(optarg, (char **) NULL, 0);
        break;
      default:
        fprintf(stderr,
                "Usage: nanosleep_test [-s slop_for_time_compare_in_ms]\n");
        return 1;
    }
  }

  for (ix = 0; ix < sizeof t_suspend/sizeof t_suspend[0]; ++ix) {
    num_errors += TestNanoSleep(&t_suspend[ix], slop_ms);
  }

  if (0 != num_errors) {
    printf("FAILED\n");
    return 1;
  } else {
    printf("PASSED\n");
    return 0;
  }
}
