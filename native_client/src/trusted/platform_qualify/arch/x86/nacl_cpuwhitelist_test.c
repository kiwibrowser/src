/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * nacl_cpuwhitelist_test.c
 */
#include "native_client/src/include/portability.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "native_client/src/trusted/cpu_features/arch/x86/cpu_x86.h"
#include "native_client/src/trusted/platform_qualify/nacl_cpuwhitelist.h"

static void CPUIDWhitelistUnitTests(void) {
  /* blacklist tests */
  if (!NaCl_VerifyBlacklist()) {
    fprintf(stderr, "ERROR: blacklist malformed\n");
    exit(-1);
  }
  if (!NaCl_CPUIsBlacklisted(NACL_BLACKLIST_TEST_ENTRY)) {
    fprintf(stderr, "ERROR: blacklist test 1 failed\n");
    exit(-1);
  }
  if (NaCl_CPUIsBlacklisted("GenuineFooFooCPU")) {
    fprintf(stderr, "ERROR: blacklist test 2 failed\n");
    exit(-1);
  }
  printf("All blacklist unit tests passed\n");
  /* whitelist tests */
  /* NOTE: whitelist is not currently used */
  if (!NaCl_VerifyWhitelist()) {
    fprintf(stderr, "ERROR: whitelist malformed\n");
    exit(-1);
  }
  if (!NaCl_CPUIsWhitelisted(" FakeEntry0000000000")) {
    fprintf(stderr, "ERROR: whitelist search 1 failed\n");
    exit(-1);
  }
  if (!NaCl_CPUIsWhitelisted("GenuineIntel00000f43")) {
    fprintf(stderr, "ERROR: whitelist search 2 failed\n");
    exit(-1);
  }
  if (!NaCl_CPUIsWhitelisted("zFakeEntry0000000000")) {
    fprintf(stderr, "ERROR: whitelist search 3 failed\n");
    exit(-1);
  }
  if (NaCl_CPUIsWhitelisted("a")) {
    fprintf(stderr, "ERROR: whitelist search 4 didn't fail\n");
    exit(-1);
  }
  if (NaCl_CPUIsWhitelisted("")) {
    fprintf(stderr, "ERROR: whitelist search 5 didn't fail\n");
    exit(-1);
  }
  if (NaCl_CPUIsWhitelisted("zFakeEntry0000000001")) {
    fprintf(stderr, "ERROR: whitelist search 6 didn't fail\n");
    exit(-1);
  }
  if (NaCl_CPUIsWhitelisted("zFakeEntry00000000000")) {
    fprintf(stderr, "ERROR: whitelist search 7 didn't fail\n");
    exit(-1);
  }
  printf("All whitelist unit tests passed\n");
}

int main(void) {
  NaClCPUData data;
  NaClCPUDataGet(&data);
  printf("white list ID: %s\n", GetCPUIDString(&data));
  if (NaCl_ThisCPUIsWhitelisted()) {
    printf("this CPU is on the whitelist\n");
  } else {
    printf("this CPU is NOT on the whitelist\n");
  }
  if (NaCl_ThisCPUIsBlacklisted()) {
    printf("this CPU is on the blacklist\n");
  } else {
    printf("this CPU is NOT on the blacklist\n");
  }

  CPUIDWhitelistUnitTests();
  return 0;
}
