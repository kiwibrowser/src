/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_NACL_CPUWHITELIST
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_NACL_CPUWHITELIST

#include "native_client/src/include/nacl_base.h"

/* NOTES:
 * The blacklist/whitelist go in an array which must be kept SORTED
 * as it is passed to bsearch.
 * An x86 CPU ID string must be 20 bytes plus a '\0'.
 */
#define NACL_BLACKLIST_TEST_ENTRY "NaClBlacklistTest123"

EXTERN_C_BEGIN

/* Return 1 if CPU is whitelisted */
int NaCl_ThisCPUIsWhitelisted(void);
/* Return 1 if CPU is blacklisted */
int NaCl_ThisCPUIsBlacklisted(void);

/* Return 1 if list is well-structured. */
int NaCl_VerifyBlacklist(void);
int NaCl_VerifyWhitelist(void);

/* Return 1 if named CPU is whitelisted */
int NaCl_CPUIsWhitelisted(const char *myid);
/* Return 1 if named CPU is blacklisted */
int NaCl_CPUIsBlacklisted(const char *myid);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_NACL_CPUWHITELIST */
