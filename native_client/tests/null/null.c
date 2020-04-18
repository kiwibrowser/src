/*
 * Copyright (c) 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "native_client/src/trusted/service_runtime/include/sys/nacl_syscalls.h"

int main(int  ac,
         char **av) {
  int             opt;
  int             num_rep = 10000000;
  int             i;
  struct timeval  time_start;
  struct timeval  time_end;
  struct timeval  time_elapsed;
  double          time_per_call;

  while (EOF != (opt = getopt(ac, av, "r:"))) switch (opt) {
    case 'r':
      num_rep = strtol(optarg, (char **) NULL, 0);
      break;
    default:
      fprintf(stderr, "Usage: null [-r repetition_count]\n");
      return 1;
  }
  gettimeofday(&time_start, (void *) NULL);
  for (i = num_rep; --i >= 0; ) {
    null_syscall();
  }
  gettimeofday(&time_end, (void *) NULL);
  time_elapsed.tv_sec = time_end.tv_sec - time_start.tv_sec;
  time_elapsed.tv_usec = time_end.tv_usec - time_start.tv_usec;
  if (time_elapsed.tv_usec < 0) {
    --time_elapsed.tv_sec;
    time_elapsed.tv_usec += 1000000;
  }
  printf("Number of null syscalls: %d\n", num_rep);
  printf("Elapsed time: %d.%06dS\n",
         (int) time_elapsed.tv_sec,
         (int) time_elapsed.tv_usec);
  time_per_call = ((time_elapsed.tv_sec + time_elapsed.tv_usec / 1.0e6)
                   / num_rep);
  printf("Time per call: %gS or %fnS\n",
         time_per_call,
         1.0e9 * time_per_call);
  printf("Calls per sec: %d\n", (int) (1.0 / time_per_call));
  return 0;
}
