/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This is a death test that creates a security race condition that
 * should be detected.
 *
 * Here is the approach:
 *
 * - Create a block of shared memory and map it somewhere.
 *
 * - Create a socketpair.
 *
 * - Create thread which reads from socketpair into shared memory,
 *   which should block since nobody is writing to other end of
 *   socketpair.  Alternate version is to create a socket
 *   address/bound socket pair, connect, and recv into shared memory.
 *
 * - Main thread re-maps over same memory, triggering the I/O memory
 *   race condition detector and crashing the application.
 *
 * We use shared memory rather than anonymous mmap in case we decide,
 * in the service runtime, to notice that the memory is already backed
 * by anonymous memory and simply perform VirtualProtect changes and
 * not actually opening a memory mapping hole that is subject to the
 * race condition.
 *
 * NB: this test is inherently racy -- the main thread should not mmap
 * until the other thread is definitely doing a read or imc_recvmsg
 * syscall, but there's no way for the other thread to atomically
 * signal the main thread and drop into the kernel syscall handler.
 *
 * Furthermore, while we can run the mmap repeatedly (up to some
 * limit) as well, that is also subject to the whims of the scheduler.
 * If the scheduler only runs the mmapping thread and not the I/O read
 * (until the mmap retry limit is reached), the test will fail as well
 * due to scheduling, which is also flakey.
 *
 * Of course, we are hopefully not dealing with a malicious scheduler,
 * and we could just repeat the mmap without limit.
 *
 * To decrease the probability that we waste lots of cycles doing
 * mmap, we first have the main thread yield a bunch of times, and
 * appeal to the scheduler gods.  If the scheduler gods have been
 * angered and the I/O thread still doesn't get run, we essentially
 * drop into an mmap infinite loop.
 *
 * This means that on an extremely loaded machine with an angry,
 * malevolent, and vengeful scheduler, this test can be flakey, due to
 * the test timing out because of the mmap loop.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>

#include "native_client/src/public/imc_syscalls.h"
#include "native_client/src/public/imc_types.h"

#define DEFAULT_PREYIELD_COUNT 8
#define SHM_PAGE_BYTES      (1<<16)
#define DEFAULT_SHM_PAGES   2

int verbose = 0;

enum TestMode {
  TEST_CONNECTED_SOCKET,
  TEST_SOCKET_PAIR,
  TEST_READ,  /* probably not possible on waterfall; need blocking descriptor */
};

/* indices match enum value */
const char *mode_name[] = {
  "imc_connect",
  "socketpair",
  "read",
};

struct TestState {
  uint32_t      yield_count;
  enum TestMode mode;
  size_t        shm_bytes;

  int           d_shm;
  void          *shm_start;

  int           d_pair[2];  /* sock and addr, or pair */
};

void *thread_main(void *p) {
  struct TestState        *tp = (struct TestState *) p;
  int                     d = -1;
  struct NaClAbiNaClImcMsgHdr h;
  struct NaClAbiNaClImcMsgIoVec v;
  ssize_t                 rv;

  switch (tp->mode) {
    case TEST_CONNECTED_SOCKET:
      if (verbose) {
        printf("imc_connect on %d\n", tp->d_pair[1]);
      }
      d = imc_connect(tp->d_pair[1]);
      if (-1 == d) {
        fprintf(stderr, "mmap_race: imc_connect failed\n");
        exit(16);
      }
      break;
    case TEST_SOCKET_PAIR:
    case TEST_READ:
      d = tp->d_pair[0];
      break;
  }
  if (TEST_READ == tp->mode) {
    if (verbose) {
      printf("reading from %d\n", d);
    }
    rv = read(d, tp->shm_start, SHM_PAGE_BYTES);
    if (-1 == rv) {
      perror("mmap_race: thread_main");
    }
    fprintf(stderr, "mmap_race:thread_main: read returned %zd\n", rv);
  } else {
    h.iov = &v;
    h.iov_length = 1;
    h.descv = NULL;
    h.desc_length = 0;
    h.flags = 0;

    v.base = tp->shm_start;
    v.length = SHM_PAGE_BYTES;

    if (verbose) {
      printf("imc_recvmsg from %d\n", d);
    }
    rv = imc_recvmsg(d, &h, 0);
    if (-1 == rv) {
      perror("mmap_race: thread_main");
    }
    fprintf(stderr, "mmap_race:thread_main: imc_recvmsg returned %zd\n", rv);
  }
  fprintf(stderr,
          "thread_main: should never reach here!  blocking I/O didn't.\n");
  exit(17);
  return NULL;
}

int main(int ac, char **av) {
  struct TestState  tstate;
  int               opt;
  size_t            ix;
  pthread_t         tid;
  int               rv;
  uint32_t          yields_so_far;
  void              *remap_addr;

  tstate.yield_count = DEFAULT_PREYIELD_COUNT;
  tstate.mode = TEST_CONNECTED_SOCKET;
  tstate.shm_bytes = DEFAULT_SHM_PAGES * SHM_PAGE_BYTES;

  while (-1 != (opt = getopt(ac, av, "m:p:vy:"))) {
    switch (opt) {
      case 'm':
        for (ix = 0; ix < sizeof mode_name/sizeof mode_name[0]; ++ix) {
          if (!strcmp(optarg, mode_name[ix])) {
            break;
          }
        }
        if (ix == sizeof mode_name / sizeof mode_name[0]) {
          fprintf(stderr, "mmap_race: test mode %s unknown\n", optarg);
          fprintf(stderr, "mmap_race: test mode must be one of:\n");
          for (ix = 0; ix < sizeof mode_name/sizeof mode_name[0]; ++ix) {
            fprintf(stderr, "           %s\n", mode_name[ix]);
          }
          return 1;
        }
        tstate.mode = (enum TestMode) ix;
        break;
      case 'p':
        tstate.shm_bytes = strtoul(optarg, (char **) NULL, 0) * SHM_PAGE_BYTES;
        break;
      case 'v':
        verbose = 1;
        break;
      case 'y':
        tstate.yield_count = strtoul(optarg, (char **) NULL, 0);
        break;
      default:
        fprintf(stderr, ("mmap_race [-v] [-p shm_pages] [-m test_mode]"
                         " [-y yield_count]\n"));
        return 1;
    }
  }

  if (verbose) {
    printf("Testing using %s\n", mode_name[tstate.mode]);
  }

  switch (tstate.mode) {
    case TEST_CONNECTED_SOCKET:
      if (-1 == imc_makeboundsock(tstate.d_pair)) {
        perror("mmap_race");
        fprintf(stderr, "mmap_race: imc_makeboundsock failed\n");
        return 2;
      }
      if (verbose) {
        printf("imc_makeboundsock: %d %d\n",
               tstate.d_pair[0], tstate.d_pair[1]);
      }
      break;
    case TEST_SOCKET_PAIR:
      if (-1 == imc_socketpair(tstate.d_pair)) {
        perror("mmap_race");
        fprintf(stderr, "mmap_race: imc_socketpair failed\n");
        return 3;
      }
      if (verbose) {
        printf("imc_socketpair: %d %d\n", tstate.d_pair[0], tstate.d_pair[1]);
      }
      break;
    case TEST_READ:
      tstate.d_pair[0] = 0;  /* stdin */
      break;
  }

  tstate.d_shm = imc_mem_obj_create(tstate.shm_bytes);
  if (-1 == tstate.d_shm) {
    perror("mmap_race");
    fprintf(stderr, "mmap_race: imc_mem_obj_create failed\n");
    return 4;
  }
  tstate.shm_start = mmap((void *) NULL,
                          SHM_PAGE_BYTES,
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED,
                          tstate.d_shm,
                          (off_t) 0);
  if (MAP_FAILED == tstate.shm_start) {
    perror("mmap_race");
    fprintf(stderr, "mmap_race: mmap failed\n");
    return 5;
  }
  if (0 != (rv = pthread_create(&tid, (const pthread_attr_t *) NULL,
                                thread_main, &tstate))) {
    perror("mmap_race");
    fprintf(stderr, "mmap_race: pthread_create failed\n");
    return 6;
  }
  if (TEST_CONNECTED_SOCKET == tstate.mode) {
    int d;

    if (verbose) {
      printf("imc_accept on %d\n", tstate.d_pair[0]);
    }
    d = imc_accept(tstate.d_pair[0]);
    if (-1 == d) {
      perror("mmap_race");
      fprintf(stderr, "mmap_race: imc_accept failed\n");
      return 7;
    }
  }

  for (yields_so_far = 0; yields_so_far < tstate.yield_count; ++yields_so_far) {
    sched_yield();
  }

  for (;;) {
    remap_addr = mmap((void *) tstate.shm_start,
                      SHM_PAGE_BYTES,
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_FIXED,
                      tstate.d_shm,
                      (off_t) 0);
    if (MAP_FAILED == remap_addr) {
      perror("mmap_race");
      fprintf(stderr, "mmap_race: could not mmap over existing location\n");
      return 8;
    }
    if (remap_addr != tstate.shm_start) {
      fprintf(stderr, "mmap_race: mmap succeeded but did not overmap!\n");
      return 9;
    }
    sched_yield();
  }
}
