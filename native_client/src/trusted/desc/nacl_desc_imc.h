/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service runtime.  NaClDescImcDesc subclass of NaClDesc.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_IMC_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_IMC_H_

#include "native_client/src/include/portability.h"

#include "native_client/src/include/nacl_base.h"

/*
 * get NaClHandle, which is a typedef and not a struct pointer, so
 * impossible to just forward declare.
 */
#include "native_client/src/shared/imc/nacl_imc_c.h"

#include "native_client/src/shared/platform/nacl_sync.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"

EXTERN_C_BEGIN

struct NaClDescEffector;
struct NaClDescXferState;
struct NaClMessageHeader;

/*
 * IMC connected sockets.  Abstractly, the base class for
 * NaClDescImcDesc and NaClDescXferableDataDesc are identical, with a
 * protected ctor that permits NaClDescImcDescCtor and
 * NaClDescXferableDataDescCtor to set the xferable flag which sets
 * the base class to the appropriate subclass behavior.
 */
struct NaClDescImcConnectedDesc {
  struct NaClDesc           base NACL_IS_REFCOUNT_SUBCLASS;
  NaClHandle                h;
};

struct NaClDescImcDesc {
  struct NaClDescImcConnectedDesc base NACL_IS_REFCOUNT_SUBCLASS;
  /*
   * race prevention.
   */
  struct NaClMutex          sendmsg_mu;
  struct NaClMutex          recvmsg_mu;
};

struct NaClDescXferableDataDesc {
  struct NaClDescImcConnectedDesc base NACL_IS_REFCOUNT_SUBCLASS;
};

int NaClDescXferableDataDescInternalize(
    struct NaClDesc               **baseptr,
    struct NaClDescXferState      *xfer)
    NACL_WUR;

int NaClDescImcConnectedDescCtor(struct NaClDescImcConnectedDesc  *self,
                                 NaClHandle                       h)
    NACL_WUR;

int NaClDescImcDescCtor(struct NaClDescImcDesc  *self,
                        NaClHandle              d)
    NACL_WUR;

int NaClDescXferableDataDescCtor(struct NaClDescXferableDataDesc  *self,
                                 NaClHandle                       h)
    NACL_WUR;

EXTERN_C_END

#endif  // NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_IMC_H_
