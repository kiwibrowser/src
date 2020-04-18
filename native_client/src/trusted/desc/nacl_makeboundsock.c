/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_global_secure_random.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_conn_cap.h"
#include "native_client/src/trusted/desc/nacl_desc_imc_bound_desc.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"


int32_t NaClCommonDescMakeBoundSock(struct NaClDesc *pair[2]) {
  int32_t                     retval;
  struct NaClSocketAddress    sa;
  struct NaClDescConnCap      *ccp;
  NaClHandle                  h;
  struct NaClDescImcBoundDesc *idp;

  retval = -NACL_ABI_ENOMEM;
  ccp = NULL;
  idp = NULL;
  h = NACL_INVALID_HANDLE;
  /*
   * create NaClDescConnCap object, invoke NaClBoundSocket, create
   * an NaClDescImcDesc object.  put both into open file table.
   */
  ccp = malloc(sizeof *ccp);
  if (NULL == ccp) {
    goto cleanup;
  }
  idp = malloc(sizeof *idp);
  if (NULL == idp) {
    goto cleanup;
  }

  do {
    NaClGenerateRandomPath(&sa.path[0], NACL_PATH_MAX);
    h = NaClBoundSocket(&sa);
    NaClLog(3, "NaClCommonDescMakeBoundSock: sa: %s, h 0x%"NACL_PRIxPTR"\n",
            sa.path, (uintptr_t) h);
  } while (NACL_INVALID_HANDLE == h);

  if (!NaClDescConnCapCtor(ccp, &sa)) {
    goto cleanup;
  }

  if (!NaClDescImcBoundDescCtor(idp, h)) {
    NaClDescUnref((struct NaClDesc *) ccp);
    goto cleanup;
  }
  h = NACL_INVALID_HANDLE;  /* idp took ownership */

  pair[0] = (struct NaClDesc *) idp;
  idp = NULL;

  pair[1] = (struct NaClDesc *) ccp;
  ccp = NULL;

  retval = 0;

 cleanup:
  free(idp);
  free(ccp);
  if (NACL_INVALID_HANDLE != h) {
    (void) NaClClose(h);
  }
  return retval;
}
