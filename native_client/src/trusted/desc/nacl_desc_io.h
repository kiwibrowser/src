/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service runtime.  NaClDescIoDesc subclass of NaClDesc.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_IO_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_IO_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

#include "native_client/src/public/nacl_desc.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"

EXTERN_C_BEGIN

struct NaClDescEffector;
struct NaClDescXferState;
struct NaClHostDesc;
struct NaClMessageHeader;

/*
 * I/O descriptors
 */

struct NaClDescIoDesc {
  struct NaClDesc           base NACL_IS_REFCOUNT_SUBCLASS;
  /*
   * No locks are needed for accessing class members; NaClHostDesc
   * should ensure thread safety, and uses are read-only.
   *
   * If we later added state that needs locking, beware lock order.
   */
  struct NaClHostDesc       *hd;
};

int NaClDescIoInternalize(struct NaClDesc               **baseptr,
                          struct NaClDescXferState      *xfer)
    NACL_WUR;

int NaClDescIoDescCtor(struct NaClDescIoDesc  *self,
                       struct NaClHostDesc    *hd) NACL_WUR;

struct NaClDescIoDesc *NaClDescIoDescMake(struct NaClHostDesc *nhdp);

/*
 * NaClDescIoDescFromDescAllocCtor is essentially the same as
 * NaClDescIoMakeFromHandle, except that instead of NaClHandle (which is
 * HANDLE on Windows), we always use a Posix I/O descriptor.
 */
struct NaClDesc *NaClDescIoDescFromDescAllocCtor(int desc,
                                                 int flags) NACL_WUR;

/* a simple factory */
struct NaClDescIoDesc *NaClDescIoDescOpen(char const *path,
                                          int mode,
                                          int perms) NACL_WUR;

uintptr_t NaClDescIoDescMapAnon(struct NaClDescEffector *effp,
                                void                    *start_addr,
                                size_t                  len,
                                int                     prot,
                                int                     flags,
                                nacl_off64_t            offset);

EXTERN_C_END

#endif  // NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_IO_H_
