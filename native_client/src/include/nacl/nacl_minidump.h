/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_INCLUDE_NACL_NACL_MINIDUMP_H_
#define NATIVE_CLIENT_SRC_INCLUDE_NACL_NACL_MINIDUMP_H_ 1

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This is the type of callback function to be registered with
 * nacl_minidump_set_callback().  Such a callback will be called when
 * a crash occurs.  The callback is passed the minidump crash dump
 * data, which the callback may choose to report by saving the data to
 * a file, sending it via IPC, etc.
 */
typedef void (*nacl_minidump_callback_t)(const void *minidump_data,
                                         size_t size);

/*
 * Initialize crash reporting by registering an exception handler.
 */
void nacl_minidump_register_crash_handler(void);

/*
 * Set the function to be called when a crash occurs.
 */
void nacl_minidump_set_callback(nacl_minidump_callback_t callback);

/*
 * nacl_minidump_set_module_name() allows setting the name of the nexe
 * to be reported in the minidump, since otherwise it is usually not
 * possible to determine this name at run time.  This is typically the
 * filename without the path, e.g. "foo.nexe".  Breakpad's
 * minidump_stackwalk tool uses this information to look up debugging
 * info when reading the minidump.
 *
 * Note that this function does not make a copy of module_name, so the
 * string must remain valid after the call to this function.
 */
void nacl_minidump_set_module_name(const char *module_name);

/*
 * Traversing the module set may fail in the midst of a crash. However,
 * capturing the module set ahead of time may yield an incomplete snapshot.
 * This function captures the module list to be used in place of a live
 * traversal during a crash.
 */
void nacl_minidump_snapshot_module_list(void);

/*
 * This function clears any current snapshot of the module list, switching to
 * live traversal of modules when a crash occurs.
 */
void nacl_minidump_clear_module_list(void);

#define NACL_MINIDUMP_BUILD_ID_SIZE 16

/*
 * nacl_minidump_set_module_build_id() allows setting the build ID of
 * the nexe to be reported in the minidump.
 */
void nacl_minidump_set_module_build_id(
    const uint8_t data[NACL_MINIDUMP_BUILD_ID_SIZE]);

#ifdef __cplusplus
}
#endif

#endif
