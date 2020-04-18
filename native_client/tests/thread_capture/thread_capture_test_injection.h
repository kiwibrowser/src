/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_TESTS_THREAD_CAPTURE_THREAD_CAPTURE_TEST_INJECTION_H_
#define NATIVE_CLIENT_TESTS_THREAD_CAPTURE_THREAD_CAPTURE_TEST_INJECTION_H_

#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

struct NaClApp;

void NaClInjectThreadCaptureSyscall(struct NaClApp *nap);

/*
 * This is a text section label, but not code that actually follows
 * the C calling convention.  We only want this for getting the
 * address.  Do not call!
 */
void NaClSyscallThreadCaptureFault(void);
void NaClSyscallThreadCaptureFaultSSE(void);
void NaClSyscallThreadCaptureFaultNoSSE(void);

extern uintptr_t g_nacl_syscall_thread_capture_fault_addr;

#endif
