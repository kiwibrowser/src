/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_UNTRUSTED_NACL_GETCWD_H_
#define NATIVE_CLIENT_SRC_UNTRUSTED_NACL_GETCWD_H_

#if defined(__cplusplus)
extern "C" {
#endif

char *__getcwd_without_malloc(char *buf, size_t size);

#if defined(__cplusplus)
}
#endif

#endif  /* NATIVE_CLIENT_SRC_UNTRUSTED_NACL_GETCWD_H_ */
