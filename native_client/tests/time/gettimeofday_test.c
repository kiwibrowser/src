/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "native_client/src/trusted/service_runtime/include/machine/_types.h"

/*
 * The mocking of gettimeofday is only useful for testing the libc
 * code, i.e., it's not very useful from the point of view of testing
 * Native Client.  To test the NaCl gettimeofday syscall
 * implementation, we need to match the "At the tone..." message and
 * do a approximate comparison with the output of python's
 * time.time().  I don't expect that we can actually require that they
 * match up to more than a few seconds, especialy for the heavily
 * loaded continuous build/test machines.
 */

int main(int ac,
         char **av) {
  int             opt;
  struct timeval  t_now;
  struct tm       t_broken_out;
  char            ctime_buf[26];
  char            *timestr;
  int             use_real_time = 1;
  time_t          mock_time = 0;

  while (EOF != (opt = getopt(ac, av, "mt:"))) {
    switch (opt) {
      case 'm':
        use_real_time = 0;
        break;
      case 't':
        mock_time = strtol(optarg, (char **) NULL, 0);
        break;
      default:
        fprintf(stderr,
                "Usage: sel_ldr [sel_ldr-options]"
                " -- gettimeofday_test.nexe [-m]\n");
        return 1;
    }
  }
  if (use_real_time) {
    if (0 != (*gettimeofday)(&t_now, NULL)) {
      perror("gettimeofday_test: gettimeofday failed");
      printf("FAIL\n");
      return 2;
    }
  } else {
    t_now.tv_sec = mock_time;
    t_now.tv_usec = 0;
  }
  printf("At the tone, the system time is %"PRId64".%06ld seconds.  BEEP!\n",
         (int64_t) t_now.tv_sec, t_now.tv_usec);
  localtime_r(&t_now.tv_sec, &t_broken_out);
  timestr = asctime_r(&t_broken_out, ctime_buf);
  if (NULL == timestr) {
    printf("FAIL\n");
  }
  printf("%s\n", timestr);
  return 0;
}
