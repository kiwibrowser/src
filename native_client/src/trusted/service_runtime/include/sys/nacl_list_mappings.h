/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Memory Region
 */

#ifndef _NATIVE_CLIENT_SRC_SERVICE_RUNTIME_INCLUDE_SYS_NACL_LIST_MAPPINGS_H_
#define _NATIVE_CLIENT_SRC_SERVICE_RUNTIME_INCLUDE_SYS_NACL_LIST_MAPPINGS_H_ 1

#if defined(NACL_IN_TOOLCHAIN_HEADERS)
# include <stdint.h>
#else
# include "native_client/src/include/portability.h"
#endif

struct NaClMemMappingInfo {
  uint32_t start;
  uint32_t size;
  uint32_t prot;
  uint32_t max_prot;
  uint32_t vmmap_type;
};

#endif /* _NATIVE_CLIENT_SRC_SERVICE_RUNTIME_INCLUDE_SYS_NACL_LIST_MAPPINGS_H_ */
