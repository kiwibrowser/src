/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#define DEFAULT_CONDVAR_TIMEOUT_MS (100)
#define DEFAULT_CPU_THRESHOLD_MS   (10 + (70-10)/2)
/*
 * In the known bug
 * http://code.google.com/p/nativeclient/issues/detail?id=2804 the CPU
 * usage was about ~69% of the pthread_cond_timedwait timeout duration
 * on Linux, and 100% on OSX.  For ARM tests under qemu, the CPU usage
 * was about 10% (!).  These values were determined experimentally.
 *
 * DEFAULT_CPU_THRESHOLD_MS / DEFAULT_CONDVAR_TIMEOUT_MS should be
 * between ~10% and ~69%.  We pick the mid point.
 */

void ErrorAbort(char const *msg, int rv, char const *file, int line) {
  fprintf(stderr, "File %s, line %d: %s, errno %d\n",
          file, line, msg, rv);
  exit(1);
}

#define Error(msg, rv) do { ErrorAbort(msg, rv, __FILE__, __LINE__); } while (0)

int main(int ac, char **av) {
  int opt;
  unsigned long t_timeout_msec = DEFAULT_CONDVAR_TIMEOUT_MS;
  int rv;
  pthread_mutex_t mu;
  pthread_cond_t cv;
  struct timespec ts;
  clock_t t_start;
  clock_t t_end;
  double t_cpu_used_ms;
  unsigned long t_cpu_threshold_ms = DEFAULT_CPU_THRESHOLD_MS;

  while (-1 != (opt = getopt(ac, av, "t:m:"))) {
    switch (opt) {
      case 't':
        t_timeout_msec = strtoul(optarg, (char **) NULL, 0);
        break;
      case 'm':
        t_cpu_threshold_ms = strtoul(optarg, (char **) NULL, 0);
        break;
      default:
        fprintf(stderr,
                ("Usage: cond_timedwait [-t timeout_in_ms]"
                 " [-m max_cpu_usage_ms\n"));
        return 1;
    }
  }
  if (0 != (rv = pthread_mutex_init(&mu,
                                    (pthread_mutexattr_t const *) NULL))) {
    Error("pthread_mutex_init", rv);
  }

  if (0 != (rv = pthread_cond_init(&cv,
                                   (pthread_condattr_t const *) NULL))) {
    Error("pthread_cond_init", rv);
  }
  if (0 != (rv = pthread_mutex_lock(&mu))) {
    Error("pthread_mutex_lock", rv);
  }
  if (0 != (rv = clock_gettime(CLOCK_REALTIME, &ts))) {
    Error("clock_gettime", rv);
  }
  ts.tv_sec += t_timeout_msec / 1000;
  ts.tv_nsec += (t_timeout_msec % 1000) * 1000 * 1000;
  if (ts.tv_nsec > 1000000000) {
    ts.tv_sec++;
    ts.tv_nsec -= 1000000000;
  }
  t_start = clock();
  if (ETIMEDOUT != (rv = pthread_cond_timedwait(&cv, &mu, &ts))) {
    Error("pthread_cond_timedwait", rv);
  }
  t_end = clock();
  t_cpu_used_ms = 1000.0 * ((double) (t_end - t_start)) / CLOCKS_PER_SEC;
  printf("pthread_cond_timedwait: waited %lu ms\n", t_timeout_msec);
  printf("CPU seconds used by pthread_cond_timedwait: %f ms\n",
         t_cpu_used_ms);
  if (t_cpu_used_ms > t_cpu_threshold_ms) {
    printf("FAILED.\n");
    rv = 1;
  } else {
    printf("OK.\n");
    rv = 0;
  }
  return rv;
}
