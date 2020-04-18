/*
 * Copyright (c) 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This file contains functions responsible for copying filenames to and from
 * the user process. For restricted filesystem access (refer to
 * documentation/filesystem_access.txt for more details), this abstracts away
 * the details of files mounted at a root directory.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_LDR_FILENAME_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_LDR_FILENAME_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"

EXTERN_C_BEGIN

/*
 * Given a file path at |src| from the user, copy the path into a buffer |dest|.
 *
 * This function abstracts the complexity of using a "mounted filesystem" --
 * regardless whether sel_ldr is configured to use raw file system access or
 * file system access restricted to a root directory, this function correctly
 * handles the translation from the "raw" user path to the "real" absolute path
 * (which will be prefixed by a root directory, if necessary).
 *
 * @param[in] nap The NaCl application object.
 * @param[out] dest A buffer to contain the user's path argument.
 * @param[in] dest_max_size The size of the dest buffer.
 * @param[in] src A pointer to user's path buffer.
 * @return 0 on success, else a negated NaCl errno.
 */
uint32_t CopyHostPathInFromUser(struct NaClApp *nap, char *dest,
                                size_t dest_max_size, uint32_t src);

/*
 * Given an absolute file path at |path|, copy the path into a user allocated
 * buffer at |dst_user_addr|.
 *
 * Like "CopyHostPathInFromUser", this function abstracts away the "mounted
 * filesystem".
 *
 * @param[in] nap The NaCl application object.
 * @param[in] dst_usr_addr The destination of the user allocated buffer.
 * @param[in] path A buffer containing the absolute file path to be copied.
 * @return 0 on success, else a negated NaCl errno.
 */
uint32_t CopyHostPathOutToUser(struct NaClApp *nap, uint32_t dst_usr_addr,
                               char *path);

EXTERN_C_END

#endif /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_LDR_FILENAME_H_ */
