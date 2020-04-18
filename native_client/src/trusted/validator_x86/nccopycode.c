/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * nccopycode.c
 * Copies two code streams in a thread-safe way
 *
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"

#if NACL_WINDOWS == 1
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/utils/types.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"
#include "native_client/src/trusted/validator/ncvalidate.h"

/* x86 HALT opcode */
static const uint8_t kNaClFullStop = 0xf4;

/*
 * Max size of aligned writes we may issue to code without syncing.
 * 8 is a safe value according to:
 *   [1] Advance Micro Devices Inc. AMD64 Architecture Program-
 *   mers Manual Volume 1: Application Programming, 2009.
 *   [2] Intel Inc. Intel 64 and IA-32 Architectures Software Developers
 *   Manual Volume 3A: System Programming Guide, Part 1, 2010.
 *   [3] Vijay Sundaresan, Daryl Maier, Pramod Ramarao, and Mark
 *   Stoodley. Experiences with multi-threading and dynamic class
 *   loading in a java just-in-time compiler. Code Generation and
 *   Optimization, IEEE/ACM International Symposium on, 0:87–
 *   97, 2006.
 */
static const int kTrustAligned = 8;

/*
 * Max size of unaligned writes we may issue to code.
 * Empirically this may be larger, however no docs to support it.
 * 1 means disabled.
 */
static const int kTrustUnaligned = 1;

/*
 * Boundary no write may ever cross.
 * On AMD machines must be leq 8.  Intel machines leq cache line.
 */
static const int kInstructionFetchSize = 8;

/* defined in nccopycode_stores.S */
void _cdecl onestore_memmove4(uint8_t* dst, uint8_t* src);

/* defined in nccopycode_stores.S */
void _cdecl onestore_memmove8(uint8_t* dst, uint8_t* src);

static INLINE int IsAligned(uint8_t *dst, int size, int align) {
  uintptr_t startaligned = ((uintptr_t)dst)        & ~(align-1);
  uintptr_t stopaligned  = ((uintptr_t)dst+size-1) & ~(align-1);
  return startaligned == stopaligned;
}

/*
 * Test if it is safe to issue a unsynced change to dst/size using a
 * writesize write.  Outputs the offset to start the write at.
 * 1 if it is ok, 0 if it is not ok.
 */
static int IsTrustedWrite(uint8_t *dst,
                          int size,
                          int writesize,
                          intptr_t* offset) {
  if (size > writesize) {
    return 0;
  }
  if (!IsAligned(dst, size, kInstructionFetchSize)) {
    return 0;
  }
  if (writesize <= kTrustAligned && IsAligned(dst, size, writesize)) {
    /* aligned write is trusted */
    *offset = (intptr_t) dst & (writesize - 1);
    return 1;
  }
  if (writesize <= kTrustUnaligned) {
    /* unaligned write is trusted */
    *offset = 0;
    return 1;
  }
  return 0;
}

/* this is global to prevent a (very smart) compiler from optimizing it out */
void* g_squashybuffer = NULL;
char g_firstbyte = 0;

static Bool SerializeAllProcessors(void) {
  /*
   * We rely on the OS mprotect() call to issue interprocessor interrupts,
   * which will cause other processors to execute an IRET, which is
   * serializing.
   *
   * This code is based on two main considerations:
   * 1. Only switching the page from exec to non-exec state is guaranteed
   * to invalidate processors' instruction caches.
   * 2. It's bad to have a page that is both writeable and executable,
   * even if that happens not simultaneously.
   */

  int size = NACL_MAP_PAGESIZE;
  if (NULL == g_squashybuffer) {
    if ((0 != NaClPageAlloc(&g_squashybuffer, size)) ||
        (0 != NaClMprotect(g_squashybuffer, size, PROT_READ|PROT_WRITE))) {
      NaClLog(0,
              ("SerializeAllProcessors: initial squashybuffer allocation"
               " failed\n"));
      return FALSE;
    }

    NaClFillMemoryRegionWithHalt(g_squashybuffer, size);
    g_firstbyte = *(char *) g_squashybuffer;
    NaClLog(0, "SerializeAllProcessors: g_firstbyte is %d\n", g_firstbyte);
  }

  if ((0 != NaClMprotect(g_squashybuffer, size, PROT_READ|PROT_EXEC))) {
    NaClLog(0,
            ("SerializeAllProcessors: interprocessor interrupt"
             " generation failed: could not reverse shield polarity (1)\n"));
    return FALSE;
  }
  /*
   * Make a read to ensure that the potential kernel laziness
   * would not affect this hack.
   */
  if (*(char *) g_squashybuffer != g_firstbyte) {
    NaClLog(0,
            ("SerializeAllProcessors: interprocessor interrupt"
             " generation failed: could not reverse shield polarity (2)\n"));
    NaClLog(0, "SerializeAllProcessors: g_firstbyte is %d\n", g_firstbyte);
    NaClLog(0, "SerializeAllProcessors: *g_squashybuffer is %d\n",
            *(char *) g_squashybuffer);
    return FALSE;
  }
  /*
   * We would like to set the protection to PROT_NONE, but on Windows
   * there's an ugly hack in NaClMprotect where PROT_NONE can result
   * in MEM_DECOMMIT, causing the contents of the page(s) to be lost!
   */
  if (0 != NaClMprotect(g_squashybuffer, size, PROT_READ)) {
    NaClLog(0,
            ("SerializeAllProcessors: interprocessor interrupt"
             " generation failed: could not reverse shield polarity (3)\n"));
    return FALSE;
  }
  return TRUE;
}

int NaClCopyInstruction(uint8_t *dst, uint8_t *src, uint8_t sz) {
  intptr_t offset = 0;
  uint8_t *firstbyte_p = dst;

  while (sz > 0 && dst[0] == src[0]) {
    /* scroll to first changed byte */
    dst++, src++, sz--;
  }

  if (sz == 0) {
    /* instructions are identical, we are done */
    return 1;
  }

  while (sz > 0 && dst[sz-1] == src[sz-1]) {
    /* trim identical bytes at end */
    sz--;
  }

  if (sz == 1) {
    /* we assume a 1-byte change is atomic */
    *dst = *src;
  } else if (IsTrustedWrite(dst, sz, 4, &offset)) {
    uint8_t tmp[4];
    memcpy(tmp, dst-offset, sizeof tmp);
    memcpy(tmp+offset, src, sz);
    onestore_memmove4(dst-offset, tmp);
  } else if (IsTrustedWrite(dst, sz, 8, &offset)) {
    uint8_t tmp[8];
    memcpy(tmp, dst-offset, sizeof tmp);
    memcpy(tmp+offset, src, sz);
    onestore_memmove8(dst-offset, tmp);
  } else {
    /* the slow path, first flip first byte to halt*/
    uint8_t firstbyte = firstbyte_p[0];
    firstbyte_p[0] = kNaClFullStop;

    if (!SerializeAllProcessors()) return 0;

    /* copy the rest of instruction */
    if (dst == firstbyte_p) {
      /* but not the first byte! */
      firstbyte = *src;
      dst++, src++, sz--;
    }
    memcpy(dst, src, sz);

    if (!SerializeAllProcessors()) return 0;

    /* flip first byte back */
    firstbyte_p[0] = firstbyte;
  }
  return 1;
}
