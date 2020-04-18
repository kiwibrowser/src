/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Native Client Resource Descriptor Transfer Protocol for trusted code.
 *
 * The public API consists of a subset of the interface used
 * internally by the service runtime.  Use only the functions declared
 * in this header file, NaClDescUnref, and NaClDesc*{Ctor}.
 *
 * The intent is that trusted code have to act as a forwarding agent,
 * e.g., the browser plugin, must also implement the NRD transfer
 * protocol in order to pass objects between NaCl modules even if the
 * forwarding agent itself will not use the access rights itself.  By
 * permitting the forwarding agent to receive the data-only portion of
 * an IMC message and to receive the NRDs as separate opaque object
 * references, the forwarding agent is free to forward individual NRDs
 * embedded in an incoming message separately, hold on to them to send
 * later, etc.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NRD_XFER_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NRD_XFER_H_

/**
 * @addtogroup nrd_xfer NaCl Resource Descriptor Transfer
 * Contains functions used by trusted code to participate in the
 * NRD transfer protocol used by the NaCl service runtime.
 *
 * @{
 */

/* in lieu of sys/types for int32_t */
#include "native_client/src/include/portability.h"

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/public/imc_types.h"  /* NaClImcMsgIoVec */
#include "native_client/src/public/nacl_desc_custom.h"
#include "native_client/src/trusted/service_runtime/include/machine/_types.h"

EXTERN_C_BEGIN

struct NaClDescEffector;
struct NaClImcTypedMsgHdr;


/**
 * Send a high-level IMC message (containing typed descriptors) over
 * an IMC channel.  Returns the number of bytes sent on success, and
 * a negated errno value (essentially the kernel return ABI) on error.
 */
ssize_t NaClImcSendTypedMessage(struct NaClDesc                 *channel,
                                const struct NaClImcTypedMsgHdr *nitmhp,
                                int32_t                         flags);

/**
 * Receive a high-level IMC message (containing typed descriptors)
 * over an IMC channel.  Returns the number of bytes received on
 * success, and a negative value, a negated errno value, on error
 * (the kernel return ABI).
 */
ssize_t NaClImcRecvTypedMessage(struct NaClDesc               *channel,
                                struct NaClImcTypedMsgHdr     *nitmhp,
                                int32_t                       flags);

/**
 * Create a bound socket and corresponding socket address as a pair.
 * Returns 0 on success, and a negative value (negated errno) on
 * error.
 *
 * pair[0] is a NaClDescImcBoundDesc, and
 * pair[1] is a NaClDescConnCap.
 */
int32_t NaClCommonDescMakeBoundSock(struct NaClDesc   *pair[2]);

/**
 * Create a pair of connected sockets.
 * Returns 0 on success, and a negative value (negated errno) on
 * error.
 *
 * pair[0] is a NaClDescXferableDataDesc, and
 * pair[1] is a NaClDescXferableDataDesc.
 */
int32_t NaClCommonDescSocketPair(struct NaClDesc *pair[2]);

EXTERN_C_END

/*
 * @}
 * End of NaCl Resource Descriptor Transfer group
 */

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_DESC_NRD_XFER_H_ */
