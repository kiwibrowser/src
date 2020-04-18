/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.  Memory
 * mapping using descriptors.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_HOST_DESC_H__
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_HOST_DESC_H__

#include <sys/stat.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/trusted/service_runtime/include/sys/time.h"

#if NACL_LINUX || NACL_OSX
# include "native_client/src/shared/platform/posix/nacl_host_desc_types.h"
#elif NACL_WINDOWS
# include "native_client/src/shared/platform/win/nacl_host_desc_types.h"
#endif



/*
 * see NACL_MAP_PAGESIZE from nacl_config.h; map operations must be aligned
 */

EXTERN_C_BEGIN

struct nacl_abi_stat;
struct NaClDescEffector;
struct NaClHostDesc;

/*
 * off64_t in linux, off_t in osx and __int64_t in win
 */
typedef int64_t nacl_off64_t;

/*
 * We do not explicitly provide an abstracted version of a
 * host-independent stat64 structure.  Instead, it is up to the user
 * of the nacl_host_desc code to not use anything but the
 * POSIX-blessed fields, to know that the shape/size may differ across
 * platforms, and to know that the st_size field is a 64-bit value
 * compatible w/ nacl_off64_t above.
 */
#if NACL_LINUX
typedef struct stat64 nacl_host_stat_t;
# define NACL_HOST_FSTAT64 fstat64
# define NACL_HOST_STAT64 stat64
# define NACL_HOST_LSTAT64 lstat64
#elif NACL_OSX
typedef struct stat nacl_host_stat_t;
# define NACL_HOST_FSTAT64 fstat
# define NACL_HOST_STAT64 stat
# define NACL_HOST_LSTAT64 lstat
#elif NACL_WINDOWS
typedef struct _stati64 nacl_host_stat_t;
# define NACL_HOST_FSTAT64 _fstat64
# define NACL_HOST_STAT64 _stati64
#elif defined __native_client__
/* nacl_host_stat_t not exposed to NaCl module code */
#else
# error "what OS?"
#endif

/*
 * TODO(bsy): it seems like these error functions are useful in more
 * places than just the NaClDesc library. Move them to a more central
 * header.  When that's done, it should be possible to include this
 * header *only* from host code, removing the need for the #ifdef
 * __native_client__ directives.
 *
 * Currently that's not possible, unless we want to forego
 * NaCl*IsNegErrno in trusted code, and go back to writing if(retval <
 * 0) to check for errors.
 */

/*
 * On 64-bit Linux, the app has the entire 32-bit address space
 * (kernel no longer occupies the top 1G), so what is an errno and
 * what is an address is trickier: we require that our NACL_ABI_
 * errno values be less than 64K.
 *
 * NB: The runtime assumes that valid errno values can ALWAYS be
 * represented using no more than 16 bits. If this assumption
 * changes, all code dealing with error number should be reviewed.
 *
 * NB 2010-02-03: The original code for this function did not work:
 *   return ((uint64_t) val) >= ~((uint64_t) 0xffff);
 * Macintosh optimized builds were not able to recognize negative values.
 * All other platforms as well as Macintosh debug builds worked fine.
 *
 * NB the 3rd, 2010-10-19: these functions take pointer arguments
 * to discourage accidental use of narrowing/widening conversions,
 * which have caused problems in the past.  We assume without proof
 * that the compiler will do the right thing when inlining.
 */
static INLINE int NaClPtrIsNegErrno(const uintptr_t *val) {
  return (*val & ~((uintptr_t) 0xffff)) == ~((uintptr_t) 0xffff);
}

static INLINE int NaClSSizeIsNegErrno(const ssize_t *val) {
  return (*val & ~((ssize_t) 0xffff)) == ~((ssize_t) 0xffff);
}

static INLINE int NaClOff64IsNegErrno(const nacl_off64_t *val) {
  return (*val & ~((nacl_off64_t) 0xffff)) == ~((nacl_off64_t) 0xffff);
}

extern int NaClXlateErrno(int errnum);

extern int NaClXlateNaClSyncStatus(NaClSyncStatus status);

#ifndef __native_client__ /* these functions are not exposed to NaCl modules
                           * (see TODO comment above)
                           */
/*
 * Mapping data from a file.
 *
 * start_addr and len must be multiples of NACL_MAP_PAGESIZE.
 *
 * Of prot bits, NACL_ABI_PROT_READ, NACL_ABI_PROT_WRITE, and
 * NACL_ABI_PROT_WRITE are allowed.  Of flags, only
 * NACL_ABI_MAP_ANONYMOUS is allowed.  start_addr may be NULL when
 * NACL_ABI_MAP_FIXED is not set, and in Windows NaClFindAddressSpace
 * is used to find a starting address (NB: NaCl module syscall
 * handling should always use NACL_ABI_MAP_FIXED to select the
 * location to be within the untrusted address space).  start_address,
 * len, and offset must be a multiple of NACL_MAP_PAGESIZE.
 *
 * Note that in Windows, in order for the mapping to be coherent (two
 * or more shared mappings/views of the file data will show changes
 * that are made in another mapping/view), the mappings must use the
 * same mapping handle or open file handle.  Either may be sent to
 * another process to achieve this.  Windows does not guarantee
 * coherence, however, if a process opened the same file again and
 * performed CreateFileMapping / MapViewOfFile using the new file
 * handle.
 *
 * Underlying host-OS syscalls:  mmap / MapViewOfFileEx
 *
 * 4GB file max
 */
extern uintptr_t NaClHostDescMap(struct NaClHostDesc  *d,
                                 struct NaClDescEffector *effp,
                                 void                 *start_addr,
                                 size_t               len,
                                 int                  prot,
                                 int                  flags,
                                 nacl_off64_t         offset) NACL_WUR;

/*
 * Undo a file mapping.  The memory range specified by start_address,
 * len must be memory that came from NaClHostDescMap.
 *
 * start_addr and len must be multiples of NACL_MAP_PAGESIZE.
 *
 * Underlying host-OS syscalls: munmap / UnmapViewOfFile
 *
 * This is labelled as "unsafe" because it is not safe to use on the
 * untrusted address space, because it leaves an unallocated hole in
 * address space.
 */
extern void NaClHostDescUnmapUnsafe(void   *start_addr,
                                    size_t len);


/*
 * These are the flags that are permitted.
 */
#define NACL_ALLOWED_OPEN_FLAGS (NACL_ABI_O_ACCMODE | NACL_ABI_O_CREAT | \
    NACL_ABI_O_TRUNC | NACL_ABI_O_APPEND | NACL_ABI_O_EXCL)

/*
 * Constructor for a NaClHostDesc object.
 *
 * |path| should be a host-OS pathname to a file.  No validation is
 * done.  |flags| should contain one of NACL_ABI_O_RDONLY,
 * NACL_ABI_O_WRONLY, and NACL_ABI_O_RDWR, and can additionally
 * contain NACL_ABI_O_CREAT, NACL_ABI_O_TRUNC, NACL_ABI_O_EXCL and
 * NACL_ABI_O_APPEND.
 *
 * Uses raw syscall return convention, so returns 0 for success and
 * non-zero (usually -NACL_ABI_EINVAL) for failure.
 *
 * We cannot return the platform descriptor/handle and be consistent
 * with a largely POSIX-ish interface, since e.g. windows handles may
 * be negative and might look like negative errno returns.  Currently
 * we use the posix API on windows, so it could work, but we may need
 * to change later.
 *
 * Underlying host-OS functions: open / _s_open_s
 */
extern int NaClHostDescOpen(struct NaClHostDesc *d,
                            char const          *path,
                            int                 flags,
                            int                 perms) NACL_WUR;

/*
 * Constructor for a NaClHostDesc object.
 *
 * Uses raw syscall return convention, so returns 0 for success and
 * non-zero (usually -NACL_ABI_EINVAL) for failure.
 *
 * d is a POSIX-interface descriptor
 *
 * flags may only contain one of NACL_ABI_O_RDONLY, NACL_ABI_O_WRONLY,
 * or NACL_ABI_O_RDWR, and must be the NACL_ABI_* versions of the
 * actual mode that d was opened with.  NACL_ABI_O_CREAT/APPEND are
 * permitted, but ignored, so it is safe to pass the same flags value
 * used in NaClHostDescOpen and pass it to NaClHostDescPosixDup.
 *
 * Underlying host-OS functions: dup / _dup; mode is what posix_d was
 * opened with
 */
extern int NaClHostDescPosixDup(struct NaClHostDesc *d,
                                int                 posix_d,
                                int                 flags) NACL_WUR;

/*
 * Essentially the same as NaClHostDescPosixDup, but without the dup
 * -- takes ownership of the descriptor rather than making a dup.
 */
extern int NaClHostDescPosixTake(struct NaClHostDesc  *d,
                                 int                  posix_d,
                                 int                  flags) NACL_WUR;


/*
 * Allocates a NaClHostDesc and invokes NaClHostDescPosixTake on it.
 * Aborts process if no memory.
 */
extern struct NaClHostDesc *NaClHostDescPosixMake(int posix_d,
                                                  int flags) NACL_WUR;
/*
 * Read data from an opened file into a memory buffer.
 *
 * buf is not validated.
 *
 * Underlying host-OS functions: read / FileRead
 */
extern ssize_t NaClHostDescRead(struct NaClHostDesc *d,
                                void                *buf,
                                size_t              len) NACL_WUR;


/*
 * Write data from a memory buffer into an opened file.
 *
 * buf is not validated.
 *
 * Underlying host-OS functions: write / FileWrite
 */
extern ssize_t NaClHostDescWrite(struct NaClHostDesc  *d,
                                 void const           *buf,
                                 size_t               count) NACL_WUR;

/*
 * Read data from an opened file into a memory buffer from specified
 * offset into file.
 *
 * buf is not validated.
 *
 * Underlying host-OS functions: pread{,64} / FileRead
 */
extern ssize_t NaClHostDescPRead(struct NaClHostDesc *d,
                                 void *buf,
                                 size_t len,
                                 nacl_off64_t offset) NACL_WUR;


/*
 * Write data from a memory buffer into an opened file at the specific
 * offset in the file.
 *
 * buf is not validated.
 *
 * Underlying host-OS functions: pwrite{,64} / FileWrite
 */
extern ssize_t NaClHostDescPWrite(struct NaClHostDesc  *d,
                                  void const *buf,
                                  size_t count,
                                  nacl_off64_t offset) NACL_WUR;

extern nacl_off64_t NaClHostDescSeek(struct NaClHostDesc *d,
                                     nacl_off64_t        offset,
                                     int                 whence);

/*
 * Fstat.
 */
extern int NaClHostDescFstat(struct NaClHostDesc  *d,
                             nacl_host_stat_t     *nasp) NACL_WUR;

/*
 * Isatty. Determine if file descriptor is connected to a TTY.
 * Returns 1 if the descriptor is a TTY, otherwise returns a negative errno
 * value.
 */
extern int NaClHostDescIsatty(struct NaClHostDesc *d) NACL_WUR;

/*
 * Dtor for the NaClHostFile object. Close the file.
 *
 * Underlying host-OS functions:  close(2) / _close
 */
extern int NaClHostDescClose(struct NaClHostDesc  *d) NACL_WUR;

extern int NaClHostDescStat(char const        *host_os_pathname,
                            nacl_host_stat_t  *nasp) NACL_WUR;

/*
 * Create directory
 */
extern int NaClHostDescMkdir(const char *path, int mode) NACL_WUR;

/*
 * Remove directory
 */
extern int NaClHostDescRmdir(const char *path) NACL_WUR;

/*
 * Change current working directory
 */
extern int NaClHostDescChdir(const char *path) NACL_WUR;

/*
 * Get current working directory.
 * This works like the POSIX getcwd(3) function except that it returns
 * an error code rather than the resulting buffer.  The provided path may not
 * be NULL.
 */
extern int NaClHostDescGetcwd(char *path, size_t len) NACL_WUR;

/*
 * Remove/delete the underlying file.
 * Underlying host-OS functions:  unlink(2) / _unlink
 */
extern int NaClHostDescUnlink(char const *path) NACL_WUR;

extern int NaClHostDescTruncate(char const *path,
                                nacl_abi_off_t length) NACL_WUR;

extern int NaClHostDescLstat(char const *path,
                             nacl_host_stat_t *nasp) NACL_WUR;

extern int NaClHostDescLink(const char *oldpath, const char *newpath) NACL_WUR;

/*
 * Rename a file or directory.
 */
extern int NaClHostDescRename(const char *oldpath,
                              const char *newpath) NACL_WUR;

/*
 * Create a new symlink called 'newpath', pointing to 'oldpath'.
 */
extern int NaClHostDescSymlink(const char *oldpath,
                               const char *newpath) NACL_WUR;

extern int NaClHostDescChmod(const char *path, nacl_abi_mode_t amode) NACL_WUR;

extern int NaClHostDescAccess(const char *path, int amode) NACL_WUR;

/*
 * Find the target of a symlink.
 * Like readlink(2) this function writes up to 'bufsize' bytes to 'buf' and
 * returns the number of bytes written but never writes the NULL terminator.
 * Returns -1 and sets errno on error.
 */
extern int NaClHostDescReadlink(const char *path, char *buf, size_t bufsize);

extern int NaClHostDescUtimes(const char *filename,
                              const struct nacl_abi_timeval *times) NACL_WUR;

extern int NaClHostDescFchmod(struct NaClHostDesc *d,
                              nacl_abi_mode_t mode) NACL_WUR;

extern int NaClHostDescFsync(struct NaClHostDesc *d) NACL_WUR;

extern int NaClHostDescFdatasync(struct NaClHostDesc *d) NACL_WUR;

extern int NaClHostDescFtruncate(struct NaClHostDesc *d,
                                 nacl_off64_t length) NACL_WUR;

/*
 * Maps NACI_ABI_ versions of the mmap prot argument to host ABI versions
 * of the bit values
 */
extern int NaClProtMap(int abi_prot);

/*
 * Utility routine.  Checks |d| is non-NULL, and that d->d is not -1.
 * If either is true, an appropriate LOG_FATAL message with |fn_name|
 * interpolated is generated.
 */
extern void NaClHostDescCheckValidity(char const *fn_name,
                                      struct NaClHostDesc *d);

#if NACL_WINDOWS
extern void NaClflProtectAndDesiredAccessMap(int prot,
                                             int is_private,
                                             int accmode,
                                             DWORD *out_flProtect,
                                             DWORD *out_dwDesiredAccess,
                                             DWORD *out_flNewProtect,
                                             char const **out_msg);

extern DWORD NaClflProtectMap(int prot);
#endif

EXTERN_C_END

#endif  /* defined __native_client__ */

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_HOST_DESC_H__ */
