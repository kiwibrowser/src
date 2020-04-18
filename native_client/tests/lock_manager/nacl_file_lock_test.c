/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/include/portability_io.h"
#include "native_client/src/include/portability_string.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/posix/nacl_file_lock.h"
#include "native_client/src/shared/platform/posix/nacl_file_lock_intern.h"
#include "native_client/tests/lock_manager/nacl_test_util_repl.h"
#include "native_client/tests/lock_manager/nacl_test_util_sexp.h"

static int g_test_driver_verbosity = 0;

struct NaClExitCleanupList {
  struct NaClExitCleanupList *next;
  void (*fn)(void *arg);
  void *arg;
} *g_NaCl_exit_cleanup_list = NULL;

void NaClExitCleanupRmdir(void *arg) {
  char *dirname = (char *) arg;
  if (g_test_driver_verbosity) {
    printf("cleanup rmdir %s\n", dirname);
  }
  (void) rmdir(dirname);
  free(arg);
}

void NaClExitCleanupUnlink(void *arg) {
  char *fname = (char *) arg;
  if (g_test_driver_verbosity) {
    printf("cleanup unlink %s\n", fname);
  }
  (void) unlink(fname);
  free(arg);
}

void NaClExitCleanupPush(void (*fn)(void *), void *arg) {
  struct NaClExitCleanupList *new_top;

  new_top = (struct NaClExitCleanupList *) malloc(sizeof *new_top);
  CHECK(NULL != new_top);
  new_top->fn = fn;
  new_top->arg = arg;
  new_top->next = g_NaCl_exit_cleanup_list;
  g_NaCl_exit_cleanup_list = new_top;
}

void NaClExitCleanupDoCleanup(void) {
  struct NaClExitCleanupList *p;
  struct NaClExitCleanupList *next;

  for (p = g_NaCl_exit_cleanup_list; NULL != p; p = next) {
    (*p->fn)(p->arg);
    next = p->next;
    free(p);
  }
  g_NaCl_exit_cleanup_list = NULL;
}

struct NaClFileLockTestImpl {
  struct NaClFileLockTestInterface base;
  pthread_mutex_t mu;
  pthread_cond_t cv;
  size_t num_files;
  int *file_locks;
};

static int NaClFileLockTestSetNumFiles(struct NaClFileLockTestInterface *vself,
                                       size_t num_files) {
  struct NaClFileLockTestImpl *self = (struct NaClFileLockTestImpl *) vself;
  size_t ix;
  int rv = 0;

  pthread_mutex_lock(&self->mu);
  for (ix = (size_t) num_files; ix < self->num_files; ++ix) {
    if (-1 != self->file_locks[ix]) {
      rv = 0;
      /* fatal error */
      goto quit;
    }
  }
  self->file_locks = realloc(self->file_locks,
                             num_files * sizeof *self->file_locks);
  CHECK(NULL != self->file_locks);
  /* cast to size_t is safe, due to < MAX_FILE pre-condition */
  for (ix = self->num_files; ix < (size_t) num_files; ++ix) {
    self->file_locks[ix] = -1;  /* not taken; no owner thread-id */
  }
  self->num_files = (size_t) num_files;
  rv = 1;
 quit:
  pthread_mutex_unlock(&self->mu);
  return rv;
}

static void NaClFileLockTestSetFileIdentityData(
    struct NaClFileLockTestInterface *vself,
    void (*orig)(struct NaClFileLockEntry *entry,
                 int desc),
    struct NaClFileLockEntry *entry,
    int desc) {
  UNREFERENCED_PARAMETER(vself);
  UNREFERENCED_PARAMETER(orig);
  entry->file_dev = 0;
  entry->file_ino = desc;
}

static int NaClFileLockTestTakeFileLock(struct NaClFileLockTestInterface *vself,
                                        void (*orig)(int desc),
                                        int thread_number,
                                        int desc) {
  struct NaClFileLockTestImpl *self = (struct NaClFileLockTestImpl *) vself;
  int success = 0;

  UNREFERENCED_PARAMETER(orig);
  pthread_mutex_lock(&self->mu);
  for (;;) {
    if (self->num_files <= (size_t) desc) {
      printf("Bad descriptor %d, num_files = %d\n",
             desc, (int) self->num_files);
      goto quit;
    }
    if (-1 == self->file_locks[desc]) {
      /* available -- take it! */
      self->file_locks[desc] = thread_number;
      success = 1;
      goto quit;
    }
    pthread_cond_wait(&self->cv, &self->mu);
  }
 quit:
  pthread_mutex_unlock(&self->mu);
  return success;
}

static int NaClFileLockTestDropFileLock(struct NaClFileLockTestInterface *vself,
                                        void (*orig)(int desc),
                                        int thread_number,
                                        int desc) {
  struct NaClFileLockTestImpl *self = (struct NaClFileLockTestImpl *) vself;
  int success = 0;

  UNREFERENCED_PARAMETER(orig);
  pthread_mutex_lock(&self->mu);
  if (self->num_files <= (size_t) desc) {
    printf("Bad descriptor %d, num_files = %d\n",
           desc, (int) self->num_files);
    goto quit;
  }
  if (thread_number != self->file_locks[desc]) {
    printf("Unlock when thread %d is not holding lock on %d\n",
           thread_number, desc);
    goto quit;
  }
  self->file_locks[desc] = -1;
  success = 1;
  pthread_cond_broadcast(&self->cv);
 quit:
  pthread_mutex_unlock(&self->mu);
  return success;
}

void NaClFileLockTestImplCtor(struct NaClFileLockTestImpl *self) {
  self->base.set_num_files = NaClFileLockTestSetNumFiles;
  self->base.set_identity = NaClFileLockTestSetFileIdentityData;
  self->base.take_lock = NaClFileLockTestTakeFileLock;
  self->base.drop_lock = NaClFileLockTestDropFileLock;
  pthread_mutex_init(&self->mu, (pthread_mutexattr_t const *) NULL);
  pthread_cond_init(&self->cv, (pthread_condattr_t const *) NULL);
  self->num_files = 0;
  self->file_locks = NULL;
}

struct NaClFileLockTestRealFileImpl {
  struct NaClFileLockTestInterface base;
  char const *tmp_dir;  /* basename for temporary files */
  pthread_mutex_t mu;
  size_t num_files;  /* monotonically non-decreasing */
  int *desc_map;  /* test uses [0..num_files), which maps to real descriptors */
};

static int NaClFileLockTestRealFileSetNumFiles(
    struct NaClFileLockTestInterface *vself,
    size_t num_files) {
  struct NaClFileLockTestRealFileImpl *self =
      (struct NaClFileLockTestRealFileImpl *) vself;
  size_t ix;
  char file_path[4096];
  int rv = 0;
  int desc;

  pthread_mutex_lock(&self->mu);
  if (num_files < self->num_files) {
    printf("RealFile set-files must be monotonically non-decreasing\n");
    rv = 0;
    goto quit;
  }
  self->desc_map = realloc(self->desc_map,
                           num_files * sizeof *self->desc_map);
  CHECK(NULL != self->desc_map);
  for (ix = self->num_files; ix < num_files; ++ix) {
    if ((size_t) SNPRINTF(file_path, sizeof file_path, "%s/%"NACL_PRIdS,
                          self->tmp_dir, ix) >= sizeof file_path) {
      printf("RealFile path too long\n");
      rv = 0;
      goto quit;
    }
    if (g_test_driver_verbosity > 0) {
      printf("creating file %s\n", file_path);
    }
    desc = OPEN(file_path, O_CREAT | O_WRONLY, 0777);
    if (-1 == desc) {
      printf("RealFile tmp file creation problem: %s\n", file_path);
      rv = 0;
      goto quit;
    }
    NaClExitCleanupPush(NaClExitCleanupUnlink, STRDUP(file_path));
    if (1 != write(desc, "0", 1)) {
      printf("RealFile tmp file (%s) write failed\n", file_path);
      rv = 0;
      goto quit;
    }
    /* save desc in lookup table */
    self->desc_map[ix] = desc;
  }
  self->num_files = num_files;
  rv = 1;
 quit:
  pthread_mutex_unlock(&self->mu);
  return rv;
}

static int NaClFileLockTestRealFileGetDesc(
    struct NaClFileLockTestRealFileImpl *self,
    int desc) {
  int real_desc;

  pthread_mutex_lock(&self->mu);
  if (desc < 0 || (size_t) desc >= self->num_files) {
    printf("RealFiles: bad desc %d\n", desc);
    real_desc = -1;
  } else {
    real_desc = self->desc_map[desc];
  }
  pthread_mutex_unlock(&self->mu);
  return real_desc;
}

static void NaClFileLockTestRealFileSetFileIdentityData(
    struct NaClFileLockTestInterface *vself,
    void (*orig)(struct NaClFileLockEntry *entry,
                 int desc),
    struct NaClFileLockEntry *entry,
    int desc) {
  struct NaClFileLockTestRealFileImpl *self =
      (struct NaClFileLockTestRealFileImpl *) vself;
  int real_desc = NaClFileLockTestRealFileGetDesc(self, desc);
  if (-1 != real_desc) {
    /*
     * This is done w/o self->mu because we never close except at Dtor.
     * This is also why num_files is monotonic non-decreasing.
     */
    if (g_test_driver_verbosity) {
      printf("real identity %d\n", real_desc);
    }
    (*orig)(entry, real_desc);
  }
}

static int NaClFileLockTestRealFileTakeFileLock(
    struct NaClFileLockTestInterface *vself,
    void (*orig)(int desc),
    int thread_number,
    int desc) {
  struct NaClFileLockTestRealFileImpl *self =
      (struct NaClFileLockTestRealFileImpl *) vself;
  int real_desc = NaClFileLockTestRealFileGetDesc(self, desc);

  UNREFERENCED_PARAMETER(thread_number);
  if (-1 == real_desc) {
    printf("RealFileTakeFileLock: Bad descriptor %d\n", desc);
    return 0;
  }
  if (g_test_driver_verbosity) {
    printf("real lock %d\n", real_desc);
  }
  (*orig)(real_desc);
  return 1;
}

static int NaClFileLockTestRealFileDropFileLock(
    struct NaClFileLockTestInterface *vself,
    void (*orig)(int desc),
    int thread_number,
    int desc) {
  struct NaClFileLockTestRealFileImpl *self =
      (struct NaClFileLockTestRealFileImpl *) vself;
  int real_desc = NaClFileLockTestRealFileGetDesc(self, desc);

  UNREFERENCED_PARAMETER(thread_number);
  if (-1 == real_desc) {
    printf("RealFileDropFileLock: Bad descriptor %d\n", desc);
    return 0;
  }
  if (g_test_driver_verbosity) {
    printf("real unlock %d\n", real_desc);
  }
  (*orig)(real_desc);
  return 1;
}

void NaClFileLockTestRealFileImplCtor(
    struct NaClFileLockTestRealFileImpl *self,
    char const *tmp_dir) {
  self->base.set_num_files = NaClFileLockTestRealFileSetNumFiles;
  self->base.set_identity = NaClFileLockTestRealFileSetFileIdentityData;
  self->base.take_lock = NaClFileLockTestRealFileTakeFileLock;
  self->base.drop_lock = NaClFileLockTestRealFileDropFileLock;
  self->tmp_dir = tmp_dir;
  pthread_mutex_init(&self->mu, (pthread_mutexattr_t const *) NULL);
  self->num_files = 0;
  self->desc_map = NULL;
}

static void read_crash(struct NaClSexpIo *p, char const *reason) {
  fprintf(stderr, "Syntax error at line %d: %s\n", p->line_num, reason);
  exit(1);
}

int main(int ac, char **av) {
  int opt;
  struct NaClSexpIo input;
  FILE *iob;
  int interactive = 0;
  int verbosity = 0;
  /*
   * See build.scons for actual delay settings; default: 10
   * millisecond
   */
  size_t epsilon_delay_nanos = 10 * NACL_NANOS_PER_MILLI;
  struct NaClFileLockTestImpl test_impl;
  struct NaClFileLockTestRealFileImpl test_real_file_impl;
  struct NaClFileLockTestInterface *test_interface = NULL;
  char const *tmp_dir = NULL;

  NaClSexpIoCtor(&input, stdin, read_crash);
  while (-1 != (opt = getopt(ac, av, "d:D:f:ivVt"))) {
    switch (opt) {
      case 'd':
        epsilon_delay_nanos =
            strtoul(optarg, (char **) NULL, 0) * NACL_NANOS_PER_MILLI;
        break;
      case 'D':
        tmp_dir = optarg;
        break;
      case 'f':
        iob = fopen(optarg, "r");
        if (NULL == iob) {
          perror("nacl_file_lock_test");
          fprintf(stderr,
                  "nacl_file_lock_test: -f test file %s could not be opened\n",
                  optarg);
          return 1;
        }
        NaClSexpIoSetIob(&input, iob);
        break;
      case 'i':
        interactive = 1;
        break;
      case 't':
        g_test_driver_verbosity++;
        break;
      case 'v':
        verbosity++;
        break;
      case 'V':
        NaClSexpIncVerbosity();
        break;
      default:
        fprintf(stderr,
                "Usage: nacl_file_lock_test [-vV] [-f fn] [-d ms]\n"
                "                           [-D directory]\n"
                "       where the flags\n\n"
                "       -v increases the test framework verbosity level\n"
                "       -V increases the s-expression parser verbosity level\n"
                "\nand\n"
                "       -f specifies the file containing the test script\n"
                "          (default in standard input)\n"
                "       -d specifies the no-event transition delay in ms\n"
                "       -D specifies the temporary directory in which test\n"
                "          temporary files would be created\n"
                );
        return 1;
    }
  }
  if (NULL == input.iob) {
    input.iob = stdin;
  }

  atexit(NaClExitCleanupDoCleanup);

  if (NULL == tmp_dir) {
    NaClFileLockTestImplCtor(&test_impl);
    test_interface = (struct NaClFileLockTestInterface *) &test_impl;
  } else {
    if (mkdir(tmp_dir, 0777) == 0) {
      NaClExitCleanupPush(NaClExitCleanupRmdir, STRDUP(tmp_dir));
    }
    NaClFileLockTestRealFileImplCtor(&test_real_file_impl, tmp_dir);
    test_interface = (struct NaClFileLockTestInterface *) &test_real_file_impl;
  }

  ReadEvalPrintLoop(&input,
                    interactive,
                    verbosity,
                    epsilon_delay_nanos,
                    test_interface);

  return 0;
}
