/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime API.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_BITS_MMAN_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_BITS_MMAN_H_

#define NACL_ABI_PROT_READ        0x1   /* Page can be read.  */
#define NACL_ABI_PROT_WRITE       0x2   /* Page can be written.  */
#define NACL_ABI_PROT_EXEC        0x4   /* Page can be executed.  */
#define NACL_ABI_PROT_NONE        0x0   /* Page can not be accessed.  */

#define NACL_ABI_PROT_MASK        0x7

#define NACL_ABI_MAP_SHARED       0x01  /* Share changes.  */
#define NACL_ABI_MAP_PRIVATE      0x02  /* Changes are private.  */

#define NACL_ABI_MAP_SHARING_MASK 0x03

#define NACL_ABI_MAP_FIXED        0x10  /* Interpret addr exactly.  */
#define NACL_ABI_MAP_ANON         0x20  /* Don't use a file.  */
#define NACL_ABI_MAP_ANONYMOUS    NACL_ABI_MAP_ANON  /* Linux alias.  */

#define NACL_ABI_MAP_FAILED       ((void *) -1)

#endif /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_BITS_MMAN_H_ */
