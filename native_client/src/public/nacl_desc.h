/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_PUBLIC_NACL_DESC_H_
#define NATIVE_CLIENT_SRC_PUBLIC_NACL_DESC_H_ 1

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/public/imc_types.h"

#if NACL_OSX
#include <mach/mach.h>
#endif

EXTERN_C_BEGIN

struct NaClDesc;

/*
 * Increments the reference count of |desc|.  Returns |desc|.
 */
struct NaClDesc *NaClDescRef(struct NaClDesc *desc);

/*
 * Decrements the reference count of |desc|.  If the reference count
 * reaches zero, this will call |desc|'s destructor and free |desc|.
 */
void NaClDescUnref(struct NaClDesc *desc);

/*
 * Create a NaClDesc for a NaClHandle which has reliable identity information.
 * That identity can be used for future validation caching.
 *
 * If the file_path string is empty, this returns a NaClDesc that is not marked
 * as validation-cacheable.
 *
 * On success, returns a new read-only NaClDesc that uses the passed handle,
 * setting file path information internally.
 * On failure, returns NULL.
 */
struct NaClDesc *NaClDescCreateWithFilePathMetadata(NaClHandle handle,
                                                    const char *file_path);

/*
 * NaClDescIoMakeFromHandle() takes ownership of the |handle| argument
 * -- when the returned NaClDesc object is destroyed (when the
 * refcount goes to 0), the handle will be closed.  The |flags|
 * argument should be one of NACL_ABI_O_RDONLY, NACL_ABI_O_RDWR,
 * NACL_ABI_O_WRONLY possibly bitwise ORed with NACL_ABI_O_APPEND.
 */
struct NaClDesc *NaClDescIoMakeFromHandle(NaClHandle handle,
                                          int flags) NACL_WUR;

struct NaClDesc *NaClDescImcShmMake(NaClHandle handle, int64_t size) NACL_WUR;
#if NACL_OSX
struct NaClDesc *NaClDescImcShmMachMake(mach_port_t handle,
                                        int64_t size) NACL_WUR;
#endif

struct NaClDesc *NaClDescSyncSocketMake(NaClHandle handle) NACL_WUR;

EXTERN_C_END

#endif
