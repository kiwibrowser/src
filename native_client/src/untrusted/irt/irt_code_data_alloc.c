/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/mman.h>

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/irt/irt_private.h"

/*
 * Application data by default begins at 256MB. Currently we do a linear
 * search for code allocations so if we allow code/data separations smaller
 * than this amount the allocated code will begin in the middle of our allowed
 * code space and the unused address space before that point will be wasted.
 */
#define MIN_DATA_SEP (256 * 1024 * 1024)

/* This is the start of the code segment for the IRT. */
extern const char __executable_start[];

static uintptr_t g_next_available_code = -1;
static pthread_mutex_t g_code_mutex = PTHREAD_MUTEX_INITIALIZER;

void irt_reserve_code_allocation(uintptr_t code_begin, size_t code_size) {
  CHECK(0 == pthread_mutex_lock(&g_code_mutex));

  if (g_next_available_code == (uintptr_t) -1 ||
      g_next_available_code < code_begin + code_size) {
    g_next_available_code = code_begin + code_size;
  }

  CHECK(0 == pthread_mutex_unlock(&g_code_mutex));
}

int nacl_irt_code_data_allocate(uintptr_t hint, size_t code_size,
                                uintptr_t data_offset, size_t data_size,
                                uintptr_t *begin) {
  const uintptr_t max_code_addr = (uintptr_t) &__executable_start;
  int ret = EINVAL;
  uintptr_t try_code_addr;

  if (code_size == 0) {
    NaClLog(LOG_WARNING, "nacl_irt_code_data_allocate: "
            "Code size must be non-zero. "
            "Single regions of memory can be allocated using mmap directly.\n");
    return EINVAL;
  }
  if (code_size % NACL_MAP_PAGESIZE != 0 ||
      data_size % NACL_MAP_PAGESIZE != 0 ||
      data_offset % NACL_MAP_PAGESIZE != 0 ||
      hint % NACL_MAP_PAGESIZE != 0) {
    NaClLog(LOG_WARNING, "nacl_irt_code_data_allocate: "
            "Could not allocate code and data, sizes must be page aligned. "
            "Page: 0x%x, Hint: 0x%x, Code Size: 0x%x, "
            "Data Size: 0x%x, Offset: 0x%x.\n",
            NACL_MAP_PAGESIZE, hint, code_size, data_size, data_offset);
    return EINVAL;
  }
  if (data_size != 0 && data_offset < MIN_DATA_SEP) {
    NaClLog(LOG_WARNING, "nacl_irt_code_data_allocate: "
            "Code and Data separation must be larger than the minimum. "
            "Data offset greater or equal to 0x%x. "
            "Data Offset: 0x%x.\n",
            MIN_DATA_SEP, data_offset);
    return EINVAL;
  }

  CHECK(0 == pthread_mutex_lock(&g_code_mutex));
  try_code_addr = g_next_available_code;

  if (try_code_addr == (uintptr_t) -1)
    try_code_addr = g_dynamic_text_start;

  /*
   * If a hint is passed in, automatically increment the next available code
   * variable to be the hint if it is larger. If it is smaller then we
   * assume it is not available anyways.
   */
  if (hint != 0 && hint > try_code_addr)
    try_code_addr = hint;

  /*
   * Continually try each code/data address space until we find a match.
   * TODO (dyen): This is the very first dead simple approach. We will need to
   * develop a way to interlock the book keeping with mmap so that it reserves
   * code segments and does not conflict.
   */
  if (try_code_addr + code_size <= max_code_addr) {
    if (data_size == 0) {
      ret = 0;
    } else {
      for (;
           try_code_addr + code_size <= max_code_addr;
           try_code_addr += code_size) {
        const uintptr_t try_data_addr = try_code_addr + data_offset;
        void *data_addr = mmap((void *) try_data_addr,
                               data_size,
                               PROT_NONE,
                               MAP_PRIVATE | MAP_ANONYMOUS,
                               -1,
                               0);

        if (data_addr != MAP_FAILED) {
          /*
           * If mmap adjusted the data, code must also be adjusted accordingly.
           * We can only accept positive adjustments to the data address though,
           * if mmap has adjusted the data address lower we must try a higher
           * hint so that the code segments do not overlap.
           */
          if ((uintptr_t) data_addr > try_data_addr) {
            try_code_addr += (uintptr_t) data_addr - try_data_addr;
            ret = 0;
            break;
          } else if ((uintptr_t) data_addr == try_data_addr) {
            ret = 0;
            break;
          }

          if (0 != munmap(data_addr, data_size)) {
            ret = errno;
            NaClLog(LOG_FATAL, "nacl_irt_code_data_allocate: "
                    "Failed to unmap allocated data segment:"
                    " addr 0x%08x, error %d (%s)\n",
                    (uintptr_t) data_addr, ret, strerror(ret));
          }
        } else {
          ret = errno;
          NaClLog(LOG_WARNING, "nacl_irt_code_data_allocate: "
                  "Failed to mmap data segment:"
                  " addr 0x%08x, error %d (%s)\n",
                  (uintptr_t) try_data_addr, ret, strerror(ret));
          break;
        }
      }
    }
  }

  /*
   * The first pass implementation only keeps a single code address pointer so
   * it is not aware of any holes that can be later used or data that has been
   * unmapped. This should be okay for now since this case only exists if all
   * 256 MB has been used up. Eventually when we have more robust book keeping
   * we will be able to fill in holes and know when all the address space has
   * actually been used.
   */
  if (try_code_addr + code_size > max_code_addr) {
    NaClLog(LOG_WARNING, "nacl_irt_code_data_allocate: "
            "Maximum code segment has been reached: 0x%08x\n",
            max_code_addr);
    ret = ENOMEM;
  } else if (ret == 0) {
    g_next_available_code = try_code_addr + code_size;
    *begin = try_code_addr;
  }

  CHECK(0 == pthread_mutex_unlock(&g_code_mutex));
  return ret;
}

const struct nacl_irt_code_data_alloc nacl_irt_code_data_alloc = {
  nacl_irt_code_data_allocate,
};
