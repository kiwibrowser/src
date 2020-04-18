/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Secure RNG abstraction.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_SECURE_RANDOM_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_SECURE_RANDOM_H_

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"

/*
 * Get struct NaClSecureRng, since users will need its size for
 * placement-new style initialization.
 * TODO(bsy): is there another way to do this?
 */
#if NACL_LINUX || NACL_OSX
# include "native_client/src/shared/platform/posix/nacl_secure_random_types.h"
#elif NACL_WINDOWS
# include "native_client/src/shared/platform/win/nacl_secure_random_types.h"
#endif

EXTERN_C_BEGIN

struct NaClSecureRngIf;  /* fwd */

#if NACL_LINUX || NACL_OSX
void NaClSecureRngModuleSetUrandomFd(int fd);
#endif

void NaClSecureRngModuleInit(void);

void NaClSecureRngModuleFini(void);

int NaClSecureRngCtor(struct NaClSecureRng *self);

/*
 * This interface is for TESTING ONLY.  Having user-provided seed
 * material can be dangerous, since the available entropy in the seed
 * material is unknown.  The interface does not define the desired
 * seed material size -- in part, to discourage non-testing uses.
 *
 * Alternate subclasses may also be used for testing; this interface
 * exercises the actual RNG code, albeit with a possibly poor/fixed
 * key.
 */
int NaClSecureRngTestingCtor(struct NaClSecureRng *self,
                             uint8_t              *seed_material,
                             size_t               seed_bytes);

/*
 * Default implementations for subclasses.  Generally speakly,
 * probably shouldn't call directly -- just use the vtbl.
 */
uint32_t NaClSecureRngDefaultGenUint32(struct NaClSecureRngIf *self);

void NaClSecureRngDefaultGenBytes(struct NaClSecureRngIf  *self,
                                  uint8_t                 *buf,
                                  size_t                  nbytes);

uint32_t NaClSecureRngDefaultUniform(struct NaClSecureRngIf *self,
                                     uint32_t               range_max);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_SECURE_RANDOM_H_ */
