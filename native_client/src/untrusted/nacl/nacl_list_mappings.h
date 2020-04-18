/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _NATIVE_CLIENT_SRC_UNTRUSTED_NACL_NACL_LIST_MAPPINGS_H_
#define _NATIVE_CLIENT_SRC_UNTRUSTED_NACL_NACL_LIST_MAPPINGS_H_ 1

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct NaClMemMappingInfo;

/**
 *  @nacl
 *  Gets a memory map of the current process.
 *  @param regions Destination to receive info on memory regions.
 *  @param count Number of regions that there are space for.
 *  @param result_count Pointer to location to receive number of regions
 *  needed.
 *  @return Returns zero on success, -1 on failure.
 *  Sets errno to EFAULT if output locations are bad.
 *  Sets errno to ENOMEM if insufficent memory exists to gather the map.
 */
int nacl_list_mappings(struct NaClMemMappingInfo *info, size_t count,
                       size_t *result_count);

#ifdef __cplusplus
}
#endif

#endif  /* _NATIVE_CLIENT_SRC_UNTRUSTED_NACL_NACL_LIST_MAPPINGS_H_ */
