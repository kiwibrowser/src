/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef NATIVE_CLIENT_TESTS_DYNAMIC_CODE_LOADING_DYNAMIC_SEGMENT_H_
#define NATIVE_CLIENT_TESTS_DYNAMIC_CODE_LOADING_DYNAMIC_SEGMENT_H_

#include <stdint.h>

extern char _etext[];
/* TODO(sehr): add a sysconf to get the page size, rather than this magic. */
#define DYNAMIC_CODE_PAGE_SIZE     (0x10000)
#define DYNAMIC_CODE_ALIGN(addr)   \
    ((((uintptr_t) (addr)) + DYNAMIC_CODE_PAGE_SIZE - 1) & \
     ~(DYNAMIC_CODE_PAGE_SIZE - 1))

#define DYNAMIC_CODE_SEGMENT_START (DYNAMIC_CODE_ALIGN(_etext))

/*
 * DYNAMIC_CODE_SEGMENT_ENDS also exists, is defined by nacl.scons.
 */

#endif  // NATIVE_CLIENT_TESTS_DYNAMIC_CODE_LOADING_DYNAMIC_SEGMENT_H_
