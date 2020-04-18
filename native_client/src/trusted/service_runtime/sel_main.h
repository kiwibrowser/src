/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_MAIN_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_MAIN_H_ 1

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * This allows setting a callback function for enabling an outer
 * sandbox in standalone sel_ldr.  This allows an embedder of NaCl to
 * provide its own outer sandbox implementation.  The callback will be
 * called by NaClSelLdrMain() after opening files (since the outer
 * sandbox is assumed to disable filesystem access), but before
 * running any untrusted code.
 */
void NaClSetEnableOuterSandboxFunc(void (*func)(void));

/*
 * This is the main entry point for running sel_ldr.  We provide this
 * entry point so that embedders of NaCl or NaCl test cases may link
 * custom versions of sel_ldr which perform some initialization prior
 * to calling NaClSelLdrMain().
 */
int NaClSelLdrMain(int argc, char **argv);

EXTERN_C_END

#endif
