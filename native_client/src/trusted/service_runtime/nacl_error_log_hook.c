/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/platform/nacl_exit.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/nacl_error_log_hook.h"
#include "native_client/src/trusted/service_runtime/nacl_error_gio.h"

struct NaClApp;

static struct NaClErrorGio g_NaCl_log_gio;
static void (*g_NaCl_log_abort_fn)(void *state,
                                   char *buf, size_t buf_bytes) = NULL;
static void *g_NaCl_log_abort_state = NULL;

static void NaClReportLogMessages(void) {
  char log_data[NACL_ERROR_GIO_MAX_BYTES];
  size_t log_data_bytes;

  if (NULL != g_NaCl_log_abort_fn) {
    /*
     * Copy the last NACL_ERROR_GIO_MAX_BYTES of log output to the
     * calling thread's stack, so that breakpad will pick it up.
     */
    log_data_bytes = NaClErrorGioGetOutput(&g_NaCl_log_gio,
                                           log_data,
                                           NACL_ARRAY_SIZE(log_data));
    (*g_NaCl_log_abort_fn)(g_NaCl_log_abort_state,
                           log_data, log_data_bytes);
  }
}

void NaClErrorLogHookInit(void (*hook)(void *state,
                                       char *buf,
                                       size_t buf_bytes),
                          void *state) {
  NaClLog(2, "NaClErrorLogHookInit: entered\n");
  if (!NaClErrorGioCtor(&g_NaCl_log_gio, NaClLogGetGio())) {
    fprintf(stderr, "sel_main_chrome: log reporting setup failed\n");
    NaClAbort();
  }

  g_NaCl_log_abort_fn = hook;
  g_NaCl_log_abort_state = state;

  NaClLogSetGio((struct Gio *) &g_NaCl_log_gio);

  NaClLogSetAbortBehavior(NaClReportLogMessages);
}
