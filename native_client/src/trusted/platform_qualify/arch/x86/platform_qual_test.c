/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * platform_qual_test.c
 *
 * Native Client Platform Qualification Test
 *
 * This uses shell status code to indicate its result; non-zero return
 * code indicates the CPUID instruction is not implemented or not
 * implemented correctly.
 */

#include <stdio.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/cpu_features/arch/x86/cpu_x86.h"
#include "native_client/src/trusted/platform_qualify/nacl_cpuwhitelist.h"
#include "native_client/src/trusted/platform_qualify/nacl_os_qualify.h"
#include "native_client/src/trusted/platform_qualify/nacl_dep_qualify.h"
#include "native_client/src/trusted/platform_qualify/arch/x86/vcpuid.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"

void TestDEPCheckFailurePath(void) {
  size_t size = NACL_PAGESIZE;
  void *page;
  CHECK(NaClPageAlloc(&page, size) == 0);

  CHECK(NaClMprotect(page, size, PROT_READ | PROT_WRITE | PROT_EXEC) == 0);
  CHECK(!NaClAttemptToExecuteDataAtAddr(page, size));

  /* DEP is not guaranteed to work on x86-32. */
  if (!(NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32)) {
    CHECK(NaClMprotect(page, size, PROT_READ | PROT_WRITE) == 0);
    CHECK(NaClAttemptToExecuteDataAtAddr(page, size));
  }

  NaClPageFree(page, size);
}

int main(void) {
  NaClLogModuleInit();

  if (NaClOsIsSupported() != 1) return -1;
  printf("OS is supported\n");

  if (NaCl_ThisCPUIsBlacklisted()) return -1;
  printf("CPU is not blacklisted\n");

  TestDEPCheckFailurePath();

  if (NaClCheckDEP() != 1) return -1;
  printf("DEP is either working or not required\n");

  if (!CPUIDImplIsValid()) return -1;
  printf("CPUID implementation looks okay\n");

  /*
   * don't use the white list for now
   * if (NaCl_CPUIsWhitelisted() == 0) return -1;
   * printf("CPU is whitelisted\n");
   */

  printf("platform_qual_test: PASS\n");
  return 0;
}
