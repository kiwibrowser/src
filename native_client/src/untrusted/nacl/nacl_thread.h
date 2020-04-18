/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_UNTRUSTED_NACL_NACL_THREAD_H_
#define NATIVE_CLIENT_SRC_UNTRUSTED_NACL_NACL_THREAD_H_ 1

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void *nacl_tls_get(void);
extern int nacl_tls_init(void *thread_ptr);

#ifdef __cplusplus
}
#endif

#endif  /* NATIVE_CLIENT_SRC_UNTRUSTED_NACL_NACL_THREAD_H_ */
