/*
 * Copyright (c) 2011 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_UNTRUSTED_IRT_STUB_PPAPI_START_H_
#define NATIVE_CLIENT_SRC_UNTRUSTED_IRT_STUB_PPAPI_START_H_

#include "ppapi/nacl_irt/public/irt_ppapi.h"

/*
 * Start PPAPI using the given set of callbacks to invoke the application code.
 * This returns nonzero on setup failure.  On success, it returns only when
 * the PPAPI event loop has terminated, then returning zero.
 */
int PpapiPluginStart(const struct PP_StartFunctions *funcs);

#endif
