/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/portability.h"
#include "native_client/src/include/portability_io.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/public/chrome_main.h"
#include "native_client/src/shared/gio/gio.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_exit.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/trusted/service_runtime/nacl_all_modules.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"

static void NaClCrashLogWriter(const char *buf, size_t buf_bytes) {
  (void) fprintf(stdout, "NaClCrashLogWriter: log buffer contents:\n");
  /*
   * TODO(phosek): fwrite is defined with __wur in glibc < 2.15, eliminate
   * the ignore result macro once glibc >= 2.16 becomes more widespread.
   */
  IGNORE_RESULT(fwrite(buf, 1, buf_bytes, stdout));
  (void) fflush(stdout);
}

int main(void) {
  struct NaClApp state;
  int retval = 1;

  NaClChromeMainInit();
  NaClSetFatalErrorCallback(NaClCrashLogWriter);
  if (!NaClAppCtor(&state)) {
    fprintf(stderr, "FAILED: could not construct NaCl App state\n");
    goto done;
  }
  NaClLog(LOG_FATAL,
          "This is a test of the emergency log recovery mechanism."
          " This is only a test.  If this had been an actual emergency,"
          " you would have been instructed to surf to one of the"
          " official web sites for your planet.\n");
  retval = 2;
 done:
  return retval;
}
