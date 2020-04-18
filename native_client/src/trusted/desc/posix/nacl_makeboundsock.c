/*
 * Copyright (c) 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_conn_cap.h"
#include "native_client/src/trusted/desc/nacl_desc_imc_bound_desc.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"


int32_t NaClCommonDescMakeBoundSock(struct NaClDesc *pair[2]) {
  struct NaClDescConnCapFd *conn_cap = NULL;
  struct NaClDescImcBoundDesc *bound_sock = NULL;
  NaClHandle fd_pair[2];

  if (NaClSocketPair(fd_pair) != 0) {
    return -NACL_ABI_EMFILE;
  }

  conn_cap = malloc(sizeof(*conn_cap));
  if (NULL == conn_cap) {
    NaClLog(LOG_FATAL, "NaClCommonDescMakeBoundSock: allocation failed\n");
  }
  if (!NaClDescConnCapFdCtor(conn_cap, fd_pair[0])) {
    NaClLog(LOG_FATAL,
            "NaClCommonDescMakeBoundSock: NaClDescConnCapFdCtor failed\n");
  }

  bound_sock = malloc(sizeof(*bound_sock));
  if (NULL == bound_sock) {
    NaClLog(LOG_FATAL, "NaClCommonDescMakeBoundSock: allocation failed\n");
  }
  if (!NaClDescImcBoundDescCtor(bound_sock, fd_pair[1])) {
    NaClLog(LOG_FATAL,
            "NaClCommonDescMakeBoundSock: NaClDescImcBoundDescCtor failed\n");
  }

  pair[0] = &bound_sock->base;
  pair[1] = &conn_cap->base;
  return 0;
}
