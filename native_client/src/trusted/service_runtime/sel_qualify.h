/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_QUALIFY_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_QUALIFY_H_

/*
 * We have several different ways of starting Native Client, but all of them
 * need to run minimal checks during sel_ldr startup to ensure that the
 * environment can't subvert our security.  This header provides a high-level
 * interface to the Platform Qualification tests for this very purpose.
 */

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/trusted/service_runtime/nacl_error_code.h"

EXTERN_C_BEGIN

/*
 * Runs the Platform Qualification tests required for safe sel startup.  This
 * may be a subset of the full set of PQ tests: it includes the tests that are
 * important enough to check at every startup, and tests that check aspects of
 * the system that may be subject to change.
 */
NaClErrorCode NaClRunSelQualificationTests(void);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_QUALIFY_H_ */
