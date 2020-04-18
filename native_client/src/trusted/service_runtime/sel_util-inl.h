/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL) misc utilities.  Inlined
 * functions.  Internal; do not include.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_UTIL_INL_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_UTIL_INL_H_ 1

#include "native_client/src/include/build_config.h"

/*
 * NaClRoundPage is a bit of a misnomer -- it always rounds up to a
 * page size, not the nearest.
 */
static INLINE size_t  NaClRoundPage(size_t    nbytes) {
  return (nbytes + NACL_PAGESIZE - 1) & ~((size_t) NACL_PAGESIZE - 1);
}

static INLINE uint32_t  NaClRoundPage32(uint32_t    nbytes) {
  return (nbytes + NACL_PAGESIZE - 1) & ~((size_t) NACL_PAGESIZE - 1);
}

static INLINE size_t  NaClRoundAllocPage(size_t    nbytes) {
  return (nbytes + NACL_MAP_PAGESIZE - 1) & ~((size_t) NACL_MAP_PAGESIZE - 1);
}

static INLINE uint32_t  NaClRoundAllocPage32(uint32_t    nbytes) {
  return (nbytes + NACL_MAP_PAGESIZE - 1) & ~((uint32_t)NACL_MAP_PAGESIZE - 1);
}

static INLINE size_t NaClTruncPage(size_t  nbytes) {
  return nbytes & ~((size_t) NACL_PAGESIZE - 1);
}

static INLINE size_t NaClTruncAllocPage(size_t  nbytes) {
  return nbytes & ~((size_t) NACL_MAP_PAGESIZE - 1);
}

static INLINE size_t  NaClBytesToPages(size_t nbytes) {
  return (nbytes + NACL_PAGESIZE - 1) >> NACL_PAGESHIFT;
}

static INLINE int /* bool */ NaClIsPageMultiple(uintptr_t addr_or_size) {
  return 0 == ((NACL_PAGESIZE - 1) & addr_or_size);
}

static INLINE int /* bool */ NaClIsAllocPageMultiple(uintptr_t addr_or_size) {
  return 0 == ((NACL_MAP_PAGESIZE - 1) & addr_or_size);
}

/*
 * True host-OS allocation unit.
 */
static INLINE size_t NaClRoundHostAllocPage(size_t  nbytes) {
#if NACL_WINDOWS
  return NaClRoundAllocPage(nbytes);
#else   /* NACL_WINDOWS */
  return NaClRoundPage(nbytes);
#endif  /* !NACL_WINDOWS */
}

static INLINE size_t NaClRoundPageNumUpToMapMultiple(size_t npages) {
  return (npages + NACL_PAGES_PER_MAP - 1) & ~((size_t) NACL_PAGES_PER_MAP - 1);
}

static INLINE size_t NaClTruncPageNumDownToMapMultiple(size_t npages) {
  return npages & ~((size_t) NACL_PAGES_PER_MAP - 1);
}

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_UTIL_INL_H_ */
