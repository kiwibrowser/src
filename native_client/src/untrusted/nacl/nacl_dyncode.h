/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _NATIVE_CLIENT_SRC_UNTRUSTED_NACL_NACL_DYNCODE_H_
#define _NATIVE_CLIENT_SRC_UNTRUSTED_NACL_NACL_DYNCODE_H_ 1

#include <stddef.h>  /* size_t */

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @nacl
 *  Validates and dynamically loads executable code into an unused address.
 *  @param dest Destination address.  Must be in the code region and
 *  be instruction-bundle-aligned.
 *  @param src Source address.  Does not need to be aligned.
 *  @param size This must be a multiple of the bundle size.
 *  @return Returns zero on success, -1 on failure.
 *  Sets errno to EINVAL if validation fails, if src or size are not
 *  properly aligned, or the destination is invalid or has already had
 *  code loaded into it.
 */
extern int nacl_dyncode_create(void *dest, const void *src, size_t size);

/**
 *  @nacl
 *  Validates and modifies previously loaded dynamic code.  Must
 *  have identical instruction boundaries to existing code.
 *  @param dest Destination address. Must be subregion of one previously created
 *  @param src Source address.
 *  @param size of both dest and src, need not be aligned.
 *  @return Returns zero on success, -1 on failure.
 *  Sets errno to EINVAL if validation fails, if src or size are not
 *  properly aligned, or the destination is not previously created
 *  dyncode region or instruction boundaries changed.
 */
extern int nacl_dyncode_modify(void *dest, const void *src, size_t size);

/**
 *  @nacl
 *  Remove inserted dynamic code or mark it for deletion if threads are
 *  unaccounted for.
 *  @param dest must match a past call to nacl_dyncode_create
 *  @param size must match a past call to nacl_dyncode_create
 *  @return Returns zero on success, -1 on failure.
 *  Fails and sets errno to EAGAIN if deletion is delayed because other
 *  threads have not checked into the nacl runtime.
 */
extern int nacl_dyncode_delete(void *dest, size_t size);

#ifdef __cplusplus
}
#endif

#endif  /* _NATIVE_CLIENT_SRC_UNTRUSTED_NACL_NACL_DYNCODE_H_ */
