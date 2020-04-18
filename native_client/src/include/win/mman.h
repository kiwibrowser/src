/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * mman.h for windows.  since we only use #define constants for the
 * virtual memory map data structure, the values do not have to be
 * compatible with linux or osx values.
 */

#ifndef NATIVE_CLIENT_SRC_INCLUDE_WIN_MMAN_H_
#define NATIVE_CLIENT_SRC_INCLUDE_WIN_MMAN_H_

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

#define PROT_NONE   0
#define PROT_READ   1
#define PROT_WRITE  2
#define PROT_EXEC   4

#define MAP_SHARED    1
#define MAP_PRIVATE   2
#define MAP_ANONYMOUS 4
#define MAP_FIXED     8

#define MAP_FAILED ((void *) -1)

#define MADV_NORMAL     0
#define MADV_RANDOM     1
#define MADV_SEQUENTIAL 2
#define MADV_WILLNEED   3
#define MADV_DONTNEED   4

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_WIN_MMAN_H_ */
