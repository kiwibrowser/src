/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_PUBLIC_WIN_DEBUG_EXCEPTION_HANDLER_H_
#define NATIVE_CLIENT_SRC_PUBLIC_WIN_DEBUG_EXCEPTION_HANDLER_H_

#include <stdlib.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"

#if NACL_WINDOWS

#include <windows.h>

EXTERN_C_BEGIN

/*
 * This runs the debug exception handler in the current thread.  The
 * current thread should already have attached to the target process
 * through the Windows debug API by calling DebugActiveProcess() or by
 * calling CreateProcess() with DEBUG_PROCESS.
 *
 * In info/info_size this function expects to receive the array of
 * bytes that was passed to the callback
 * attach_debug_exception_handler_func() that was registered via
 * NaClChromeMainArgs (or passed to any other
 * NaClAttachDebugExceptionHandlerFunc callback called internally by
 * NaClDebugExceptionHandlerEnsureAttached()).
 */
void NaClDebugExceptionHandlerRun(HANDLE process_handle,
                                  const void *info, size_t info_size);

EXTERN_C_END

#endif

#endif
