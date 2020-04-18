/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_THREAD_SUSPENSION_H_
#define NATIVE_CLIENT_SERVICE_RUNTIME_THREAD_SUSPENSION_H_ 1

#include <stdlib.h>

#include "native_client/src/include/nacl_compiler_annotations.h"

struct NaClAppThread;
struct NaClSignalContext;

/* These codes are only used for testing. */
enum NaClUnwindCase {
  NACL_UNWIND_in_springboard = 1,
  NACL_UNWIND_in_trampoline,
  NACL_UNWIND_in_pcrel_thunk,
  NACL_UNWIND_in_dispatch_thunk,
  NACL_UNWIND_in_syscallseg,
  NACL_UNWIND_in_tls_fast_path,
  NACL_UNWIND_after_saving_regs,
};

static INLINE const char *NaClUnwindCaseToString(
    enum NaClUnwindCase unwind_case) {
  switch (unwind_case) {
#define NACL_MAP_NAME(name) case NACL_UNWIND_##name: return #name;
    NACL_MAP_NAME(in_springboard);
    NACL_MAP_NAME(in_trampoline);
    NACL_MAP_NAME(in_pcrel_thunk);
    NACL_MAP_NAME(in_dispatch_thunk);
    NACL_MAP_NAME(in_syscallseg);
    NACL_MAP_NAME(in_tls_fast_path);
    NACL_MAP_NAME(after_saving_regs);
    default: return NULL;
#undef NACL_MAP_NAME
  }
}

void NaClGetRegistersForContextSwitch(struct NaClAppThread *natp,
                                      struct NaClSignalContext *regs,
                                      enum NaClUnwindCase *unwind_case);

#endif
