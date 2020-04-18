/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl run time.
 */

#include <fcntl.h>

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"

#include "native_client/src/trusted/desc/nacl_desc_io.h"

#include "native_client/src/trusted/service_runtime/arch/x86/nacl_ldt_x86.h"
#include "native_client/src/trusted/service_runtime/nacl_app.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"


static uint16_t NaClAllocateSegmentForCodeRegion(struct NaClApp *nap) {
  uintptr_t code_start = nap->mem_start;
  size_t    code_bytes = nap->dynamic_text_end;
  size_t    code_pages = code_bytes >> NACL_PAGESHIFT;

  VCHECK((code_bytes & ((1 << NACL_PAGESHIFT) - 1)) == 0,
        ("code_bytes (0x%08"NACL_PRIxS") is not page aligned\n",
         code_bytes));

  if (code_pages < 1) {
    NaClLog(LOG_FATAL,
            "NaClAllocateSegmentForCodeRegion: fewer than one code pages?\n");
  }
  NaClLog(2,
          "NaClLdtAllocatePageSelector(code, 1, 0x%08"
          NACL_PRIxPTR", 0x%"NACL_PRIxS"\n",
          code_start, code_pages);

  return NaClLdtAllocatePageSelector(NACL_LDT_DESCRIPTOR_CODE,
                                     1,
                                     (void *) code_start,
                                     code_pages);
}


/*
 * NB: in our memory model, we roughly follow standard 7th edition unix but with
 * a >16-bit address space: data and code overlap, and the start of the data
 * segment is the same as the start of the code region; and the data segment
 * actually includes the memory hole between the break and the top of the stack,
 * as well as the stack and environment variables and other things in memory
 * above the stack.
 *
 * The code pages, which is marked read-only via the page protection mechanism,
 * could be viewed as read-only data.  Nothing prevents a NaCl application from
 * looking at its own code.
 *
 * The same segment selector is used for ds, es, and ss, and thus "des_seg".
 * Nuthin' to do with the old Data Encryption Standard.
 */
static uint16_t NaClAllocateSegmentForDataRegion(struct NaClApp *nap) {
  uintptr_t           data_start = nap->mem_start;
  size_t              data_pages = ((size_t) 1U <<
                                    (nap->addr_bits - NACL_PAGESHIFT));

  CHECK(nap->addr_bits > NACL_PAGESHIFT);

  if (data_pages < 1) {
    NaClLog(LOG_FATAL,
            "NaClAllocateSegmentForDataRegion: address space"
            " is fewer than one page?\n");
  }
  NaClLog(2,
          "NaClLdtAllocatePageSelector(data, 1, 0x%08"NACL_PRIxPTR", "
          "0x%"NACL_PRIxS"\n",
          data_start, data_pages - 1);

  return NaClLdtAllocatePageSelector(NACL_LDT_DESCRIPTOR_DATA,
                                     0,
                                     (void *) data_start,
                                     data_pages);
}


/*
 * Allocate ldt for app, without creating the main thread.
 */
NaClErrorCode NaClAppPrepareToLaunch(struct NaClApp     *nap) {
  uint16_t            cs;
  uint16_t            des_seg;

  NaClErrorCode       retval = LOAD_INTERNAL;

  NaClXMutexLock(&nap->mu);

  cs = NaClAllocateSegmentForCodeRegion(nap);

  NaClLog(2, "got 0x%x\n", cs);
  if (0 == cs) {
    retval = SRT_NO_SEG_SEL;
    goto done;
  }

  des_seg = NaClAllocateSegmentForDataRegion(nap);

  NaClLog(2, "got 0x%x\n", des_seg);
  if (0 == des_seg) {
    NaClLdtDeleteSelector(cs);
    retval = SRT_NO_SEG_SEL;
    goto done;
  }

  nap->code_seg_sel = cs;
  nap->data_seg_sel = des_seg;

  /*
   * Note that gs is thread-specific and not global, so that is allocated
   * elsewhere.  See nacl_app_thread.c.
   */

  retval = LOAD_OK;
done:
  NaClXMutexUnlock(&nap->mu);
  return retval;
}
