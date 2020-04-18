/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/public/imc_syscalls.h"
#include "native_client/src/public/imc_types.h"

static char const quote[] =
    "  En tibi norma Poli, & divae libramina Molis,"
    "  Computus atque Jovis; quas, dum primordia rerum"
    "  Pangeret, omniparens Leges violare Creator"
    "  Noluit, aeternique operis fundamina fixit."
    "  Intima panduntur victi penetralia caeli,"
    "  Nec latet extremos quae Vis circumrotat Orbes."
    "  Sol solio residens ad se jubet omnia prono"
    "  Tendere descensu, nec recto tramite currus"
    "  Sidereos patitur vastum per inane moveri;"
    "  Sed rapit immotis, se centro, singula Gyris."
    "  Jam patet horrificis quae sit via flexa Cometis;"
    "  Jam non miramur barbati Phaenomena Astri."
    "  Discimus hinc tandem qua causa argentea Phoebe"
    "  Passibus haud aequis graditur; cur subdita nulli"
    "  Hactenus Astronomo numerorum fraena recuset:"
    "  Cur remeant Nodi, curque Auges progrediuntur.";

int CreateTestFile(char const *fn, int open_flags) {
  FILE *iob;
  int d;

  iob = fopen(fn, "w");
  if (NULL == iob) {
    fprintf(stderr,
            "CreateTestFile: could not open for initial contents, errno %d\n",
            errno);
  }
  if (EOF == fputs(quote, iob)) {
    fprintf(stderr,
            "CreateTestFile: could not write initial contents, errno %d\n",
            errno);
    exit(1);
  }
  if (EOF == fclose(iob)) {
    fprintf(stderr,
            "CreateTestFile: could not close/flush initial contents,"
            " errno %d\n",
            errno);
    exit(1);
  }
  d = open(fn, open_flags);
  if (-1 == d) {
    fprintf(stderr,
            "CreateTestFile: could not open for test access, errno %d\n",
            errno);
    exit(1);
  }

  return d;
}

int CheckReadExpectations(int fd, size_t start, size_t num_bytes) {
  char buffer[1<<16];
  size_t got;
  if (num_bytes > sizeof buffer) {
    fprintf(stderr, "CheckReadExpectations: num_bytes too large\n");
    return 1;
  }
  got = read(fd, buffer, num_bytes);
  if (got != num_bytes) {
    fprintf(stderr, "CheckReadExpectations: read got %zu, expected %zu\n",
            got, num_bytes);
    return 1;
  }
  if (0 != memcmp(quote + start, buffer, num_bytes)) {
    fprintf(stderr,
            "CheckReadExpectations: data differs\n"
            " expected: %.*s\n"
            "      got: %.*s\n",
            (int) num_bytes, quote + start,
            (int) num_bytes, buffer);
    return 1;
  }
  return 0;
}

int CheckPartialRead(int fd) {
  return CheckReadExpectations(fd, 0, 100);
}

int CheckContinuedRead(int fd) {
  return CheckReadExpectations(fd, 100, 64);
}

int CheckSharedFilePosOrig(int fd) {
  return CheckReadExpectations(fd, 164, 32);
}

static int GetReplicatedDescDup(int d) {
  return dup(d);
}

struct ThreadWork {
  pthread_mutex_t mu;
  pthread_cond_t cv;
  int imc_d;
  int d;
};

static void ThreadWorkCtor(struct ThreadWork *tw, int d) {
  int err;
  err = pthread_mutex_init(&tw->mu, (pthread_mutexattr_t *) NULL);
  if (0 != err) {
    fprintf(stderr,
            "ThreadWorkCtor: pthread_mutex_init failed, errno %d\n", err);
    exit(1);
  }
  err = pthread_cond_init(&tw->cv, (pthread_condattr_t *) NULL);
  if (0 != err) {
    fprintf(stderr,
            "ThreadWorkCtor: pthread_cond_init failed, errno %d\n", err);
    exit(1);
  }
  tw->imc_d = d;
  tw->d = -1;
}

static void CheckedClose(int d, char const *component) {
  if (close(d) != 0) {
    fprintf(stderr, "%s: close failed, errno %d\n", component, errno);
    exit(1);
  }
}

static void ThreadWorkDtor(struct ThreadWork *tw) {
  int err;
  err = pthread_mutex_destroy(&tw->mu);
  if (0 != err) {
    fprintf(stderr,
            "ThreadWorkDtor: pthread_mutex_destroy failed, errno %d\n", err);
  }
  err = pthread_cond_destroy(&tw->cv);
  if (0 != err) {
    fprintf(stderr,
            "ThreadWorkDtor: pthread_cond_destroy failed, errno %d\n", err);
  }
  if (tw->d != -1) {
    CheckedClose(tw->d, "ThreadWorkDtor");
    tw->d = -1;
  }
}

static void *thread_func(void *state) {
  struct ThreadWork *tw = (struct ThreadWork *) state;
  int connected;
  struct NaClAbiNaClImcMsgHdr msg;
  int d;

  connected = imc_connect(tw->imc_d);
  if (-1 == connected) {
    fprintf(stderr, "thread_func: imc_connect failed, errno %d\n", errno);
    exit(1);
  }
  msg.iov = NULL;
  msg.iov_length = 0;
  msg.descv = &d;
  msg.desc_length = 1;
  if (0 != imc_recvmsg(connected, &msg, 0)) {
    fprintf(stderr, "thread_func: imc_recvmsg failed, errno %d\n", errno);
    exit(1);
  }
  pthread_mutex_lock(&tw->mu);
  tw->d = d;
  pthread_cond_broadcast(&tw->cv);
  pthread_mutex_unlock(&tw->mu);
  CheckedClose(connected, "thread_func: connected descriptor");
  return NULL;
}

static int GetReplicatedDescImc(int d) {
  int bound_pair[2];
  struct ThreadWork tw;
  pthread_t tid;
  int err;
  int connected;
  struct NaClAbiNaClImcMsgHdr msg;
  int got_d;

  if (0 != imc_makeboundsock(bound_pair)) {
    fprintf(stderr,
            "GetReplicatedDescImc: imc_makeboundsock failed, errno %d\n",
            errno);
    exit(1);
  }
  ThreadWorkCtor(&tw, bound_pair[1]);
  err = pthread_create(&tid, (pthread_attr_t const *) NULL,
                       thread_func, (void *) &tw);
  if (0 != err) {
    fprintf(stderr, "GetReplicatedDescImc: pthread_create failed, errno %d\n",
            err);
    exit(1);
  }

  connected = imc_accept(bound_pair[0]);
  if (-1 == connected) {
    fprintf(stderr, "GetReplicatedDescImc: imc_accept failed, errno %d\n",
            errno);
    exit(1);
  }

  msg.iov = NULL;
  msg.iov_length = 0;
  msg.descv = &d;
  msg.desc_length = 1;
  if (0 != imc_sendmsg(connected, &msg, 0)) {
    fprintf(stderr, "GetReplicatedDescImc: imc_sendmsg failed, errno %d\n",
            errno);
    exit(1);
  }

  pthread_mutex_lock(&tw.mu);
  while (tw.d == -1) {
    pthread_cond_wait(&tw.cv, &tw.mu);
  }
  got_d = tw.d;
  tw.d = -1;
  pthread_mutex_unlock(&tw.mu);

  err = pthread_join(tid, NULL);
  if (0 != err) {
    fprintf(stderr, "GetReplicatedDescImc: pthread_join failed, errno %d\n",
            err);
    exit(1);
  }

  CheckedClose(bound_pair[0], "GetReplicatedDescImc: bound_pair[0]");
  CheckedClose(bound_pair[1], "GetReplicatedDescImc: bound_pair[1]");
  CheckedClose(connected, "GetReplicatedDescImc: connected descriptor");
  ThreadWorkDtor(&tw);
  return got_d;
}

struct TestParams {
  char const *test_name;
  int (*get_desc)(int original_desc);
  int open_flags;  /* ensure O_RDONLY and O_RDWR operates the same */
};

static struct TestParams const tests[] = {
  { "dup, ronly", GetReplicatedDescDup, O_RDONLY },
  { "dup, rw", GetReplicatedDescDup, O_RDWR },
  { "imc, ronly", GetReplicatedDescImc, O_RDONLY },
  { "imc, rw", GetReplicatedDescImc, O_RDWR },
};

int main(int ac, char **av) {
  char const *test_file_dir = "/tmp/filepos_test";
  int fd, fd_copy;
  size_t err_count;
  size_t test_errors;
  size_t ix;
  int opt;
  int num_runs = 1;
  int test_run;

  while (EOF != (opt = getopt(ac, av, "c:t:"))) {
    switch (opt) {
      case 'c':
        num_runs = atoi(optarg);
        break;
      case 't':
        test_file_dir = optarg;
        break;
      default:
        fprintf(stderr,
                "Usage: filepos_test [-c run_count]\n"
                "                    [-t test_temporary_dir]\n");
        exit(1);
    }
  }

  err_count = 0;
  for (test_run = 0; test_run < num_runs; ++test_run) {
    printf("Test run %d\n\n", test_run);
    for (ix = 0; ix < NACL_ARRAY_SIZE(tests); ++ix) {
      char test_file_name[PATH_MAX];
      printf("%s\n", tests[ix].test_name);
      snprintf(test_file_name, sizeof test_file_name,
               "%s/f%d.%u", test_file_dir, test_run, ix);
      fd = CreateTestFile(test_file_name, tests[ix].open_flags);
      CheckPartialRead(fd);
      fd_copy = tests[ix].get_desc(fd);

      test_errors = CheckContinuedRead(fd_copy);
      printf("continued read, copy: %s\n",
             (0 == test_errors) ? "PASS" : "FAIL");
      err_count += test_errors;

      test_errors = CheckSharedFilePosOrig(fd);
      printf("continued read, orig: %s\n",
             (0 == test_errors) ? "PASS" : "FAIL");
      err_count += test_errors;

      CheckedClose(fd, "filepos_test: main: test file, orig fd");
      CheckedClose(fd_copy, "filepos_test: main: test file, fd copy");
    }
  }

  /* we ignore the 2^32 or 2^64 total errors case */
  return (err_count > 255) ? 255 : err_count;
}
