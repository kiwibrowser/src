/*
 * Copyright (c) 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  IMC API.
 */

#ifndef _NATIVE_CLIENT_SRC_PUBLIC_IMC_TYPES_H_
#define _NATIVE_CLIENT_SRC_PUBLIC_IMC_TYPES_H_

/*
 * This file defines the C API for NativeClient applications.  The
 * ABI is implicitly defined.
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/trusted/service_runtime/nacl_size_t.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TODO(sehr): there should be one instance to avoid conflicting definitions.
 */
#ifndef __nacl_handle_defined
#define __nacl_handle_defined
#if NACL_WINDOWS
typedef HANDLE NaClHandle;
#else
typedef int NaClHandle;
#endif
#endif

struct NaClAbiNaClImcMsgIoVec {
#ifdef __native_client__
  void            *base;
#else
  uint32_t        base;
#endif
  nacl_abi_size_t length;
};

struct NaClAbiNaClImcMsgHdr {
#ifdef __native_client__
  struct NaClAbiNaClImcMsgIoVec  *iov;
#else
  uint32_t                iov;
#endif
  nacl_abi_size_t         iov_length;
#ifdef __native_client__
  int                     *descv;
#else
  uint32_t                descv;
#endif
  nacl_abi_size_t         desc_length;
  int                     flags;
};

#ifndef __native_client__
struct NaClImcMsgIoVec {
  void    *base;
  size_t  length;
};

struct NaClImcMsgHdr {
  struct NaClImcMsgIoVec  *iov;
  nacl_abi_size_t         iov_length;
  int                     *descv;
  nacl_abi_size_t         desc_length;
  int                     flags;
};
#endif

/*
 * NACL_ABI_IMC_IOVEC_MAX: How many struct NaClIOVec are permitted?
 * These are copied to kernel space in order to translate/validate
 * addresses, and are on the thread stack when processing
 * NaClSysSendmsg and NaClSysRecvmsg syscalls.  Each object takes 8
 * bytes, so beware running into NACL_KERN_STACK_SIZE above.
 */
#define NACL_ABI_IMC_IOVEC_MAX      256

/*
 * NAC_ABI_IMC_DESC_MAX: How many descriptors are permitted?  Each
 * object is 4 bytes.  An array of ints are on the kernel stack.
 *
 * TODO(bsy): coordinate w/ NACL_HANDLE_COUNT_MAX in nacl_imc_c.h.
 * Current IMC-imposed limit seems way too small.
 */
#define NACL_ABI_IMC_USER_DESC_MAX  8
#define NACL_ABI_IMC_DESC_MAX       8

/*
 * NACL_ABI_IMC_USER_BYTES_MAX: read must go into a kernel buffer first
 * before a variable-length header describing the number and types of
 * NaClHandles encoded is parsed, with the rest of the data not yet
 * consumed turning into user data.
 */
#define NACL_ABI_IMC_USER_BYTES_MAX    (128 << 10)
#define NACL_ABI_IMC_BYTES_MAX                   \
  (NACL_ABI_IMC_USER_BYTES_MAX                   \
   + (1 + NACL_PATH_MAX) * NACL_ABI_IMC_USER_DESC_MAX + 16)
/*
 * 4096 + (1 + 28) * 256 = 11520, so the read buffer must be malloc'd
 * or be part of the NaClAppThread structure; the kernel thread stack
 * is too small for it.
 *
 * NB: the header has an end tag and the size is rounded up to the
 * next 16 bytes.
 */

/* these values must match src/shared/imc/nacl_imc{,_c}.h */
#define NACL_ABI_RECVMSG_DATA_TRUNCATED 0x1
#define NACL_ABI_RECVMSG_DESC_TRUNCATED 0x2

#define NACL_ABI_IMC_NONBLOCK           0x1

#ifdef __cplusplus
}
#endif

#endif
