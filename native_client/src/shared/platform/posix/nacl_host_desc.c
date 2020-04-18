/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.  Memory
 * mapping using descriptors.
 *
 * Note that we avoid using the thread-specific data / thread local
 * storage access to the "errno" variable, and instead use the raw
 * system call return interface of small negative numbers as errors.
 */

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <utime.h>

#include "native_client/src/include/build_config.h"

#if NACL_LINUX
# include <pthread.h>
/* pthread_once for NaClHostDescInit, which is no-op on other OSes */
#endif

#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/include/portability.h"

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"

#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"
#include "native_client/src/trusted/service_runtime/include/sys/unistd.h"

#if NACL_LINUX
# include "native_client/src/shared/platform/posix/nacl_file_lock.h"
# if NACL_ANDROID
#  define PREAD pread
#  define PWRITE pwrite
# else
#  define PREAD pread64
#  define PWRITE pwrite64
# endif
#elif NACL_OSX
# define PREAD pread
# define PWRITE pwrite
#else
# error "Which POSIX OS?"
#endif

/*
 * Map our ABI to the host OS's ABI.
 */
static INLINE mode_t NaClMapMode(nacl_abi_mode_t abi_mode) {
  mode_t m = 0;
  if (0 != (abi_mode & NACL_ABI_S_IRUSR))
    m |= S_IRUSR;
  if (0 != (abi_mode & NACL_ABI_S_IWUSR))
    m |= S_IWUSR;
  if (0 != (abi_mode & NACL_ABI_S_IXUSR))
    m |= S_IXUSR;
  return m;
}

/*
 * Map our ABI to the host OS's ABI.
 */
static INLINE int NaClMapAccessMode(int nacl_mode) {
  int mode = 0;
  if (nacl_mode == NACL_ABI_F_OK) {
    mode = F_OK;
  } else {
    if (nacl_mode & NACL_ABI_R_OK)
      mode |= R_OK;
    if (nacl_mode & NACL_ABI_W_OK)
      mode |= W_OK;
    if (nacl_mode & NACL_ABI_X_OK)
      mode |= X_OK;
  }
  return mode;
}

/*
 * Map our ABI to the host OS's ABI.  On linux, this should be a big no-op.
 */
static INLINE int NaClMapOpenFlags(int nacl_flags) {
  int host_os_flags;

  nacl_flags &= (NACL_ABI_O_ACCMODE | NACL_ABI_O_CREAT | NACL_ABI_O_EXCL
                 | NACL_ABI_O_TRUNC | NACL_ABI_O_APPEND);

  host_os_flags = 0;
#define C(H) case NACL_ABI_ ## H: \
  host_os_flags |= H;             \
  break;
  switch (nacl_flags & NACL_ABI_O_ACCMODE) {
    C(O_RDONLY);
    C(O_WRONLY);
    C(O_RDWR);
  }
#undef C
#define M(H) do { \
    if (0 != (nacl_flags & NACL_ABI_ ## H)) {   \
      host_os_flags |= H;                       \
    }                                           \
  } while (0)
  M(O_CREAT);
  M(O_EXCL);
  M(O_TRUNC);
  M(O_APPEND);
#undef M
  return host_os_flags;
}

static INLINE int NaClMapOpenPerm(int nacl_perm) {
  int host_os_perm;

  host_os_perm = 0;
#define M(H) do { \
    if (0 != (nacl_perm & NACL_ABI_ ## H)) { \
      host_os_perm |= H; \
    } \
  } while (0)
  M(S_IRUSR);
  M(S_IWUSR);
#undef M
  return host_os_perm;
}

static INLINE int NaClMapFlagMap(int nacl_map_flags) {
  int host_os_flags;

  host_os_flags = 0;
#define M(H) do { \
    if (0 != (nacl_map_flags & NACL_ABI_ ## H)) { \
      host_os_flags |= H; \
    } \
  } while (0)
  M(MAP_SHARED);
  M(MAP_PRIVATE);
  M(MAP_FIXED);
  M(MAP_ANONYMOUS);
#undef M

  return host_os_flags;
}

/*
 * The NaClHostDescInit function should be invoked in all Ctor-like
 * functions.  Since no other NaClHostDesc member function should be
 * usable without having invoked a Ctor, lock management functions can
 * never be invoked without the lock manager object having been
 * initialized.
 */
#if NACL_LINUX

struct NaClFileLockManager gNaClFileLockManager;

static void NaClHostDescOnceInit(void) {
  NaClFileLockManagerCtor(&gNaClFileLockManager);
}

/* should be inlined */
static INLINE void NaClHostDescInit(void) {
  static pthread_once_t control = PTHREAD_ONCE_INIT;
  pthread_once(&control, NaClHostDescOnceInit);
}

static INLINE void NaClHostDescExclusiveLock(int host_desc) {
  NaClFileLockManagerLock(&gNaClFileLockManager, host_desc);
}

static INLINE void NaClHostDescExclusiveUnlock(int host_desc) {
  NaClFileLockManagerUnlock(&gNaClFileLockManager, host_desc);
}

#else

static INLINE void NaClHostDescInit(void) {
  ;
}

static INLINE void NaClHostDescExclusiveLock(int host_desc) {
  UNREFERENCED_PARAMETER(host_desc);
}

static INLINE void NaClHostDescExclusiveUnlock(int host_desc) {
  UNREFERENCED_PARAMETER(host_desc);
}

#endif

/*
 * TODO(bsy): handle the !NACL_ABI_MAP_FIXED case.
 */
uintptr_t NaClHostDescMap(struct NaClHostDesc *d,
                          struct NaClDescEffector *effp,
                          void                *start_addr,
                          size_t              len,
                          int                 prot,
                          int                 flags,
                          nacl_off64_t        offset) {
  int   desc;
  void  *map_addr;
  int   host_prot;
  int   tmp_prot;
  int   host_flags;
  int   need_exec;
  UNREFERENCED_PARAMETER(effp);

  NaClLog(4,
          ("NaClHostDescMap(0x%08"NACL_PRIxPTR", "
           "0x%08"NACL_PRIxPTR", 0x%08"NACL_PRIxS", "
           "0x%x, 0x%x, 0x%08"NACL_PRIx64")\n"),
          (uintptr_t) d,
          (uintptr_t) start_addr,
          len,
          prot,
          flags,
          (int64_t) offset);
  if (NULL == d && 0 == (flags & NACL_ABI_MAP_ANONYMOUS)) {
    NaClLog(LOG_FATAL, "NaClHostDescMap: 'this' is NULL and not anon map\n");
  }
  if (NULL != d && -1 == d->d) {
    NaClLog(LOG_FATAL, "NaClHostDescMap: already closed\n");
  }
  if ((0 == (flags & NACL_ABI_MAP_SHARED)) ==
      (0 == (flags & NACL_ABI_MAP_PRIVATE))) {
    NaClLog(LOG_FATAL,
            "NaClHostDescMap: exactly one of NACL_ABI_MAP_SHARED"
            " and NACL_ABI_MAP_PRIVATE must be set.\n");
  }

  prot &= NACL_ABI_PROT_MASK;

  if (flags & NACL_ABI_MAP_ANONYMOUS) {
    desc = -1;
  } else {
    desc = d->d;
  }
  /*
   * Translate prot, flags to host_prot, host_flags.
   */
  host_prot = NaClProtMap(prot);
  host_flags = NaClMapFlagMap(flags);

  NaClLog(4, "NaClHostDescMap: host_prot 0x%x, host_flags 0x%x\n",
          host_prot, host_flags);

  /*
   * In chromium-os, the /dev/shm and the user partition (where
   * installed apps live) are mounted no-exec, and a special
   * modification was made to the chromium-os version of the Linux
   * kernel to allow mmap to use files as backing store with
   * PROT_EXEC. The standard mmap code path will fail mmap requests
   * that ask for PROT_EXEC, but mprotect will allow chaning the
   * permissions later. This retains most of the defense-in-depth
   * property of disallowing PROT_EXEC in mmap, but enables the use
   * case of getting executable code from a file without copying.
   *
   * See https://code.google.com/p/chromium/issues/detail?id=202321
   * for details of the chromium-os change.
   */
  tmp_prot = host_prot & ~PROT_EXEC;
  need_exec = (0 != (PROT_EXEC & host_prot));
  map_addr = mmap(start_addr, len, tmp_prot, host_flags, desc, offset);
  if (need_exec && MAP_FAILED != map_addr) {
    if (0 != mprotect(map_addr, len, host_prot)) {
      /*
       * Not being able to turn on PROT_EXEC is fatal: we have already
       * replaced the original mapping -- restoring them would be too
       * painful.  Without scanning /proc (disallowed by outer
       * sandbox) or Mach's vm_region call, there is no way
       * simple/direct to figure out what was there before.  On Linux
       * we could have mremap'd the old memory elsewhere, but still
       * would require probing to find the contiguous memory segments
       * within the original address range.  And restoring dirtied
       * pages on OSX the mappings for which had disappeared may well
       * be impossible (getting clean copies of the pages is feasible,
       * but insufficient).
       */
      NaClLog(LOG_FATAL,
              "NaClHostDescMap: mprotect to turn on PROT_EXEC failed,"
              " errno %d\n", errno);
    }
  }

  NaClLog(4, "NaClHostDescMap: mmap returned %"NACL_PRIxPTR"\n",
          (uintptr_t) map_addr);

  if (MAP_FAILED == map_addr) {
    NaClLog(LOG_INFO,
            ("NaClHostDescMap: "
             "mmap(0x%08"NACL_PRIxPTR", 0x%"NACL_PRIxS", "
             "0x%x, 0x%x, 0x%d, 0x%"NACL_PRIx64")"
             " failed, errno %d.\n"),
            (uintptr_t) start_addr, len, host_prot, host_flags, desc,
            (int64_t) offset,
            errno);
    return -NaClXlateErrno(errno);
  }
  if (0 != (flags & NACL_ABI_MAP_FIXED) && map_addr != start_addr) {
    NaClLog(LOG_FATAL,
            ("NaClHostDescMap: mmap with MAP_FIXED not fixed:"
             " returned 0x%08"NACL_PRIxPTR" instead of 0x%08"NACL_PRIxPTR"\n"),
            (uintptr_t) map_addr,
            (uintptr_t) start_addr);
  }
  NaClLog(4, "NaClHostDescMap: returning 0x%08"NACL_PRIxPTR"\n",
          (uintptr_t) map_addr);

  return (uintptr_t) map_addr;
}

void NaClHostDescUnmapUnsafe(void *addr, size_t length) {
  if (munmap(addr, length) != 0) {
    NaClLog(LOG_FATAL, "NaClHostDescUnmapUnsafe: munmap() failed: "
            "address 0x%p, length 0x%" NACL_PRIxS ", errno %d\n",
            addr, length, errno);
  }
}

static int NaClHostDescCtor(struct NaClHostDesc  *d,
                            int fd,
                            int flags) {
  NaClHostDescInit();
  d->d = fd;
  d->flags = flags;
  NaClLog(3, "NaClHostDescCtor: success.\n");
  return 0;
}

int NaClHostDescOpen(struct NaClHostDesc  *d,
                     char const           *path,
                     int                  flags,
                     int                  mode) {
  int host_desc;
  int posix_flags;

  NaClLog(3, "NaClHostDescOpen(0x%08"NACL_PRIxPTR", %s, 0x%x, 0x%x)\n",
          (uintptr_t) d, path, flags, mode);
  if (NULL == d) {
    NaClLog(LOG_FATAL, "NaClHostDescOpen: 'this' is NULL\n");
  }
  /*
   * Sanitize access flags.
   */
  if (0 != (flags & ~NACL_ALLOWED_OPEN_FLAGS)) {
    return -NACL_ABI_EINVAL;
  }

  switch (flags & NACL_ABI_O_ACCMODE) {
    case NACL_ABI_O_RDONLY:
    case NACL_ABI_O_WRONLY:
    case NACL_ABI_O_RDWR:
      break;
    default:
      NaClLog(LOG_ERROR,
              "NaClHostDescOpen: bad access flags 0x%x.\n",
              flags);
      return -NACL_ABI_EINVAL;
  }

  posix_flags = NaClMapOpenFlags(flags);
#if NACL_LINUX
  posix_flags |= O_LARGEFILE;
#endif
  mode = NaClMapOpenPerm(mode);

  NaClLog(3, "NaClHostDescOpen: invoking POSIX open(%s,0x%x,0%o)\n",
          path, posix_flags, mode);
  host_desc = open(path, posix_flags, mode);
  NaClLog(3, "NaClHostDescOpen: got descriptor %d\n", host_desc);
  if (-1 == host_desc) {
    NaClLog(2, "NaClHostDescOpen: open returned -1, errno %d\n", errno);
    return -NaClXlateErrno(errno);
  }
  return NaClHostDescCtor(d, host_desc, flags);
}

int NaClHostDescPosixDup(struct NaClHostDesc  *d,
                         int                  posix_d,
                         int                  flags) {
  int host_desc;

  NaClHostDescInit();
  if (NULL == d) {
    NaClLog(LOG_FATAL, "NaClHostDescPosixDup: 'this' is NULL\n");
  }
  /*
   * Sanitize access flags.
   */
  if (0 != (flags & ~NACL_ALLOWED_OPEN_FLAGS)) {
    return -NACL_ABI_EINVAL;
  }

  switch (flags & NACL_ABI_O_ACCMODE) {
    case NACL_ABI_O_RDONLY:
    case NACL_ABI_O_WRONLY:
    case NACL_ABI_O_RDWR:
      break;
    default:
      NaClLog(LOG_ERROR,
              "NaClHostDescPosixDup: bad access flags 0x%x.\n",
              flags);
      return -NACL_ABI_EINVAL;
  }

  host_desc = dup(posix_d);
  if (-1 == host_desc) {
    return -NACL_ABI_EINVAL;
  }
  d->d = host_desc;
  d->flags = flags;
  return 0;
}

int NaClHostDescPosixTake(struct NaClHostDesc *d,
                          int                 posix_d,
                          int                 flags) {
  NaClHostDescInit();
  if (NULL == d) {
    NaClLog(LOG_FATAL, "NaClHostDescPosixTake: 'this' is NULL\n");
  }
  /*
   * Sanitize access flags.
   */
  if (0 != (flags & ~NACL_ALLOWED_OPEN_FLAGS)) {
    return -NACL_ABI_EINVAL;
  }

  switch (flags & NACL_ABI_O_ACCMODE) {
    case NACL_ABI_O_RDONLY:
    case NACL_ABI_O_WRONLY:
    case NACL_ABI_O_RDWR:
      break;
    default:
      NaClLog(LOG_ERROR,
              "NaClHostDescPosixTake: bad access flags 0x%x.\n",
              flags);
      return -NACL_ABI_EINVAL;
  }

  d->d = posix_d;
  d->flags = flags;
  return 0;
}

ssize_t NaClHostDescRead(struct NaClHostDesc  *d,
                         void                 *buf,
                         size_t               len) {
  ssize_t retval;

  NaClHostDescCheckValidity("NaClHostDescRead", d);
  if (NACL_ABI_O_WRONLY == (d->flags & NACL_ABI_O_ACCMODE)) {
    NaClLog(3, "NaClHostDescRead: WRONLY file\n");
    return -NACL_ABI_EBADF;
  }
  return ((-1 == (retval = read(d->d, buf, len)))
          ? -NaClXlateErrno(errno) : retval);
}

ssize_t NaClHostDescWrite(struct NaClHostDesc *d,
                          void const          *buf,
                          size_t              len) {
  /*
   * See NaClHostDescPWrite for details for why need_lock is required.
   */
  int need_lock;
  ssize_t retval;

  NaClHostDescCheckValidity("NaClHostDescWrite", d);
  if (NACL_ABI_O_RDONLY == (d->flags & NACL_ABI_O_ACCMODE)) {
    NaClLog(3, "NaClHostDescWrite: RDONLY file\n");
    return -NACL_ABI_EBADF;
  }

  need_lock = NACL_LINUX && (0 != (d->flags & NACL_ABI_O_APPEND));
  /*
   * Grab O_APPEND attribute lock, in case pwrite occurs in another
   * thread.
   */
  if (need_lock) {
    NaClHostDescExclusiveLock(d->d);
  }
  retval = write(d->d, buf, len);
  if (need_lock) {
    NaClHostDescExclusiveUnlock(d->d);
  }
  if (-1 == retval) {
    retval = -NaClXlateErrno(errno);
  }
  return retval;
}

nacl_off64_t NaClHostDescSeek(struct NaClHostDesc  *d,
                              nacl_off64_t         offset,
                              int                  whence) {
  nacl_off64_t retval;

  NaClHostDescCheckValidity("NaClHostDescSeek", d);
#if NACL_LINUX
  return ((-1 == (retval = lseek64(d->d, offset, whence)))
          ? -NaClXlateErrno(errno) : retval);
#elif NACL_OSX
  return ((-1 == (retval = lseek(d->d, offset, whence)))
          ? -NaClXlateErrno(errno) : retval);
#else
# error "What Unix-like OS is this?"
#endif
}

ssize_t NaClHostDescPRead(struct NaClHostDesc *d,
                          void *buf,
                          size_t len,
                          nacl_off64_t offset) {
  ssize_t retval;

  NaClHostDescCheckValidity("NaClHostDescPRead", d);
  if (NACL_ABI_O_WRONLY == (d->flags & NACL_ABI_O_ACCMODE)) {
    NaClLog(3, "NaClHostDescPRead: WRONLY file\n");
    return -NACL_ABI_EBADF;
  }
  return ((-1 == (retval = PREAD(d->d, buf, len, offset)))
          ? -NaClXlateErrno(errno) : retval);
}

ssize_t NaClHostDescPWrite(struct NaClHostDesc *d,
                           void const *buf,
                           size_t len,
                           nacl_off64_t offset) {
  int need_lock;
  ssize_t retval;

  NaClHostDescCheckValidity("NaClHostDescPWrite", d);
  if (NACL_ABI_O_RDONLY == (d->flags & NACL_ABI_O_ACCMODE)) {
    NaClLog(3, "NaClHostDescPWrite: RDONLY file\n");
    return -NACL_ABI_EBADF;
  }
  /*
   * Linux's interpretation of what the POSIX standard requires
   * differs from OSX.  On Linux, pwrite using a descriptor that was
   * opened with O_APPEND will ignore the supplied offset parameter
   * and append.  On OSX, the supplied offset parameter wins.
   *
   * The POSIX specification at
   *
   *  http://pubs.opengroup.org/onlinepubs/9699919799/functions/pwrite.html
   *
   * says that pwrite should always pay attention to the supplied offset.
   *
   * We standardize on POSIX behavior.
   */
  need_lock = NACL_LINUX && (0 != (d->flags & NACL_ABI_O_APPEND));
  if (need_lock) {
    int orig_flags;
    /*
     * Grab lock that all NaCl platform library using applications
     * will use.  NB: if the descriptor is shared with a non-NaCl
     * platform library-using application, there is a race.
     */
    NaClHostDescExclusiveLock(d->d);
    /*
     * Temporarily disable O_APPEND and issue the pwrite.
     */
    orig_flags = fcntl(d->d, F_GETFL, 0);
    CHECK(orig_flags & O_APPEND);
    if (-1 == fcntl(d->d, F_SETFL, orig_flags & ~O_APPEND)) {
      NaClLog(LOG_FATAL,
              "NaClHostDescPWrite: could not fcntl F_SETFL (~O_APPEND)\n");
    }
    retval = PWRITE(d->d, buf, len, offset);
    /*
     * Always re-enable O_APPEND regardless of pwrite success or
     * failure.
     */
    if (-1 == fcntl(d->d, F_SETFL, orig_flags)) {
      NaClLog(LOG_FATAL,
              "NaClHostDescPWrite: could not fcntl F_SETFL (restore)\n");
    }
    NaClHostDescExclusiveUnlock(d->d);
    if (-1 == retval) {
      retval = -NaClXlateErrno(errno);
    }
    return retval;
  }

  return ((-1 == (retval = PWRITE(d->d, buf, len, offset)))
          ? -NaClXlateErrno(errno) : retval);
}

/*
 * See NaClHostDescStat below.
 */
int NaClHostDescFstat(struct NaClHostDesc  *d,
                      nacl_host_stat_t     *nhsp) {
  NaClHostDescCheckValidity("NaClHostDescFstat", d);
  if (NACL_HOST_FSTAT64(d->d, nhsp) == -1) {
    return -NaClXlateErrno(errno);
  }

  return 0;
}

int NaClHostDescIsatty(struct NaClHostDesc *d) {
  int retval;

  NaClHostDescCheckValidity("NaClHostDescIsatty", d);
  retval = isatty(d->d);
  /* When isatty fails it returns zero and sets errno. */
  return (0 == retval) ? -NaClXlateErrno(errno) : retval;
}

int NaClHostDescClose(struct NaClHostDesc *d) {
  int retval;

  NaClHostDescCheckValidity("NaClHostDescClose", d);
  retval = close(d->d);
  if (-1 != retval) {
    d->d = -1;
  }
  return (-1 == retval) ? -NaClXlateErrno(errno) : retval;
}

/*
 * This is not a host descriptor function, but is closely related to
 * fstat and should behave similarly.
 */
int NaClHostDescStat(char const *path, nacl_host_stat_t *nhsp) {

  if (NACL_HOST_STAT64(path, nhsp) == -1) {
    return -NaClXlateErrno(errno);
  }

  return 0;
}

int NaClHostDescMkdir(const char *path, int mode) {
  if (mkdir(path, mode) != 0)
    return -NaClXlateErrno(errno);
  return 0;
}

int NaClHostDescRmdir(const char *path) {
  if (rmdir(path) != 0)
    return -NaClXlateErrno(errno);
  return 0;
}

int NaClHostDescChdir(const char *path) {
  if (chdir(path) != 0)
    return -NaClXlateErrno(errno);
  return 0;
}

int NaClHostDescGetcwd(char *path, size_t len) {
  if (getcwd(path, len) == NULL)
    return -NaClXlateErrno(errno);
  return 0;
}

int NaClHostDescUnlink(const char *path) {
  if (unlink(path) != 0)
    return -NaClXlateErrno(errno);
  return 0;
}

int NaClHostDescTruncate(char const *path, nacl_abi_off_t length) {
  if (truncate(path, length) != 0)
    return -NaClXlateErrno(errno);
  return 0;
}

int NaClHostDescLstat(char const *path, nacl_host_stat_t *nhsp) {
  if (NACL_HOST_LSTAT64(path, nhsp) == -1)
    return -NaClXlateErrno(errno);
  return 0;
}

int NaClHostDescLink(const char *oldpath, const char *newpath) {
  if (link(oldpath, newpath) != 0)
    return -NaClXlateErrno(errno);
  return 0;
}

int NaClHostDescRename(const char *oldpath, const char *newpath) {
  if (rename(oldpath, newpath) != 0)
    return -NaClXlateErrno(errno);
  return 0;
}

int NaClHostDescSymlink(const char *oldpath, const char *newpath) {
  if (symlink(oldpath, newpath) != 0)
    return -NaClXlateErrno(errno);
  return 0;
}

int NaClHostDescChmod(const char *path, nacl_abi_mode_t mode) {
  if (chmod(path, NaClMapMode(mode)) != 0)
    return -NaClXlateErrno(errno);
  return 0;
}

int NaClHostDescAccess(const char *path, int amode) {
  if (access(path, NaClMapAccessMode(amode)) != 0)
    return -NaClXlateErrno(errno);
  return 0;
}

int NaClHostDescReadlink(const char *path, char *buf, size_t bufsize) {
  int retval = readlink(path, buf, bufsize);
  if (retval < 0)
    return -NaClXlateErrno(errno);
  return retval;
}

int NaClHostDescUtimes(const char *filename,
                       const struct nacl_abi_timeval *times) {
  struct timeval host_times[2];
  if (times != NULL) {
    host_times[0].tv_sec = times[0].nacl_abi_tv_sec;
    host_times[0].tv_usec = times[0].nacl_abi_tv_usec;
    host_times[1].tv_sec = times[1].nacl_abi_tv_sec;
    host_times[1].tv_usec = times[1].nacl_abi_tv_usec;
  }
  if (-1 == utimes(filename, (times != NULL ? host_times : NULL))) {
    return -NaClXlateErrno(errno);
  }
  return 0;
}

int NaClHostDescFchdir(struct NaClHostDesc *d) {
  NaClHostDescCheckValidity("NaClHostDescFchdir", d);
  if (-1 == fchdir(d->d)) {
    return -NaClXlateErrno(errno);
  }
  return 0;
}

int NaClHostDescFchmod(struct NaClHostDesc *d, nacl_abi_mode_t mode) {
  NaClHostDescCheckValidity("NaClHostDescFchmod", d);
  if (-1 == fchmod(d->d, NaClMapMode(mode))) {
    return -NaClXlateErrno(errno);
  }
  return 0;
}

int NaClHostDescFsync(struct NaClHostDesc *d) {
  NaClHostDescCheckValidity("NaClHostDescFsync", d);
  if (-1 == fsync(d->d)) {
    return -NaClXlateErrno(errno);
  }
  return 0;
}

int NaClHostDescFdatasync(struct NaClHostDesc *d) {
  NaClHostDescCheckValidity("NaClHostDescFdatasync", d);
  /* fdatasync does not exist for osx. */
#if NACL_OSX
  if (-1 == fsync(d->d)) {
#else
  if (-1 == fdatasync(d->d)) {
#endif
    return -NaClXlateErrno(errno);
  }
  return 0;
}

int NaClHostDescFtruncate(struct NaClHostDesc *d, nacl_off64_t length) {
  NaClHostDescCheckValidity("NaClHostDescFtruncate", d);
#if NACL_LINUX
  if (-1 == ftruncate64(d->d, length)) {
#elif NACL_OSX
  if (-1 == ftruncate(d->d, length)) {
#else
# error Unsupported platform
#endif
    return -NaClXlateErrno(errno);
  }
  return 0;
}
