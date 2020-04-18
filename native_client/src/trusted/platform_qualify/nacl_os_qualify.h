/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_NACL_OS_QUALIFY_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_NACL_OS_QUALIFY_H_

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * Returns 1 if the operating system can run Native Client modules.
 */
int NaClOsIsSupported(void);

/*
 * Returns 1 if the operating system is a 64-bit version of Windows.
 *
 * TODO(adonovan): This is currently the only platform on which we
 * deploy a 64-bit sel_ldr, and this function is currently used to
 * check that.  This is not ideal; we should have a higher-level
 * feature test interface that answers questions such as "what
 * sel_ldr(s) are available on this system" directly, instead of
 * strewing such assumptions throughout the code.
 */
int NaClOsIs64BitWindows(void);

#if NACL_OSX
/*
 * Returns 1 if the operating system is OS X 10.10+.
 */
int NaClOSX10Dot10OrLater(void);

/*
 * Returns 1 if the operating system is OS X 10.7+.
 */
int NaClOSX10Dot7OrLater(void);
#endif

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_NACL_OS_QUALIFY_H_ */
