/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/sel_main_common.h"

#include "native_client/src/include/build_config.h"

#if NACL_OSX
#include <crt_externs.h>
#endif

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"

NaClErrorCode NaClMainLoadIrt(struct NaClApp *nap, struct NaClDesc *nd,
                              struct NaClValidationMetadata *metadata) {
  NaClErrorCode errcode;
  errcode = NaClAppLoadFileDynamically(nap, nd, metadata);
  if (errcode != LOAD_OK) {
    return errcode;
  }
  CHECK(NULL == nap->irt_nexe_desc);
  NaClDescRef(nd);
  nap->irt_nexe_desc = nd;
  return LOAD_OK;
}

const char ** NaClGetEnviron(void) {
  const char **envp;
#if NACL_OSX
  /* Mac dynamic libraries cannot access the environ variable directly. */
  envp = (const char **) *_NSGetEnviron();
#else
  /* Overzealous code style check is overzealous. */
  /* @IGNORE_LINES_FOR_CODE_HYGIENE[1] */
  extern char **environ;
  envp = (const char **) environ;
#endif
  return envp;
}
