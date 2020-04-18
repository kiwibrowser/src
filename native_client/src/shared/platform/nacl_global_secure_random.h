/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Secure RNG.  Global singleton.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_GLOBAL_SECURE_RANDOM_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_GLOBAL_SECURE_RANDOM_H_

#include "native_client/src/shared/platform/nacl_secure_random.h"
#include "native_client/src/shared/platform/nacl_sync.h"

void NaClGlobalSecureRngInit(void);

void NaClGlobalSecureRngSwitchRngForTesting(struct NaClSecureRng *);

void NaClGlobalSecureRngFini(void);

int32_t NaClGlobalSecureRngUniform(int32_t range_max);

uint32_t NaClGlobalSecureRngUint32(void);

void NaClGlobalSecureRngGenerateBytes(uint8_t *buf, size_t buf_size);

/*
 * Generate a random alpha-numeric name for a socket, a semaphore or some
 * other device.
 */
void NaClGenerateRandomPath(char *path, int length);

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_GLOBAL_SECURE_RANDOM_H_ */
