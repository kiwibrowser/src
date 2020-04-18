/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __NACL_MIPS_DEFS_H__
#define __NACL_MIPS_DEFS_H__

/*
 * Values NACL_BLOCK_SHIFT and NACL_BLOCK_SIZE are copied from:
 * native_client/src/trusted/service_runtime/nacl_config.h
 */

#define NACL_BLOCK_SHIFT   4
#define NACL_BLOCK_SIZE    16
#define JUMP_MASK          $t6
#define STORE_MASK         $t7

#endif
