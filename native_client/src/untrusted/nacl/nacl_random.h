/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _NATIVE_CLIENT_SRC_UNTRUSTED_NACL_NACL_RANDOM_H_
#define _NATIVE_CLIENT_SRC_UNTRUSTED_NACL_NACL_RANDOM_H_ 1

#include <stddef.h>  /* size_t */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * nacl_secure_random_init() is deprecated.  Calling it before calling
 * nacl_secure_random() is no longer required.  It always returns 0,
 * indicating success.
 */
extern int nacl_secure_random_init(void);

/**
 *  @nacl
 *  Obtain secure random bytes (note: non-blocking).
 *  @param dest Destination address.
 *  @param bytes Number of bytes to write to dest.
 *  @param bytes_written Pointer to a size_t which will hold the number of
 *         bytes written to dest.
 *  @return Returns zero on success, or a non-zero error number on failure.
 */
extern int nacl_secure_random(void *dest, size_t bytes, size_t *bytes_written);

#ifdef __cplusplus
}
#endif

#endif  /* _NATIVE_CLIENT_SRC_UNTRUSTED_NACL_NACL_RANDOM_H_ */
