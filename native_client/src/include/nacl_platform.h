/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_INCLUDE_NACL_PLATFORM_H_
#define NATIVE_CLIENT_SRC_INCLUDE_NACL_PLATFORM_H_ 1

/* elf stuff */
/* ... */

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/elf.h"

#if NACL_LINUX || NACL_OSX
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#endif

/* mmap enums, e.g.  PROT_READ, PROT_WRITE*/
#if NACL_WINDOWS
#include "native_client/src/include/win/mman.h"
#elif NACL_OSX
# include <sys/mman.h>
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
#elif NACL_LINUX
# include <sys/mman.h>
#endif

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_NACL_PLATFORM_H_ */
