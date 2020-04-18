/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This test program creates two NaCl sandboxes within the same host process.
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_exit.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/service_runtime/nacl_all_modules.h"
#include "native_client/src/trusted/service_runtime/nacl_app.h"
#include "native_client/src/trusted/service_runtime/nacl_valgrind_hooks.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"


#define SEND_DESC 3
#define RECEIVE_DESC 3


int main(int argc, char **argv) {
  struct NaClApp app[2];
  struct NaClDesc *nd;
  NaClHandle handle_pair[2];
  int i;
  char *domain1_args[] = {"prog", "domain1"};
  char *domain2_args[] = {"prog", "domain2"};
  int return_code;

  if (argc != 2)
    NaClLog(LOG_FATAL, "Expected 1 argument: executable filename\n");

  NaClAllModulesInit();

  NaClFileNameForValgrind(argv[1]);
  nd = (struct NaClDesc *) NaClDescIoDescOpen(argv[1], NACL_ABI_O_RDONLY, 0);
  CHECK(NULL != nd);

  for (i = 0; i < 2; i++) {
    CHECK(NaClAppCtor(&app[i]));

    /* Use a smaller guest address space size, because 32-bit Windows
       does not let us allocate 2GB of address space.  We don't do this
       for x86-64 because there is an assertion in NaClAllocateSpace()
       that requires 4GB. */
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
    app[i].addr_bits = 29; /* 512MB per process */
#endif

    /*
     * On x86-32, we cannot enable ASLR even when the address space is
     * 512MB.  In particular, when on Windows where the available
     * contiguous address space is tight, if the random choice for the
     * base of the first 512MB address space puts that address space
     * in the middle of the available address space region, the
     * address space allocation code might not be able to (randomly)
     * find another contiguous 512MB region for the second NaCl
     * module.
     */
    NaClXMutexLock(&app[i].mu);
    CHECK(NaClAppLoadFileAslr(nd, &app[i],
                              NACL_DISABLE_ASLR) == LOAD_OK);
    NaClXMutexUnlock(&app[i].mu);
    NaClAppInitialDescriptorHookup(&app[i]);
    CHECK(NaClAppPrepareToLaunch(&app[i]) == LOAD_OK);
  }

  /* Set up an IMC connection between the two guests.  This allows us
     to test communications between the two and also synchronise the
     output for the purpose of checking against the golden file. */
  CHECK(NaClSocketPair(handle_pair) == 0);
  NaClAddImcHandle(&app[0], handle_pair[0], SEND_DESC);
  NaClAddImcHandle(&app[1], handle_pair[1], RECEIVE_DESC);

  CHECK(NaClCreateMainThread(&app[0], 2, domain1_args, NULL));
  CHECK(NaClCreateMainThread(&app[1], 2, domain2_args, NULL));

  return_code = NaClWaitForMainThreadToExit(&app[0]);
  CHECK(return_code == 101);
  return_code = NaClWaitForMainThreadToExit(&app[1]);
  CHECK(return_code == 102);

  /*
   * Avoid calling exit() because it runs process-global destructors
   * which might break code that is running in our unjoined threads.
   */
  NaClExit(0);
}
