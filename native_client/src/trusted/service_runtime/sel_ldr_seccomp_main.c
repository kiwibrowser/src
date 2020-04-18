/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/seccomp_bpf/seccomp_bpf.h"
#include "native_client/src/trusted/service_runtime/sel_main.h"

static void EnableSeccompBpfSandbox(void) {
  if (0 != NaClInstallBpfFilter()) {
    NaClLog(LOG_FATAL, "Seccomp-bpf filter failed to apply. "
            "Looks like the kernel does not support it yet.\n");
  }
}

int main(int argc, char **argv) {
  NaClSetEnableOuterSandboxFunc(EnableSeccompBpfSandbox);
  return NaClSelLdrMain(argc, argv);
}
