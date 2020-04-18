/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaClDescQuota subclass of NaClDesc, which contains other NaClDescs.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_QUOTA_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_QUOTA_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"

#include "native_client/src/shared/platform/nacl_sync.h"

EXTERN_C_BEGIN

/*
 * When quota limit is reached, then a short write operation is
 * performed unless quota says zero bytes can be transferred, in which
 * case we will return -NACL_ABI_EDQUOT, disk quota exceeded.
 *
 * This wrapper is for enforcing disk storage quotas, not I/O transfer
 * rates.  A version of this that has read/write/sendmsg/recvmsg
 * quotas can be done as well, though the RateLimitQuota interface
 * should include a CondVar to sleep on to avoid polling, with an
 * upcall mechanism to deliver additional quota.
 */

#define NACL_DESC_QUOTA_FILE_ID_LEN   (16)
/*
 * An ID of 128 bits should suffice for random IDs to not collide,
 * assuming RNG is good.  Smaller would suffice if it is just a
 * counter.
 */

struct NaClDescQuota {
  struct NaClDesc                base NACL_IS_REFCOUNT_SUBCLASS;
  struct NaClMutex               mu;
  struct NaClDesc                *desc;
  uint8_t                        file_id[NACL_DESC_QUOTA_FILE_ID_LEN];
  struct NaClDescQuotaInterface  *quota_interface;
};

/*
 * Takes ownership of desc.
 */
int NaClDescQuotaCtor(struct NaClDescQuota           *self,
                      struct NaClDesc                *desc,
                      uint8_t const                  *file_id,
                      struct NaClDescQuotaInterface  *quota_interface)
    NACL_WUR;

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_QUOTA_H_ */
