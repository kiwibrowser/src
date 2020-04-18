/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_SIZE_T_H
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_SIZE_T_H

#include "native_client/src/include/portability.h"

#ifndef nacl_abi_size_t_defined
#define nacl_abi_size_t_defined
typedef uint32_t nacl_abi_size_t;
#endif

#define NACL_ABI_SIZE_T_MIN ((nacl_abi_size_t) 0)
#define NACL_ABI_SIZE_T_MAX ((nacl_abi_size_t) -1)

#ifndef nacl_abi_ssize_t_defined
#define nacl_abi_ssize_t_defined
typedef int32_t nacl_abi_ssize_t;
#endif

#define NACL_ABI_SSIZE_T_MAX \
  ((nacl_abi_ssize_t) (NACL_ABI_SIZE_T_MAX >> 1))
#define NACL_ABI_SSIZE_T_MIN \
  (~NACL_ABI_SSIZE_T_MAX)

#define NACL_PRIdNACL_SIZE NACL_PRId32
#define NACL_PRIiNACL_SIZE NACL_PRIi32
#define NACL_PRIoNACL_SIZE NACL_PRIo32
#define NACL_PRIuNACL_SIZE NACL_PRIu32
#define NACL_PRIxNACL_SIZE NACL_PRIx32
#define NACL_PRIXNACL_SIZE NACL_PRIX32

/**
 * Inline functions to aid in conversion between system (s)size_t and
 * nacl_abi_(s)size_t
 *
 * These are defined as inline functions only if __native_client__ is
 * undefined, since in a nacl module size_t and nacl_abi_size_t are always
 * the same (and we don't have a definition for INLINE, so these won't compile)
 *
 * If __native_client__ *is* defined, these turn into no-ops.
 */
#ifdef __native_client__
  /**
   * NB: The "no-op" version of these functions does NO type conversion.
   * Please DO NOT CHANGE THIS. If you get a type error using these functions
   * in a NaCl module, it's a real error and should be fixed in your code.
   */
  #define nacl_abi_size_t_saturate(x) (x)
  #define nacl_abi_ssize_t_saturate(x) (x)
#else /* __native_client */
  static INLINE nacl_abi_size_t nacl_abi_size_t_saturate(size_t x) {
    if (x > NACL_ABI_SIZE_T_MAX) {
      return NACL_ABI_SIZE_T_MAX;
    } else {
      return (nacl_abi_size_t)x;
    }
  }

  static INLINE nacl_abi_ssize_t nacl_abi_ssize_t_saturate(ssize_t x) {
    if (x > NACL_ABI_SSIZE_T_MAX) {
      return NACL_ABI_SSIZE_T_MAX;
    } else if (x < NACL_ABI_SSIZE_T_MIN) {
      return NACL_ABI_SSIZE_T_MIN;
    } else {
      return (nacl_abi_ssize_t) x;
    }
  }
#endif /* __native_client */

#endif
