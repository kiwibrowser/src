/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/load_file.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/service_runtime/nacl_valgrind_hooks.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"


NaClErrorCode NaClAppLoadFileFromFilename(struct NaClApp *nap,
                                          const char *filename) {
  struct NaClDesc *nd;
  NaClErrorCode err;

  NaClFileNameForValgrind(filename);

  nd = (struct NaClDesc *) NaClDescIoDescOpen(filename, NACL_ABI_O_RDONLY,
                                              0666);
  if (NULL == nd) {
    return LOAD_OPEN_ERROR;
  }

  NaClAppLoadModule(nap, nd);
  err = NaClGetLoadStatus(nap);
  NaClDescUnref(nd);
  nd = NULL;

  return err;
}
