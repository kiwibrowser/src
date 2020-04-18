/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Windows error code to errno translation.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_XLATE_SYSTEM_ERROR_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_XLATE_SYSTEM_ERROR_H_

/*
 * Translates (some of) windows error codes to errno codes.
 */
int NaClXlateSystemError(int sys_error_code);

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_XLATE_SYSTEM_ERROR_H_ */
