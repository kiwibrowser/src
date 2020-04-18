/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime API.
 */


#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_BITS_WORDSIZE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_BITS_WORDSIZE_H_

#if !defined(__WORDSIZE)
# if defined(__powerpc64__) || defined(__s390x__) || defined(__arch64__) || \
     defined(__sparcv9) || defined(__x86_64__) || defined(_AMD64_) || \
     defined(_M_AMD64)
#  define __WORDSIZE 64
# else
#  define __WORDSIZE 32
# endif
#endif

#endif /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_BITS_WORDSIZE_H_ */
