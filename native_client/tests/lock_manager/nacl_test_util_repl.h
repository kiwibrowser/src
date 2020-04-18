/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_TESTS_LOCK_MANAGER_NACL_TEST_UTIL_REPL_H_
#define NATIVE_CLIENT_TESTS_LOCK_MANAGER_NACL_TEST_UTIL_REPL_H_

#include <pthread.h>

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN

struct NaClSexpIo;
struct NaClFileLockEntry;

struct NaClFileLockTestInterface {
  /* public */
  int (*set_num_files)(struct NaClFileLockTestInterface *self,
                       size_t num_files);
  void (*set_identity)(struct NaClFileLockTestInterface *self,
                       void (*orig)(struct NaClFileLockEntry *entry,
                                    int desc),
                       struct NaClFileLockEntry *entry,
                       int desc);
  int (*take_lock)(struct NaClFileLockTestInterface *self,
                   void (*orig)(int desc),
                   int thread_number,
                   int desc);
  int (*drop_lock)(struct NaClFileLockTestInterface *self,
                   void (*orig)(int desc),
                   int thread_number,
                   int desc);
  /* impl will extend this */
};

void ReadEvalPrintLoop(struct NaClSexpIo *input,
                       int interactive,
                       int verbosity,
                       size_t epsilon_delay_nanos,
                       struct NaClFileLockTestInterface *test_if);

EXTERN_C_END

#endif
