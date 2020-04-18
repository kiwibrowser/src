/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime logging code.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_LOG_INTERN_H__
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_LOG_INTERN_H__

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * The global variable gNaClLogAbortBehavior should only be modified
 * by test code after NaClLogModuleInit() has been called.
 *
 * This variable is needed to make the test infrastructure simpler: on
 * Windows, abort(3) causes the UI to pop up a window
 * (Retry/Abort/Debug/etc), and while for CHECK macros (see
 * nacl_check.h) we probably do normally want that kind of intrusive,
 * in-your-face error reporting, running death tests on our testing
 * infrastructure in a continuous build, continuous test environment
 * cannot tolerate requiring any human interaction.  And since NaCl
 * developers elsewhere will want to be able to run tests, including
 * regedit kludgery to temporarily disable the popup is not a good
 * idea -- even when scoped to the test application by name (need to
 * do this for every death test), it should be feasible to have
 * multiple copies of the svn source tree, and to run at least the
 * small tests in those tree in parallel.
 */
extern void (*gNaClLogAbortBehavior)(void);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_LOG_INTERN_H__ */
