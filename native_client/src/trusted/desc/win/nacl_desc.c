/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.  Memory
 * mapping using descriptors.
 */

#include "native_client/src/include/portability.h"
#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_sync_socket.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"

int32_t NaClAbiStatHostDescStatXlateCtor(struct nacl_abi_stat   *dst,
                                         nacl_host_stat_t const *src) {
  nacl_abi_mode_t m;

  memset(dst, 0, sizeof *dst);

  if (src->st_size > INT32_MAX) {
    return -NACL_ABI_EOVERFLOW;
  }

  dst->nacl_abi_st_dev = 0;
  /*
   * MSDN says: st_ino has no meaning on most windows file systems,
   * but in case windows mounts a Unix filesystem, we copy it anyway.
   */
  if (src->st_ino != 0)
    dst->nacl_abi_st_ino = src->st_ino;
  else
    dst->nacl_abi_st_ino = NACL_FAKE_INODE_NUM;

  switch (src->st_mode & S_IFMT) {
    case S_IFREG:
      m = NACL_ABI_S_IFREG;
      break;
    case S_IFDIR:
      m = NACL_ABI_S_IFDIR;
      break;
    case S_IFCHR:
      /* stdin/out/err can be inherited, so this is okay */
      m = NACL_ABI_S_IFCHR;
      break;
    default:
      NaClLog(LOG_INFO,
              ("NaClAbiStatHostDescStatXlateCtor:"
               " Unusual NaCl descriptor type (not constructible)."
               " The NaCl app has a file with st_mode = 0%o.\n"),
              src->st_mode);
      m = NACL_ABI_S_UNSUP;
  }
  if (0 != (src->st_mode & _S_IREAD)) {
    m |= NACL_ABI_S_IRUSR;
  }
  if (0 != (src->st_mode & _S_IWRITE)) {
    m |= NACL_ABI_S_IWUSR;
  }
  if (0 != (src->st_mode & _S_IEXEC)) {
    m |= NACL_ABI_S_IXUSR;
  }
  dst->nacl_abi_st_mode = m;
  dst->nacl_abi_st_nlink = src->st_nlink;
  dst->nacl_abi_st_uid = (nacl_abi_uid_t) -1;  /* not root */
  dst->nacl_abi_st_gid = (nacl_abi_gid_t) -1;  /* not wheel */
  dst->nacl_abi_st_rdev = 0;
  dst->nacl_abi_st_size = (nacl_abi_off_t) src->st_size;
  dst->nacl_abi_st_blksize = 0;
  dst->nacl_abi_st_blocks = 0;
  dst->nacl_abi_st_atime = (nacl_abi_time_t) src->st_atime;
  dst->nacl_abi_st_mtime = (nacl_abi_time_t) src->st_mtime;
  dst->nacl_abi_st_ctime = (nacl_abi_time_t) src->st_ctime;

  /*
   * For now, zero these fields.  We may want to expose the
   * corresponding values if the underlying host OS supports
   * nanosecond resolution timestamps later.
   */
  dst->nacl_abi_st_atimensec = 0;
  dst->nacl_abi_st_mtimensec = 0;
  dst->nacl_abi_st_ctimensec = 0;

  return 0;
}

/* Read/write to a NaClHandle */
ssize_t NaClDescReadFromHandle(NaClHandle handle,
                               void       *buf,
                               size_t     length) {
  size_t count = 0;
  CHECK(length < kMaxSyncSocketMessageLength);

  while (count < length) {
    DWORD len;
    DWORD chunk = (DWORD) (
      ((length - count) <= UINT_MAX) ? (length - count) : UINT_MAX);
    if (ReadFile(handle, (char *) buf + count,
                 chunk, &len, NULL) == FALSE) {
      return (ssize_t) ((0 < count) ? count : -1);
    }
    count += len;
  }
  return (ssize_t) count;
}

ssize_t NaClDescWriteToHandle(NaClHandle handle,
                              void const *buf,
                              size_t     length) {
  size_t count = 0;
  CHECK(length < kMaxSyncSocketMessageLength);

  while (count < length) {
    DWORD len;
    DWORD chunk = (DWORD) (
      ((length - count) <= UINT_MAX) ? (length - count) : UINT_MAX);
    if (WriteFile(handle, (const char *) buf + count,
                  chunk, &len, NULL) == FALSE) {
      return (ssize_t) ((0 < count) ? count : -1);
    }
    count += len;
  }
  return (ssize_t) count;
}
