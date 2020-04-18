/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_EFFECTOR_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_EFFECTOR_H_

/*
 * This file defines an interface class that the nacl_desc* routines
 * use to manipulate record keeping data structures (if any).  This
 * eliminates the need for the nrd_xfer library to directly manipulate
 * NaClApp or NaClAppThread contents, so trusted code that wish to use
 * the nrd_xfer library can provide their own NaClDescEffector
 * implementation to stub out, for example, recording of virtual
 * memory map changes, pre-mmap unmmaping of 64K allocations according
 * to the memory object backing the page, etc.
 */

#include "native_client/src/include/nacl_base.h"

#include "native_client/src/include/portability.h"  /* uintptr_t, off_t, etc */

EXTERN_C_BEGIN

struct NaClDesc;
struct NaClDescEffectorVtbl;

/* virtual base class; no ctor, no vtbl */
struct NaClDescEffector {
  struct NaClDescEffectorVtbl const *vtbl;
};

/*
 * Virtual functions use the kernel return interface: non-negative
 * values are success codes, (small) negative values are negated
 * NACL_ABI_* errno returns.
 */

struct NaClDescEffectorVtbl {
  /*
   * For service runtime, the NaClDesc's Map virtual function will
   * call this to unmap any existing memory before mapping new pages
   * in on top.  This method should handle the necessary unmapping
   * (figure out the memory object that is backing the memory and call
   * UnmapViewOfFile or VirtualFree as appropriate).  On linux and
   * osx, this can be a no-op since mmap will override existing
   * mappings in an atomic fashion (yay!).  The sysaddr will be a
   * multiple of allocation size, as will be nbytes.
   *
   * For trusted applications, this can also be a no-op as long as the
   * application chooses a valid (not committed nor reserved) address
   * range.
   *
   * Note that UnmapMemory may be called without a corresponding
   * UpdateAddrmap (w/ delete_mem=1), since that may be deferred until
   * the memory hole has been populated with something else.
   *
   * This is NOT used by the NaClDesc's own Unmap method.
   */
  void (*UnmapMemory)(struct NaClDescEffector  *vself,
                      uintptr_t                sysaddr,
                      size_t                   nbytes);
};

EXTERN_C_END

#endif  // NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_EFFECTOR_H_
