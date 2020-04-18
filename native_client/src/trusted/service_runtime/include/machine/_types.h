/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl kernel / service run-time system call ABI.
 * This file defines nacl target machine dependent types.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_MACHINE__TYPES_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_MACHINE__TYPES_H_

#if defined(NACL_IN_TOOLCHAIN_HEADERS)
# include <machine/_default_types.h>
#else
# include "native_client/src/include/portability.h"
#endif

#define __need_size_t
#include <stddef.h>

#ifndef NULL
#define NULL 0
#endif

#ifndef nacl_abi___dev_t_defined
#define nacl_abi___dev_t_defined
typedef __int64_t nacl_abi___dev_t;
#if !defined(NACL_IN_TOOLCHAIN_HEADERS)
typedef nacl_abi___dev_t nacl_abi_dev_t;
#endif
#endif

#ifndef nacl_abi___ino_t_defined
#define nacl_abi___ino_t_defined
typedef __uint64_t nacl_abi___ino_t;
#if !defined(NACL_IN_TOOLCHAIN_HEADERS)
typedef nacl_abi___ino_t nacl_abi_ino_t;
#endif
#endif

#ifndef nacl_abi___mode_t_defined
#define nacl_abi___mode_t_defined
typedef __uint32_t nacl_abi___mode_t;
#if !defined(NACL_IN_TOOLCHAIN_HEADERS)
typedef nacl_abi___mode_t nacl_abi_mode_t;
#endif
#endif

#ifndef nacl_abi___nlink_t_defined
#define nacl_abi___nlink_t_defined
typedef __uint32_t nacl_abi___nlink_t;
#if !defined(NACL_IN_TOOLCHAIN_HEADERS)
typedef nacl_abi___nlink_t nacl_abi_nlink_t;
#endif
#endif

#ifndef nacl_abi___uid_t_defined
#define nacl_abi___uid_t_defined
typedef __uint32_t nacl_abi___uid_t;
#if !defined(NACL_IN_TOOLCHAIN_HEADERS)
typedef nacl_abi___uid_t nacl_abi_uid_t;
#endif
#endif

#ifndef nacl_abi___gid_t_defined
#define nacl_abi___gid_t_defined
typedef __uint32_t nacl_abi___gid_t;
#if !defined(NACL_IN_TOOLCHAIN_HEADERS)
typedef nacl_abi___gid_t nacl_abi_gid_t;
#endif
#endif

#ifndef nacl_abi___off_t_defined
#define nacl_abi___off_t_defined
typedef __int64_t nacl_abi__off_t;
#if !defined(NACL_IN_TOOLCHAIN_HEADERS)
typedef nacl_abi__off_t nacl_abi_off_t;
#endif
#endif

#ifndef nacl_abi___off64_t_defined
#define nacl_abi___off64_t_defined
typedef __int64_t nacl_abi__off64_t;
#if !defined(NACL_IN_TOOLCHAIN_HEADERS)
typedef nacl_abi__off64_t nacl_abi_off64_t;
#endif
#endif

#ifndef nacl_abi___blksize_t_defined
#define nacl_abi___blksize_t_defined
typedef __int32_t nacl_abi___blksize_t;
typedef nacl_abi___blksize_t nacl_abi_blksize_t;
#endif

#ifndef nacl_abi___blkcnt_t_defined
#define nacl_abi___blkcnt_t_defined
typedef __int32_t nacl_abi___blkcnt_t;
typedef nacl_abi___blkcnt_t nacl_abi_blkcnt_t;
#endif

#ifndef nacl_abi___time_t_defined
#define nacl_abi___time_t_defined
typedef __int64_t       nacl_abi___time_t;
typedef nacl_abi___time_t nacl_abi_time_t;
#endif

#ifndef nacl_abi___timespec_defined
#define nacl_abi___timespec_defined
struct nacl_abi_timespec {
  nacl_abi_time_t tv_sec;
#if defined(NACL_IN_TOOLCHAIN_HEADERS)
  long int        tv_nsec;
#else
  __int32_t         tv_nsec;
#endif
};
#endif

#endif
