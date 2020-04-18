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
#include "native_client/src/include/portability_io.h"

#include <windows.h>
#include <direct.h>
#include <io.h>
#include <sys/types.h>
#include <sys/utime.h>
#include <share.h>
#include <accctrl.h>
#include <aclapi.h>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_find_addrsp.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/shared/platform/win/xlate_system_error.h"
#include "native_client/src/trusted/desc/nacl_desc_effector.h"
#include "native_client/src/trusted/desc/nacl_desc_effector_trusted_mem.h"

#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/internal_errno.h"
#include "native_client/src/trusted/service_runtime/sel_util-inl.h"

#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/include/bits/stat.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/service_runtime/include/sys/unistd.h"

#define OFFSET_FOR_FILEPOS_LOCK (GG_LONGLONG(0x7000000000000000))

/*
 * By convention, we use locking the byte at OFFSET_FOR_FILEPOS_LOCK
 * as locking for the implicit file position associated with a file
 * handle.  According to MSDN, LockFileEx of a byte range that does
 * not (yet) exist in a file is not an error, which makes sense in
 * that one might want to have exclusive access to a file region that
 * is beyond the end of the file before populating it.  We assume that
 * OFFSET_FOR_FILEPOS_LOCK is large enough that no real file will
 * actually be that big (even if sparse) and cause problems.
 *
 * One drawback of this is that two independent file handles on the
 * same file will share the same lock.  If this leads to actual
 * contention issues, we can use the following randomized approach,
 * ASSUMING that each file handle / posix-level host descriptor is
 * introduced to NaCl at most once (e.g., no dup'ing and invoking
 * NaClHostDescPosixTake multiple times): we pick a random offset from
 * OFFSET_FOR_FILEPOS_LOCK, and make sure we transfer that with the
 * file handle in the nrd_xfer protocol.  This way, we use a range of
 * byte offsets for locking files and avoid false contention.  We
 * would be subject to the birthday paradox, of course, so if we
 * picked a 16-bit random offset to use, then if a file is opened ~256
 * times we would start seeing performance issues caused by
 * contention, which is probably acceptable; a 32-bit nonce would be
 * plenty.
 *
 * On Windows, fcntl is not available.  A very similar function to
 * lockf, _locking, exists in the Windows CRT.  It does not permit
 * specification of the start of a region, only size (just like lockf)
 * -- implicitly from the current position -- which is less than
 * useful for our purposes.
 */
static void NaClTakeFilePosLock(HANDLE hFile) {
  OVERLAPPED overlap;
  DWORD err;

  memset(&overlap, 0, sizeof overlap);
  overlap.Offset = (DWORD) (OFFSET_FOR_FILEPOS_LOCK & 0xffffffffU);
  overlap.OffsetHigh = (DWORD) (OFFSET_FOR_FILEPOS_LOCK >> 32);
  /*
   * LockFileEx should never fail -- untrusted code cannot cause hFile
   * to become invalid, since all NaClHostDesc objects are wrapped in
   * NaClDesc objects and all uses of NaClDesc objects take a
   * reference before use, so a threading race that closes a
   * descriptor at the untrusted code level will only dereference the
   * NaClDesc (and make it unavailable to the untrusted code), but the
   * object will not be destroyed until after the NaClDesc-level
   * operation (which in turn invokes the NaClHostDesc level
   * operation) completes.  Only after the operation completes will
   * the reference to the NaClDesc be drop by the syscall handler.
   */
  if (!LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK,
                  /* dwReserved= */ 0,
                  /* nNumberOfBytesToLockLow= */ 1,
                  /* nNumberOfBytesToLockHigh= */ 0,
                  &overlap)) {
    err = GetLastError();
    NaClLog(LOG_FATAL, "NaClTakeFilePosLock: LockFileEx failed, error %u\n",
            err);
  }
}

/*
 * Map our ABI to the host OS's ABI.
 * Note: there is no X bit equivalent on windows so NACL_ABI_S_IXUSR
 * is ignored.
 */
static INLINE mode_t NaClMapMode(nacl_abi_mode_t abi_mode) {
  mode_t m = 0;
  if (0 != (abi_mode & NACL_ABI_S_IRUSR))
    m |= _S_IREAD;
  if (0 != (abi_mode & NACL_ABI_S_IWUSR))
    m |= _S_IWRITE;
  return m;
}

/* Windows doesn't define R_OK or W_OK macros but expects these constants */
#define WIN_F_OK 0
#define WIN_R_OK 4
#define WIN_W_OK 2

/*
 * Map our ABI to the host OS's ABI.
 * There is no X_OK (0x1) on win32 so we ignore
 * NACL_ABI_X_OK and report everything that exists
 * as being executable.
 */
static INLINE int NaClMapAccessMode(int nacl_mode) {
  int mode = 0;
  if (nacl_mode == NACL_ABI_F_OK) {
    mode = WIN_F_OK;
  } else {
    if (nacl_mode & NACL_ABI_R_OK)
      mode |= WIN_R_OK;
    if (nacl_mode & NACL_ABI_W_OK)
      mode |= WIN_W_OK;
  }
  return mode;
}

static void NaClDropFilePosLock(HANDLE hFile) {
  OVERLAPPED overlap;
  DWORD err;

  memset(&overlap, 0, sizeof overlap);
  overlap.Offset = (DWORD) (OFFSET_FOR_FILEPOS_LOCK & 0xffffffffU);
  overlap.OffsetHigh = (DWORD) (OFFSET_FOR_FILEPOS_LOCK >> 32);
  if (!UnlockFileEx(hFile,
                    /* dwReserved= */ 0,
                    /* nNumberOfBytesToLockLow= */ 1,
                    /* nNumberOfBytesToLockHigh= */ 0,
                    &overlap)) {
    err = GetLastError();
    NaClLog(LOG_FATAL, "NaClDropFilePosLock: UnlockFileEx failed, error %u\n",
            err);
  }
}

static nacl_off64_t NaClLockAndGetCurrentFilePos(HANDLE hFile) {
  LARGE_INTEGER to_move;
  LARGE_INTEGER cur_pos;
  DWORD err;

  NaClTakeFilePosLock(hFile);
  to_move.QuadPart = 0;
  if (!SetFilePointerEx(hFile, to_move, &cur_pos, FILE_CURRENT)) {
    err = GetLastError();
    NaClLog(LOG_FATAL,
            "NaClLockAndGetCurrentFilePos: SetFilePointerEx failed, error %u\n",
            err);
  }
  return cur_pos.QuadPart;
}

static void NaClSetCurrentFilePosAndUnlock(HANDLE hFile,
                                           nacl_off64_t pos) {
  LARGE_INTEGER to_move;
  DWORD err;

  to_move.QuadPart = pos;
  if (!SetFilePointerEx(hFile, to_move, (LARGE_INTEGER *) NULL, FILE_BEGIN)) {
    err = GetLastError();
    NaClLog(LOG_FATAL,
            "NaClSetCurrentFilePosAndUnlock: SetFilePointerEx failed:"
            " error %d\n",
            err);
  }
  NaClDropFilePosLock(hFile);
}

/*
 * WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
 *
 * The implementation of the host descriptor abstractions will
 * probably change.  In particularly, blocking I/O calls must be
 * interruptible in order to implement the address-space move
 * mechanism for mmap error recovery, and the it seems that the only
 * way that this would be feasible is to do the following: instead of
 * using the POSIX abstraction layer, do the I/O using Windows file
 * handles opened for asynchronous operations.  When a potentially
 * blocking system call (e.g., read or write) is performed, use
 * overlapped I/O via ReadFile/WriteFile to initiate the I/O operation
 * in a non-blocking manner, and use a separate event object, so that
 * the thread can, after initiating the I/O, perform
 * WaitForMultipleObjects on both I/O completion (event in the
 * OVERLAPPED structure) and on mmap-generated interrupts.  The event
 * can be signalled via SetEvent by any other thread that wish to
 * perform a safe mmap operation.
 *
 * When the safe mmap is to occur, all other application threads are
 * stopped (beware, of course, of the race condition where two threads
 * try to do mmap), and the remaining running thread performs
 * VirtualFree and MapViewOfFileEx.  If a thread (from some injected
 * DLL) puts some memory in the hole created by VirtualFree before the
 * MapViewOfFileEx runs, then we have to move the entire address space
 * to avoid allowing the untrusted NaCl app from touching this
 * innocent thread's memory.
 *
 * What this implies is that a mechanism must exist in order for the
 * mmapping thread to stop all other application threads, and this is
 * why the blocking syscalls must be interruptible.  When interrupted,
 * the thread that initiated the I/O must perform CancelIo and check,
 * via GetOverlappedResult, to see how much have completed, etc, then
 * put itself into a restartable state -- we might simply return EINTR
 * if no work has been dnoe and require libc to restart the syscall in
 * the SysV style, though it should be possible to just restart the
 * syscall in the BSD style -- and to signal the mmapping thread that
 * it is ready.
 *
 * Alternatively, these interrupted operations can return a private
 * version of EAGAIN, so that the code calling the host descriptor
 * (syscall handler) can quiesce the thread and restart the I/O
 * operation once the address space move is complete.
 *
 * WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
 */

/*
 * TODO(bsy, gregoryd): check that _get_errno is indeed a thread-safe way
 * to get the error from the last 'syscall' into the posix layer.
 */
int GetErrno(void) {
  int thread_errno;

  (void) _get_errno(&thread_errno);
  return NaClXlateErrno(thread_errno);
}

static INLINE size_t size_min(size_t a, size_t b) {
  return (a < b) ? a : b;
}

/*
 * The mapping and unmapping code work in 64K chunks rather than a
 * single large allocation because all of our uses will use 64K
 * chunks.  Higher level code keeps track of whether memory came from
 * VirtualAlloc or NaClHostDescMap, and will call the appropriate
 * deallocation functions.
 *
 * NB: if prot is NACL_ABI_PROT_NONE, then the memory should be
 * deallocated via VirtualFree as if it came from paging file rather
 * than via a file mapping object representing the paging file (and
 * thus UnmapViewOfFile).
 */

/*
 * out_flProtect == 0 means error, and the error string can be used
 * for a logging message (except for the cases that the caller should
 * be checking for).
 *
 * in parameters are all NACL_ABI_ values or bool (0/1).
 *
 * accmode may be NACL_ABI_O_RDONLY or NACL_ABI_O_RDWR, but not
 * NACL_ABI_O_WRONLY (see below).
 *
 * Caller should check for:
 *
 * - PROT_NONE -> special case handling,
 * - NACL_ABI_O_APPEND and PROT_WRITE -> EACCES,
 * - accmode is O_WRONLY -> EACCES,
 *
 * The interpretation here is that PROT_EXEC or PROT_WRITE implies
 * asking for PROT_READ, since most hardware behaves this way.  So if
 * the descriptor is O_WRONLY, we generally refuse.
 *
 * The file mapping object created by CreateFileMapping's flProtect
 * argument specifies the MAXIMUM protection, and MapViewOfFileEx will
 * request a lesser level of access.  We should always
 * CreateFileMapping with a high level of access so that
 * VirtualProtect (used by mprotect) can be used to turn on write when
 * the initial mmap had read-only mappings.
 *
 * BUG(phosek): Due to Windows XP limitation, in particular the missing
 * PAGE_EXECUTE_WRITECOPY protection support for file mapping objects,
 * we cannot mmap r/o file as private, read/write and later make it
 * executable or vice versa mmap r/o file as private, read/execute and
 * later make it writable. This is a platform difference, but since
 * untrusted code is not allowed to mmap files as write/execute, this
 * difference is invisible to application developers and will therefore
 * likely remain unresolved as the solution would likely be very
 * expensive. Furthemore, after dropping the support for Windows XP, this
 * difference can be easily resolved by updating the flag mapping.
 */
void NaClflProtectAndDesiredAccessMap(int prot,
                                      int is_private,
                                      int accmode,
                                      DWORD *out_flMappingProtect,
                                      DWORD *out_flProtect,
                                      DWORD *out_dwDesiredAccess,
                                      char const **out_msg) {
#define M(mp,p,da,err) { mp, p, da, err, #mp, #p, #da }
  static struct {
    DWORD flMappingProtect;
    DWORD flProtect;
    DWORD dwDesiredAccess;
    char const *err;
    char const *flMappingProtect_str;
    char const *flProtect_str;
    char const *dwDesiredAccess_str;
  } table[] = {
    /* RDONLY */
    /* shared */
    /* PROT_NONE */
    M(PAGE_EXECUTE_READ, PAGE_NOACCESS, FILE_MAP_READ, NULL),
    /* PROT_READ */
    M(PAGE_EXECUTE_READ, PAGE_READONLY, FILE_MAP_READ, NULL),
    /* PROT_WRITE */
    M(0, 0, 0, "file open for read only; no shared write allowed"),
    /* PROT_READ | PROT_WRITE */
    M(0, 0, 0, "file open for read only; no shared read/write allowed"),
    /* PROT_EXEC */
    M(PAGE_EXECUTE_WRITECOPY, PAGE_EXECUTE,
      FILE_MAP_READ | FILE_MAP_EXECUTE, NULL),
    /* PROT_READ | PROT_EXEC */
    M(PAGE_EXECUTE_WRITECOPY, PAGE_EXECUTE_READ,
      FILE_MAP_READ | FILE_MAP_EXECUTE, NULL),
    /* PROT_WRITE | PROT_EXEC */
    M(0, 0, 0, "file open for read only; no shared write/exec allowed"),
    /* PROT_READ | PROT_WRITE | PROT_EXEC */
    M(0, 0, 0, "file open for read only; no shared read/write/exec allowed"),

    /* is_private */
    /* PROT_NONE */
    M(PAGE_EXECUTE_READ, PAGE_NOACCESS, FILE_MAP_READ, NULL),
    /* PROT_READ */
    M(PAGE_EXECUTE_READ, PAGE_READONLY, FILE_MAP_READ, NULL),
    /* PROT_WRITE */
    M(PAGE_EXECUTE_READ, PAGE_WRITECOPY, FILE_MAP_COPY, NULL),
    /* PROT_READ | PROT_WRITE */
    M(PAGE_EXECUTE_READ, PAGE_WRITECOPY, FILE_MAP_COPY, NULL),

    /*
     * NB: PAGE_EXECUTE_WRITECOPY is not supported on Server 2003 or
     * XP, which means that the mmap will fail.  In this case we fallback
     * to PAGE_EXECUTE_READ.
     *
     * Even with PAGE_EXECUTE_WRITECOPY, the PROT_WRITE | PROT_EXEC
     * case where we are asking for FILE_MAP_COPY | FILE_MAP_EXECUTE
     * seems to always fail, with GetLastError() yielding 87
     * (ERROR_INVALID_PARAMETER).  This may be due to antivirus.
     */

    /* PROT_EXEC */
    M(PAGE_EXECUTE_WRITECOPY, PAGE_EXECUTE,
      FILE_MAP_READ | FILE_MAP_EXECUTE, NULL),
    /* PROT_READ | PROT_EXEC */
    M(PAGE_EXECUTE_WRITECOPY, PAGE_EXECUTE_READ,
      FILE_MAP_READ | FILE_MAP_EXECUTE, NULL),
    /* PROT_WRITE | PROT_EXEC */
    M(PAGE_EXECUTE_WRITECOPY, PAGE_EXECUTE_WRITECOPY,
      FILE_MAP_COPY | FILE_MAP_EXECUTE, NULL),
    /* PROT_READ | PROT_WRITE | PROT_EXEC */
    M(PAGE_EXECUTE_WRITECOPY, PAGE_EXECUTE_WRITECOPY,
      FILE_MAP_COPY | FILE_MAP_EXECUTE, NULL),

    /* RDWR */
    /* shared */
    /* PROT_NONE */
    M(PAGE_EXECUTE_READWRITE, PAGE_NOACCESS, FILE_MAP_READ, NULL),
    /* PROT_READ */
    M(PAGE_EXECUTE_READWRITE, PAGE_READONLY, FILE_MAP_READ, NULL),
    /* PROT_WRITE */
    M(PAGE_EXECUTE_READWRITE, PAGE_READWRITE, FILE_MAP_WRITE, NULL),
    /* PROT_READ | PROT_WRITE */
    M(PAGE_EXECUTE_READWRITE, PAGE_READWRITE, FILE_MAP_WRITE, NULL),

    /* PROT_EXEC */
    M(PAGE_EXECUTE_READWRITE, PAGE_EXECUTE,
      FILE_MAP_READ | FILE_MAP_EXECUTE, NULL),
    /* PROT_READ | PROT_EXEC */
    M(PAGE_EXECUTE_READWRITE, PAGE_EXECUTE_READ,
      FILE_MAP_READ | FILE_MAP_EXECUTE, NULL),
    /* PROT_WRITE | PROT_EXEC */
    M(PAGE_EXECUTE_READWRITE, PAGE_EXECUTE_READWRITE,
      FILE_MAP_WRITE | FILE_MAP_EXECUTE, NULL),
    /* PROT_READ | PROT_WRITE | PROT_EXEC */
    M(PAGE_EXECUTE_READWRITE, PAGE_EXECUTE_READWRITE,
      FILE_MAP_WRITE | FILE_MAP_EXECUTE, NULL),

    /* is_private */
    /* PROT_NONE */
    M(PAGE_EXECUTE_READWRITE, PAGE_NOACCESS, FILE_MAP_READ, NULL),
    /* PROT_READ */
    M(PAGE_EXECUTE_READWRITE, PAGE_READONLY, FILE_MAP_READ, NULL),
    /* PROT_WRITE */
    M(PAGE_EXECUTE_READWRITE, PAGE_WRITECOPY, FILE_MAP_COPY, NULL),
    /* PROT_READ | PROT_WRITE */
    M(PAGE_EXECUTE_READWRITE, PAGE_WRITECOPY, FILE_MAP_COPY, NULL),

    /* PROT_EXEC */
    M(PAGE_EXECUTE_READWRITE, PAGE_EXECUTE,
      FILE_MAP_WRITE | FILE_MAP_EXECUTE, NULL),
    /* PROT_READ | PROT_EXEC */
    M(PAGE_EXECUTE_READWRITE, PAGE_EXECUTE_READ,
      FILE_MAP_WRITE | FILE_MAP_EXECUTE, NULL),
    /* PROT_WRITE | PROT_EXEC */
    M(PAGE_EXECUTE_READWRITE, PAGE_EXECUTE_WRITECOPY,
      FILE_MAP_WRITE | FILE_MAP_EXECUTE, NULL),
    /* PROT_READ | PROT_WRITE | PROT_EXEC */
    M(PAGE_EXECUTE_READWRITE, PAGE_EXECUTE_WRITECOPY,
      FILE_MAP_WRITE | FILE_MAP_EXECUTE, NULL),
  };
#undef M

  size_t ix;

  NaClLog(3,
          "NaClflProtectAndDesiredAccessMap(prot 0x%x,"
          " priv 0x%x, accmode 0x%x, ...)\n",
          prot, is_private, accmode);

  NACL_COMPILE_TIME_ASSERT(NACL_ABI_O_RDONLY == 0);
  NACL_COMPILE_TIME_ASSERT(NACL_ABI_O_RDWR == 2);
  NACL_COMPILE_TIME_ASSERT(NACL_ABI_PROT_NONE == 0);
  NACL_COMPILE_TIME_ASSERT(NACL_ABI_PROT_READ == 1);
  NACL_COMPILE_TIME_ASSERT(NACL_ABI_PROT_WRITE == 2);
  NACL_COMPILE_TIME_ASSERT(NACL_ABI_PROT_EXEC == 4);

  CHECK(accmode != NACL_ABI_O_WRONLY);

  /*
   * NACL_ABI_O_RDONLY == 0, NACL_ABI_O_RDWR == 2, so multiplying by 8
   * yields a base separation of 8 for the 16 element subtable indexed
   * by the NACL_ABI_PROT_{READ|WRITE|EXEC} and is_private values.
   */
  ix = ((prot & NACL_ABI_PROT_MASK) +
        (is_private << 3) +
        ((accmode & NACL_ABI_O_ACCMODE) << 3));

  CHECK(ix < NACL_ARRAY_SIZE(table));  /* compiler should elide this */

  if (NULL != out_flMappingProtect) {
    *out_flMappingProtect = table[ix].flMappingProtect;
  }
  if (NULL != out_flProtect) {
    *out_flProtect = table[ix].flProtect;
  }
  if (NULL != out_dwDesiredAccess) {
    *out_dwDesiredAccess = table[ix].dwDesiredAccess;
  }
  if (NULL != out_msg) {
    *out_msg = table[ix].err;
  }

  NaClLog(3, "NaClflProtectAndDesiredAccessMap: %s %s %s\n",
          table[ix].flMappingProtect_str,
          table[ix].flProtect_str,
          table[ix].dwDesiredAccess_str);
}

/*
 * Returns flProtect flags for VirtualAlloc'd memory, file based
 * mappings should always use NaClflProtectAndDesiredAccessMap.
 */
DWORD NaClflProtectMap(int prot) {
#define M(p) { p, #p }
  static struct {
    DWORD flProtect;
    char const *flProtect_str;
  } table[] = {
    /* PROT_NONE */
    M(PAGE_NOACCESS),
    /* PROT_READ */
    M(PAGE_READONLY),
    /* PROT_WRITE */
    M(PAGE_READWRITE),
    /* PROT_READ | PROT_WRITE */
    M(PAGE_READWRITE),

    /* PROT_EXEC */
    M(PAGE_EXECUTE),
    /* PROT_READ | PROT_EXEC */
    M(PAGE_EXECUTE_READ),
    /* PROT_WRITE | PROT_EXEC */
    M(PAGE_EXECUTE_READWRITE),
    /* PROT_READ | PROT_WRITE | PROT_EXEC */
    M(PAGE_EXECUTE_READWRITE),
  };
#undef M

  size_t ix;

  NaClLog(3, "NaClflProtectMap(prot 0x%x)\n", prot);

  NACL_COMPILE_TIME_ASSERT(NACL_ABI_PROT_NONE == 0);
  NACL_COMPILE_TIME_ASSERT(NACL_ABI_PROT_READ == 1);
  NACL_COMPILE_TIME_ASSERT(NACL_ABI_PROT_WRITE == 2);
  NACL_COMPILE_TIME_ASSERT(NACL_ABI_PROT_EXEC == 4);

  ix = (prot & NACL_ABI_PROT_MASK);
  CHECK(ix < NACL_ARRAY_SIZE(table));  /* compiler should elide this */

  NaClLog(3, "NaClflProtectMap: %s\n", table[ix].flProtect_str);

  return table[ix].flProtect;
}

/*
 * Unfortunately, when the descriptor is imported via
 * NaClHostDescPosixTake or NaClHostDescPosixDup, the underlying file
 * handle may not have GENERIC_EXECUTE permission associated with it,
 * unlike the files open using NaClHostDescOpen.  This means that the
 * CreateFileMapping with flMappingProtect that specifies PAGE_EXECUTE_*
 * will fail. Since we don't know whether GENERIC_EXECUTE without doing
 * a query, we instead lazily determine the need by detecting the
 * CreateFileMapping error and retrying using a fallback
 * flMappingProtect that does not have EXECUTE rights.  We record this
 * in the descriptor so that the next time we do not have to try with
 * the PAGE_EXECUTE_* and have it deterministically fail.
 *
 * This function is also used when mapping in anonymous memory.  We
 * assume that we never map anonymous executable memory -- mmap of
 * executable data is always from a file, and the page will be
 * non-writable -- and we ensure that anonymous memory is never
 * executable.
 */
static DWORD NaClflProtectRemoveExecute(DWORD flProtect) {
  switch (flProtect) {
    case PAGE_EXECUTE:
      return PAGE_NOACCESS;
    case PAGE_EXECUTE_READ:
      return PAGE_READONLY;
    case PAGE_EXECUTE_READWRITE:
      return PAGE_READWRITE;
    case PAGE_EXECUTE_WRITECOPY:
      return PAGE_WRITECOPY;
  }
  return flProtect;
}

/* Check if flProtect has executable permission. */
static int NaClflProtectHasExecute(DWORD flProtect) {
  return flProtect == PAGE_EXECUTE ||
         flProtect == PAGE_EXECUTE_READ ||
         flProtect == PAGE_EXECUTE_READWRITE ||
         flProtect == PAGE_EXECUTE_WRITECOPY;
}

/*
 * TODO(mseaborn): Reduce duplication between this function and
 * nacl::Map()/NaClMap().
 */
uintptr_t NaClHostDescMap(struct NaClHostDesc *d,
                          struct NaClDescEffector *effp,
                          void                *start_addr,
                          size_t              len,
                          int                 prot,
                          int                 flags,
                          nacl_off64_t        offset) {
  uintptr_t retval;
  uintptr_t addr;
  int       desc_flags;
  HANDLE    hFile;
  HANDLE    hMap;
  int       retry_fallback;
  DWORD     flMappingProtect;
  DWORD     dwDesiredAccess;
  DWORD     flProtect;
  DWORD     flOldProtect;
  char const *err_msg;
  DWORD     dwMaximumSizeHigh;
  DWORD     dwMaximumSizeLow;
  uintptr_t map_result;
  size_t    chunk_offset;
  size_t    chunk_size;

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
  addr = (uintptr_t) start_addr;
  prot &= NACL_ABI_PROT_MASK;

  /*
   * Check that if FIXED, start_addr is not NULL.
   * Use platform free address space locator to set start_addr if NULL and
   * not FIXED.
   */
  if (0 == (flags & NACL_ABI_MAP_FIXED)) {
    /*
     * Not fixed, addr is a hint... which we ignore.  We cannot just
     * let windows pick, since we are mapping memory in chunks of
     * 64-kB to permit piecewise unmapping.
     */
    if (!NaClFindAddressSpace(&addr, len)) {
      NaClLog(LOG_ERROR,
              "NaClHostDescMap: not fixed, and could not find space\n");
      return (uintptr_t) -NACL_ABI_ENOMEM;
    }

    NaClLog(4,
            "NaClHostDescMap: NOT FIXED, found space at %"NACL_PRIxPTR"\n",
            addr);

    start_addr = (void *) addr;
  }

  flProtect = 0;
  dwDesiredAccess = 0;

  if (0 != (flags & NACL_ABI_MAP_ANONYMOUS)) {
    /*
     * anonymous memory must be free'able later via VirtualFree
     */
    NaClLog(3, "NaClHostDescMap: anonymous mapping\n");

    flProtect = NaClflProtectMap(prot & (~PROT_EXEC));
    NaClLog(3, "NaClHostDescMap: flProtect 0x%x\n", flProtect);

    for (chunk_offset = 0;
         chunk_offset < len;
         chunk_offset += NACL_MAP_PAGESIZE) {
      uintptr_t chunk_addr = addr + chunk_offset;

      (*effp->vtbl->UnmapMemory)(effp, chunk_addr, NACL_MAP_PAGESIZE);

      NaClLog(3,
              "NaClHostDescMap: VirtualAlloc(0x%08x,,%x,%x)\n",
              (void *) (addr + chunk_offset),
              MEM_COMMIT | MEM_RESERVE,
              flProtect);
      map_result = (uintptr_t) VirtualAlloc((void *) chunk_addr,
                                            NACL_MAP_PAGESIZE,
                                            MEM_COMMIT | MEM_RESERVE,
                                            flProtect);
      if (map_result != addr + chunk_offset) {
        NaClLog(LOG_FATAL,
                ("Could not VirtualAlloc anonymous memory at"
                 " addr 0x%08x with prot %x\n"),
                addr + chunk_offset, flProtect);
      }
    }
    NaClLog(3, "NaClHostDescMap: (anon) returning 0x%08"NACL_PRIxPTR"\n",
            start_addr);
    return (uintptr_t) start_addr;
  }

  if (NULL == d) {
    desc_flags = NACL_ABI_O_RDWR;
  } else {
    desc_flags = d->flags;
  }

  if (0 != (desc_flags & NACL_ABI_O_APPEND) &&
      0 != (prot & NACL_ABI_PROT_WRITE)) {
    return (uintptr_t) -NACL_ABI_EACCES;
  }
  if (NACL_ABI_O_WRONLY == (desc_flags & NACL_ABI_O_ACCMODE)) {
    return (uintptr_t) -NACL_ABI_EACCES;
  }
  NaClflProtectAndDesiredAccessMap(prot,
                                   0 != (flags & NACL_ABI_MAP_PRIVATE),
                                   (desc_flags & NACL_ABI_O_ACCMODE),
                                   &flMappingProtect,
                                   &flProtect,
                                   &dwDesiredAccess,
                                   &err_msg);
  if (0 == flProtect) {
    NaClLog(3, "NaClHostDescMap: %s\n", err_msg);
    return (uintptr_t) -NACL_ABI_EACCES;
  }
  NaClLog(3,
          "NaClHostDescMap: flMappingProtect 0x%x,"
          " dwDesiredAccess 0x%x, flProtect 0x%x\n",
          flMappingProtect, dwDesiredAccess, flProtect);

  hFile = (HANDLE) _get_osfhandle(d->d);
  dwMaximumSizeLow = 0;
  dwMaximumSizeHigh = 0;

  /*
   * Ensure consistency of the d->flMappingProtect access.
   */
  NaClFastMutexLock(&d->mu);
  if (0 != d->flMappingProtect) {
    flMappingProtect = d->flMappingProtect;
    retry_fallback = 0;
  } else {
    retry_fallback = 1;
  }
  NaClFastMutexUnlock(&d->mu);

  /*
   * Finite retry cycle.  We can fallback from PAGE_EXECUTE_WRITECOPY to
   * PAGE_EXECUTE_READ and from having executable permissions to not having
   * them.
   */
  while (1) {
    /*
     * If hFile is INVALID_HANDLE_VALUE, the memory is backed by the
     * system paging file.  Why does it returns NULL instead of
     * INVALID_HANDLE_VALUE when there is an error?
     */
    hMap = CreateFileMapping(hFile,
                             NULL,
                             flMappingProtect,
                             dwMaximumSizeHigh,
                             dwMaximumSizeLow,
                             NULL);
    if (NULL == hMap && retry_fallback) {
      /*
       * PAGE_EXECUTE_WRITECOPY is not supported on Windows XP so we fallback
       * to PAGE_EXECUTE_READ.
       */
      if (PAGE_EXECUTE_WRITECOPY == flMappingProtect) {
        NaClLog(3,
                "NaClHostDescMap: CreateFileMapping failed, retrying with"
                " PAGE_EXECUTE_READ instead of PAGE_EXECUTE_WRITECOPY\n");
        flMappingProtect = PAGE_EXECUTE_READ;
        continue;
      }
      if (0 == (prot & NACL_ABI_PROT_EXEC) &&
          NaClflProtectHasExecute(flMappingProtect)) {
        NaClLog(3,
                "NaClHostDescMap: CreateFileMapping failed, retrying without"
                " execute permission.  Original flMappingProtect 0x%x\n",
                flMappingProtect);
        NaClflProtectAndDesiredAccessMap(prot & (~PROT_EXEC),
                                         0 != (flags & NACL_ABI_MAP_PRIVATE),
                                         (desc_flags & NACL_ABI_O_ACCMODE),
                                         &flMappingProtect,
                                         &flProtect,
                                         &dwDesiredAccess,
                                         &err_msg);
        if (0 == flProtect) {
          NaClLog(3, "NaClHostDescMap: %s\n", err_msg);
          return (uintptr_t) -NACL_ABI_EACCES;
        }
        flMappingProtect = NaClflProtectRemoveExecute(flMappingProtect);
        NaClLog(3,
                "NaClHostDescMap: fallback flMappingProtect 0x%x,"
                " dwDesiredAccess 0x%x, flProtect 0x%x\n",
                flMappingProtect, dwDesiredAccess, flProtect);
        continue;
      }
      NaClLog(3,
              "NaClHostDescMap: not retrying, since caller explicitly asked"
              " for NACL_ABI_PROT_EXEC\n");
      break;
    }
    /*
     * Remember successful flProtect used.  Note that this just
     * ensures reads of d->flMappingProtect gets a consistent value;
     * we have a potential race where two threads perform mmap and in
     * parallel determine the replacement flProtect value.  This is
     * okay, since those two threads should arrive at the same
     * replacement value.  This could be replaced with an atomic
     * word.
     */
    NaClFastMutexLock(&d->mu);
    d->flMappingProtect = flMappingProtect;
    NaClFastMutexUnlock(&d->mu);
    break;
  }

  if (NULL == hMap) {
    DWORD err = GetLastError();
    NaClLog(LOG_INFO,
            "NaClHostDescMap: CreateFileMapping failed: %d\n",
            err);
    return -NaClXlateSystemError(err);
  }
  NaClLog(3, "NaClHostDescMap: CreateFileMapping got handle %d\n",
          (int)(uintptr_t)hMap);
  NaClLog(3, "NaClHostDescMap: dwDesiredAccess 0x%x\n", dwDesiredAccess);

  retval = (uintptr_t) -NACL_ABI_EINVAL;

  for (chunk_offset = 0;
       chunk_offset < len;
       chunk_offset += NACL_MAP_PAGESIZE) {
    uintptr_t chunk_addr = addr + chunk_offset;
    nacl_off64_t net_offset;
    uint32_t net_offset_high;
    uint32_t net_offset_low;

    (*effp->vtbl->UnmapMemory)(effp, chunk_addr, NACL_MAP_PAGESIZE);

    chunk_size = size_min(len - chunk_offset, NACL_MAP_PAGESIZE);
    /* in case MapViewOfFile cares that we exceed the file size */
    net_offset = offset + chunk_offset;
    net_offset_high = (uint32_t) (net_offset >> 32);
    net_offset_low = (uint32_t) net_offset;
    NaClLog(4,
            "NaClHostDescMap: MapViewOfFileEx(hMap=%d, dwDesiredAccess=0x%x,"
            " net_offset_high = 0x%08x, net_offset_low = 0x%08x,"
            " chunk_size = 0x%"NACL_PRIxS", chunk_addr = 0x%"NACL_PRIxPTR"\n",
            hMap, dwDesiredAccess, net_offset_high, net_offset_low,
            chunk_size, chunk_addr);
    map_result = (uintptr_t) MapViewOfFileEx(hMap,
                                             dwDesiredAccess,
                                             net_offset_high,
                                             net_offset_low,
                                             chunk_size,
                                             (void *) chunk_addr);
    NaClLog(3,
            "NaClHostDescMap: map_result %"NACL_PRIxPTR
            ", chunk_addr %"NACL_PRIxPTR
            ", addr + chunk_offset %"NACL_PRIxPTR"\n",
            map_result, chunk_addr, (addr + chunk_offset));
    if ((addr + chunk_offset) != map_result) {
      /*
       * MapViewOfFileEx() failed.  If we are mapping into untrusted
       * address space, we opened an mmap hole.  We didn't expect the
       * failure, and it's difficult to restore the old mappings that we
       * removed, so for safety we must abort with LOG_FATAL.
       *
       * Otherwise, if this is a trusted mapping, we can return an error
       * gracefully.  NaClElfFileMapSegment() currently triggers errors
       * here by mapping beyond the file's extent: see
       * https://crbug.com/406632.
       */
      int log_type =
          effp == NaClDescEffectorTrustedMem() ? LOG_ERROR : LOG_FATAL;
      DWORD err = GetLastError();
      size_t unmap_offset;
      NaClLog(log_type,
              "MapViewOfFileEx failed at 0x%08"NACL_PRIxPTR
              ", got 0x%08"NACL_PRIxPTR", err %x\n",
              addr + chunk_offset,
              map_result,
              err);
      for (unmap_offset = 0;
           unmap_offset < chunk_offset;
           unmap_offset += NACL_MAP_PAGESIZE) {
        (void) UnmapViewOfFile((void *) (addr + unmap_offset));
      }
      retval = (uintptr_t) -NaClXlateSystemError(err);
      goto cleanup;
    }
    if (!VirtualProtect((void *) map_result,
                        NaClRoundPage(chunk_size),
                        flProtect,
                        &flOldProtect)) {
        DWORD err = GetLastError();
        NaClLog(LOG_INFO,
                "VirtualProtect failed at 0x%08x, err %x\n",
                addr, err);
        retval = (uintptr_t) -NaClXlateSystemError(err);
        goto cleanup;
    }
  }
  retval = (uintptr_t) start_addr;
cleanup:
  (void) CloseHandle(hMap);
  NaClLog(3, "NaClHostDescMap: returning %"NACL_PRIxPTR"\n", retval);
  return retval;
}

void NaClHostDescUnmapUnsafe(void    *start_addr,
                             size_t  len) {
  uintptr_t addr;
  size_t    off;

  addr = (uintptr_t) start_addr;

  for (off = 0; off < len; off += NACL_MAP_PAGESIZE) {
    if (!UnmapViewOfFile((void *) (addr + off))) {
      NaClLog(LOG_FATAL,
              "NaClHostDescUnmapUnsafe: UnmapViewOfFile() failed: "
              "start address 0x%p, length 0x%" NACL_PRIxS
              ", offset 0x%" NACL_PRIxS ", error %d\n",
              start_addr, len, off, GetLastError());
    }
  }
}

static void NaClHostDescCtorIntern(struct NaClHostDesc *hd,
                                   int posix_d,
                                   int flags) {
  nacl_host_stat_t stbuf;

  hd->d = posix_d;
  hd->flags = flags;
  hd->flMappingProtect = 0;
  if (_fstat64(posix_d, &stbuf) != 0) {
    /* inherited non-fstat'able are IPC channels, e.g., for bootstrap channel */
    NaClLog(4,
            "NaClHostDescCtorIntern: could not _fstat64,"
            " assuming non-seekable\n");
    hd->protect_filepos = 0;
  } else {
    int file_type = stbuf.st_mode & S_IFMT;
    /*
     * Inherited stdio are console handles and are not seekable.
     *
     * Posix descriptors (wrapping Windows HANDLES) opened for
     * O_WRONLY | O_APPEND cannot have byte range locks applied, which
     * is how the protect_filepos mechanism is implemented.  Luckily,
     * this is only needed for O_RDWR | O_APPEND or non-append
     * descriptor.
     */
    hd->protect_filepos = (((S_IFREG == file_type) ||
                           (S_IFDIR == file_type)) &&
                           !((flags & NACL_ABI_O_APPEND) != 0 &&
                             (flags & NACL_ABI_O_ACCMODE) ==
                             NACL_ABI_O_WRONLY));
  }
  if (!NaClFastMutexCtor(&hd->mu)) {
    NaClLog(LOG_FATAL, "NaClHostDescCtorIntern: NaClFastMutexCtor failed\n");
  }
}

int NaClHostDescOpen(struct NaClHostDesc  *d,
                     char const           *path,
                     int                  flags,
                     int                  perms) {
  DWORD dwDesiredAccess;
  DWORD dwCreationDisposition;
  DWORD dwFlagsAndAttributes;
  int oflags;
  HANDLE hFile;
  DWORD err;
  int fd;

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
      dwDesiredAccess = GENERIC_READ | GENERIC_EXECUTE;
      oflags = _O_RDONLY | _O_BINARY;
      break;
    case NACL_ABI_O_RDWR:
      oflags = _O_RDWR | _O_BINARY;
      dwDesiredAccess = GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE;
      break;
    case NACL_ABI_O_WRONLY:  /* Enforced in the Read call */
      oflags = _O_WRONLY | _O_BINARY;
      dwDesiredAccess = GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE;
      break;
    default:
      NaClLog(LOG_ERROR,
              "NaClHostDescOpen: bad access flags 0x%x.\n",
              flags);
      return -NACL_ABI_EINVAL;
  }
  /*
   * Possibly make the file read-only.  The file attribute only
   * applies if the file is created; if it pre-exists, the attributes
   * from the file is combined with the FILE_FLAG_* values.
   */
  if (0 == (perms & NACL_ABI_S_IWUSR)) {
    dwFlagsAndAttributes = (FILE_ATTRIBUTE_READONLY |
                            FILE_FLAG_POSIX_SEMANTICS);
  } else {
    dwFlagsAndAttributes = (FILE_ATTRIBUTE_NORMAL |
                            FILE_FLAG_POSIX_SEMANTICS);
  }
  /*
   * Postcondition: flags & NACL_ABI_O_ACCMODE is one of the three
   * allowed values.
   */
  switch (flags & (NACL_ABI_O_CREAT | NACL_ABI_O_TRUNC)) {
    default:
      /*
       * There is no other value possible, but the Windows compiler
       * is too stupid to realize that and thinks that the variable
       * might be used uninitialized.
       */
    case 0:
      dwCreationDisposition = OPEN_EXISTING;
      break;
    case NACL_ABI_O_CREAT:
      if (flags & NACL_ABI_O_EXCL)
        /* Force creation of new file (fails if file already exists) */
        dwCreationDisposition = CREATE_NEW;
      else
        /* Open existing file, or create one if needed. */
        dwCreationDisposition = OPEN_ALWAYS;
      break;
    case NACL_ABI_O_TRUNC:
      /* truncates existing file, but does not create a new one */
      dwCreationDisposition = TRUNCATE_EXISTING;
      break;
    case NACL_ABI_O_CREAT | NACL_ABI_O_TRUNC:
      /* truncates existing file or creates a new one */
      dwCreationDisposition = CREATE_ALWAYS;
      break;
  }
  if (0 != (flags & NACL_ABI_O_APPEND)) {
    oflags |= _O_APPEND;
  }

  NaClLog(1,
          "NaClHostDescOpen: CreateFileA(path=%s, desired_access=0x%x,"
          " share_mode=ALL, security_attributes=NULL, creation_disposition=%d,"
          " flags_and_attributes=%d, template_file=NULL)\n",
          path, dwDesiredAccess, dwCreationDisposition, dwFlagsAndAttributes);

  hFile = CreateFileA(path, dwDesiredAccess,
                      (FILE_SHARE_DELETE |
                       FILE_SHARE_READ |
                       FILE_SHARE_WRITE),
                      NULL,
                      dwCreationDisposition,
                      dwFlagsAndAttributes,
                      NULL);
  if (INVALID_HANDLE_VALUE == hFile) {
    err = GetLastError();
    NaClLog(3, "NaClHostDescOpen: CreateFile failed %d\n", err);
    return -NaClXlateSystemError(err);
  }
  fd = _open_osfhandle((intptr_t) hFile, oflags);
  /*
   * oflags _O_APPEND, _O_RDONLY, and _O_TEXT are meaningful; unclear
   * whether _O_RDWR, _O_WRONLY, etc has any effect.
   */
  if (-1 == fd) {
    NaClLog(LOG_FATAL, "NaClHostDescOpen failed: err %d\n",
            GetLastError());
  }
  NaClHostDescCtorIntern(d, fd, flags);
  return 0;
}

int NaClHostDescPosixDup(struct NaClHostDesc  *d,
                         int                  posix_d,
                         int                  flags) {
  int host_desc;

  NaClLog(3, "NaClHostDescPosixDup(0x%08x, %d, 0%o)\n",
          (uintptr_t) d, posix_d, flags);
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
              "NaClHostDescOpen: bad access flags 0x%x.\n",
              flags);
      return -NACL_ABI_EINVAL;
  }

  host_desc = _dup(posix_d);
  if (-1 == host_desc) {
    return -GetErrno();
  }
  NaClHostDescCtorIntern(d, host_desc, flags);
  return 0;
}

int NaClHostDescPosixTake(struct NaClHostDesc *d,
                          int                 posix_d,
                          int                 flags) {
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
              "NaClHostDescOpen: bad access flags 0x%x.\n",
              flags);
      return -NACL_ABI_EINVAL;
  }
  NaClHostDescCtorIntern(d, posix_d, flags);
  return 0;
}

ssize_t NaClHostDescRead(struct NaClHostDesc  *d,
                         void                 *buf,
                         size_t               len) {
  /* Windows ReadFile only supports DWORD, so we need
   * to clamp the length. */
  unsigned int actual_len;
  HANDLE fh;
  DWORD bytes_received;
  DWORD err;

  if (len < UINT_MAX) {
    actual_len = (unsigned int) len;
  } else {
    actual_len = UINT_MAX;
  }

  NaClHostDescCheckValidity("NaClHostDescRead", d);
  if (NACL_ABI_O_WRONLY == (d->flags & NACL_ABI_O_ACCMODE)) {
    NaClLog(3, "NaClHostDescRead: WRONLY file\n");
    return -NACL_ABI_EBADF;
  }
  /*
   * We drop into using Windows ReadFile rather than using _read from
   * the POSIX compatibility layer here.  The reason for this is
   * because the pread/pwrite implementation uses ReadFile/WriteFile,
   * it would be more consistent with the pread/pwrite implementation
   * to just also use ReadFile/WriteFile directly here as well.
   *
   * NB: contrary to the documentation available on MSDN, operations
   * on synchronous files with non-NULL LPOVERLAPPED arguments result
   * in the *implicit* file position getting updated before the
   * ReadFile/WriteFile returning, rather than the Offset/OffsetHigh
   * members of the explicit OVERLAPPED structure (!).  In order to
   * support mixed read/pread syscall sequences (and similarly mixed
   * write/pwrite sequences) we must effectively lock the file
   * position from access by other threads and then read/write, so
   * that when pread/pwrite mess up the implicit file position
   * temporarily, it would not be visible.
   */
  fh = (HANDLE) _get_osfhandle(d->d);
  CHECK(INVALID_HANDLE_VALUE != fh);

  /*
   * Ensure that we do not corrupt shared implicit file position.
   */
  if (d->protect_filepos) {
    NaClTakeFilePosLock(fh);
  }
  if (!ReadFile(fh, buf, actual_len, &bytes_received, NULL)) {
    err = GetLastError();
    if (ERROR_HANDLE_EOF == err) {
      bytes_received = 0;
    } else {
      NaClLog(4, "NaClHostDescRead: ReadFile error %d\n", err);
      bytes_received = -NaClXlateSystemError(err);
    }
  }
  if (d->protect_filepos) {
    NaClDropFilePosLock(fh);
  }

  return bytes_received;
}

ssize_t NaClHostDescWrite(struct NaClHostDesc *d,
                          void const          *buf,
                          size_t              len) {
  /*
   * Windows WriteFile only supports DWORD uint, so we need to clamp
   * the length.
   */
  unsigned int actual_len;
  HANDLE fh;
  DWORD bytes_written;
  DWORD err;
  OVERLAPPED overlap;
  OVERLAPPED *overlap_ptr = NULL;

  if (NACL_ABI_O_RDONLY == (d->flags & NACL_ABI_O_ACCMODE)) {
    NaClLog(3, "NaClHostDescWrite: RDONLY file\n");
    return -NACL_ABI_EBADF;
  }
  if (len < UINT_MAX) {
    actual_len = (unsigned int) len;
  } else {
    actual_len = UINT_MAX;
  }

  NaClHostDescCheckValidity("NaClHostDescWrite", d);
  /*
   * See discussion in NaClHostDescRead above wrt why we use WriteFile
   * instead of _write below.
   */
  if (0 != (NACL_ABI_O_APPEND & d->flags)) {
    nacl_off64_t offset = GG_LONGLONG(0xffffffffffffffff);
    memset(&overlap, 0, sizeof overlap);
    overlap.Offset = (DWORD) offset;
    overlap.OffsetHigh = (DWORD) (offset >> 32);
    overlap_ptr = &overlap;
  }
  fh = (HANDLE) _get_osfhandle(d->d);
  CHECK(INVALID_HANDLE_VALUE != fh);
  /*
   * Ensure that we do not corrupt shared implicit file position.
   */
  if (d->protect_filepos) {
    NaClTakeFilePosLock(fh);
  }
  if (!WriteFile(fh, buf, actual_len, &bytes_written, overlap_ptr)) {
    err = GetLastError();
    NaClLog(4, "NaClHostDescWrite: WriteFile error %d\n", err);

    bytes_written = -NaClXlateSystemError(err);
  }
  if (d->protect_filepos) {
    NaClDropFilePosLock(fh);
  }

  return bytes_written;
}

nacl_off64_t NaClHostDescSeek(struct NaClHostDesc  *d,
                              nacl_off64_t         offset,
                              int                  whence) {
  HANDLE hFile;
  nacl_off64_t retval;

  NaClHostDescCheckValidity("NaClHostDescSeek", d);
  hFile = (HANDLE) _get_osfhandle(d->d);
  CHECK(INVALID_HANDLE_VALUE != hFile);
  if (d->protect_filepos) {
    NaClTakeFilePosLock(hFile);
  }
  retval = _lseeki64(d->d, offset, whence);
  if (d->protect_filepos) {
    NaClDropFilePosLock(hFile);
  }
  return (-1 == retval) ? -errno : retval;
}

ssize_t NaClHostDescPRead(struct NaClHostDesc *d,
                          void *buf,
                          size_t len,
                          nacl_off64_t offset) {
  HANDLE fh;
  OVERLAPPED overlap;
  DWORD bytes_received;
  DWORD err;
  nacl_off64_t orig_pos = 0;

  NaClHostDescCheckValidity("NaClHostDescPRead", d);
  if (NACL_ABI_O_WRONLY == (d->flags & NACL_ABI_O_ACCMODE)) {
    NaClLog(3, "NaClHostDescPRead: WRONLY file\n");
    return -NACL_ABI_EBADF;
  }
  if (offset < 0) {
    return -NACL_ABI_EINVAL;
  }
  /*
   * There are reports of driver issues that may require clamping len
   * to a megabyte or so, lest ReadFile returns an error with
   * GetLastError() returning ERROR_INVALID_PARAMETER, but since we
   * do not expect to ever read from / write to anything other than
   * filesystem files, we do not clamp.
   */
  fh = (HANDLE) _get_osfhandle(d->d);
  CHECK(INVALID_HANDLE_VALUE != fh);
  memset(&overlap, 0, sizeof overlap);
  overlap.Offset = (DWORD) offset;
  overlap.OffsetHigh = (DWORD) (offset >> 32);
  if (len > UINT_MAX) {
    len = UINT_MAX;
  }
  if (d->protect_filepos) {
    orig_pos = NaClLockAndGetCurrentFilePos(fh);
  }
  if (!ReadFile(fh, buf, (DWORD) len, &bytes_received, &overlap)) {
    err = GetLastError();
    if (ERROR_HANDLE_EOF == err) {
      bytes_received = 0;
      /* handle as if returned true. */
    } else {
      NaClLog(4, "NaClHostDescPRead: ReadFile failed, error %d\n", err);
      NaClSetCurrentFilePosAndUnlock(fh, orig_pos);
      bytes_received = -NaClXlateSystemError(err);
    }
  }
  if (d->protect_filepos) {
    NaClSetCurrentFilePosAndUnlock(fh, orig_pos);
  }
  return bytes_received;
}

ssize_t NaClHostDescPWrite(struct NaClHostDesc *d,
                           void const *buf,
                           size_t len,
                           nacl_off64_t offset) {
  HANDLE fh;
  OVERLAPPED overlap;
  DWORD bytes_sent;
  DWORD err;
  nacl_off64_t orig_pos = 0;

  NaClHostDescCheckValidity("NaClHostDescPWrite", d);
  if (NACL_ABI_O_RDONLY == (d->flags & NACL_ABI_O_ACCMODE)) {
    NaClLog(3, "NaClHostDescPWrite: RDONLY file\n");
    return -NACL_ABI_EBADF;
  }
  if (offset < 0) {
    /*
     * This also avoids the case where having 0xffffffff in both
     * overlap.Offset and overlap.OffsetHigh means append to the file.
     * In Posix, offset does not permit special meanings being encoded
     * like this.
     */
    return -NACL_ABI_EINVAL;
  }
  fh = (HANDLE) _get_osfhandle(d->d);
  CHECK(INVALID_HANDLE_VALUE != fh);
  memset(&overlap, 0, sizeof overlap);
  overlap.Offset = (DWORD) offset;
  overlap.OffsetHigh = (DWORD) (offset >> 32);
  if (len > UINT_MAX) {
    len = UINT_MAX;
  }
  if (d->protect_filepos) {
    orig_pos = NaClLockAndGetCurrentFilePos(fh);
  }
  if (!WriteFile(fh, buf, (DWORD) len, &bytes_sent, &overlap)) {
    err = GetLastError();
    if (ERROR_HANDLE_EOF == err) {
      bytes_sent = 0;
      /* handle as if returned true. */
    } else {
      NaClLog(4,
              "NaClHostDescPWrite: WriteFile failed, error %d\n", err);
      bytes_sent = -NaClXlateSystemError(err);
    }
  }
  if (d->protect_filepos) {
    NaClSetCurrentFilePosAndUnlock(fh, orig_pos);
  }
  return bytes_sent;
}

int NaClHostDescFstat(struct NaClHostDesc   *d,
                      nacl_host_stat_t      *nasp) {
  NaClHostDescCheckValidity("NaClHostDescFstat", d);
  if (NACL_HOST_FSTAT64(d->d, nasp) == -1) {
    return -GetErrno();
  }

  return 0;
}

int NaClHostDescIsatty(struct NaClHostDesc *d) {
  int retval;

  NaClHostDescCheckValidity("NaClHostDescIsatty", d);
  retval = _isatty(d->d);
  /* When windows _isatty fails it returns zero, but does not set errno. */
  return (0 == retval) ? -NACL_ABI_ENOTTY : 1;
}

int NaClHostDescClose(struct NaClHostDesc *d) {
  int retval;

  NaClHostDescCheckValidity("NaClHostDescClose", d);
  if (-1 != d->d) {
    retval = _close(d->d);
    if (-1 == retval) {
      return -GetErrno();
    }
    d->d = -1;
  }
  NaClFastMutexDtor(&d->mu);
  return 0;
}

/*
 * This is not a host descriptor function, but is closely related to
 * fstat and should behave similarly.
 */
int NaClHostDescStat(char const *path, nacl_host_stat_t *nhsp) {
  if (NACL_HOST_STAT64(path, nhsp) == -1) {
    return -GetErrno();
  }

  return 0;
}

int NaClHostDescMkdir(const char *path, int mode) {
  UNREFERENCED_PARAMETER(mode);
  if (_mkdir(path) != 0)
    return -NaClXlateErrno(errno);
  return 0;
}

int NaClHostDescRmdir(const char *path) {
  if (_rmdir(path) != 0)
    return -NaClXlateErrno(errno);
  return 0;
}

int NaClHostDescChdir(const char *path) {
  if (_chdir(path) != 0)
    return -NaClXlateErrno(errno);
  return 0;
}

int NaClHostDescGetcwd(char *path, size_t len) {
  if (_getcwd(path, (int) len) == NULL)
    return -NaClXlateErrno(errno);
  return 0;
}

int NaClHostDescUnlink(const char *path) {
  /*
   * If the file exists and is not writable we make it writeable
   * before calling _unlink() to match the POSIX semantics where
   * unlink(2) can remove readonly files.
   */
  if (_access(path, WIN_F_OK) == 0 && _access(path, WIN_W_OK) != 0) {
    if (_chmod(path, _S_IREAD | S_IWRITE) != 0) {
      /* If _chmod fails just log it and contine on to call _unlink anyway */
      NaClLog(3, "NaClHostDescUnlink: _chmod failed: %d\n", errno);
    }
  }

  if (_unlink(path) != 0)
    return -NaClXlateErrno(errno);

  return 0;
}

static int DoTruncate(HANDLE hfile, nacl_abi_off_t length) {
  LARGE_INTEGER win_length;
  DWORD err;

  win_length.QuadPart = length;
  if (!SetFilePointerEx(hfile, win_length, NULL, FILE_BEGIN)) {
    err = GetLastError();
    NaClLog(LOG_ERROR,
            "NaClHostDescTruncate: SetFilePointerEx failed:"
            " last error %d.\n", err);
    return -NaClXlateSystemError(err);
  }

  if (!SetEndOfFile(hfile)) {
    err = GetLastError();
    NaClLog(LOG_ERROR,
            "NaClHostDescTruncate: could not truncate file:"
            " last error %d.\n", err);
    if (err == ERROR_USER_MAPPED_FILE) {
      NaClLog(LOG_ERROR,
              "NaClHostDescTruncate: this is due to an existing"
              " mapping of the same file.\n");
    }
    return -NaClXlateSystemError(err);
  }

  return 0;
}

int NaClHostDescLstat(char const *path, nacl_host_stat_t *nhsp) {
  /*
   * Since symlinks don't exist on windows, stat() and lstat()
   * are equivalent.
   */
  return NaClHostDescStat(path, nhsp);
}

int NaClHostDescLink(const char *oldpath, const char *newpath) {
  /*
   * Hard linking not implemented for win32
   */
  NaClLog(1, "NaClHostDescLink: hard linking not supported on windows.\n");
  return -NACL_ABI_ENOSYS;
}

int NaClHostDescRename(const char *oldpath, const char *newpath) {
  if (rename(oldpath, newpath) != 0)
    return -NaClXlateErrno(errno);
  return 0;
}

int NaClHostDescSymlink(const char *oldpath, const char *newpath) {
  /*
   * Symlinks are not supported on win32.
   * TODO(smklein): Look into symlink support for windows, and identify if
   * linux-style symlinks can be emulated.
   */
  NaClLog(1, "NaClHostDescSymlink: symbolic links not supported on windows.\n");
  return -NACL_ABI_ENOSYS;
}

int NaClHostDescChmod(const char *path, nacl_abi_mode_t mode) {
  if (_chmod(path, NaClMapMode(mode)) != 0)
    return -NaClXlateErrno(errno);
  return 0;
}

int NaClHostDescAccess(const char *path, int amode) {
  if (_access_s(path, NaClMapAccessMode(amode)) != 0)
    return -NaClXlateErrno(errno);
  return 0;
}

int NaClHostDescReadlink(const char *path, char *buf, size_t bufsize) {
  /*
   * readlink(2) sets errno to EINVAL when the file in question is
   * not a symlink.  Since win32 does not support symlinks we simply
   * return EINVAL in all cases here.
   *
   * Partial implementation exists here:
   * https://chromiumcodereview.appspot.com/24889002/
   * TODO(phosek/smklein): Complete the partial implementation of readlink
   * if basic symlink support arrives on nacl for windows.
   */
  NaClLog(1,
          "NaClHostDescReadlink: symbolic links not supported on Windows.\n");
  return -NACL_ABI_EINVAL;
}

int NaClHostDescUtimes(const char *filename,
                       const struct nacl_abi_timeval *times) {
  struct _utimbuf host_time;
  if (times != NULL) {
    /* Sec + rounded up usec */
    host_time.actime = times[0].nacl_abi_tv_sec +
                       (times[0].nacl_abi_tv_usec >= 500000);
    host_time.modtime = times[1].nacl_abi_tv_sec +
                        (times[1].nacl_abi_tv_usec >= 500000);
  }
  if (_utime(filename, (times != NULL ? &host_time : NULL)) == -1) {
    return -NaClXlateErrno(errno);
  }
  return 0;
}

int NaClHostDescFcntl(struct NaClHostDesc *d, int cmd, long arg) {
  NaClHostDescCheckValidity("NaClHostDescFcntl", d);
  NaClLog(1,
          "NaClHostDescFcntl: Fcntl not yet supported on Windows.\n");
  return -NACL_ABI_EINVAL;
}

int NaClHostDescFchmod(struct NaClHostDesc *d, nacl_abi_mode_t mode) {
  NaClHostDescCheckValidity("NaClHostDescFchmod", d);
  /*
   * TODO(smklein): Use the following
   * https://msdn.microsoft.com/en-us/library/ks2530z6.aspx
   * and
   * https://msdn.microsoft.com/en-us/library/aa366789(VS.85).aspx
   * to get the path. Then, call chmod.
   */
  NaClLog(1,
          "NaClHostDescFchmod: Fchmod not yet supported on Windows.\n");
  return -NACL_ABI_EINVAL;
}

int NaClHostDescFsync(struct NaClHostDesc *d) {
  HANDLE hFile;
  DWORD err;

  NaClHostDescCheckValidity("NaClHostDescFsync", d);

  hFile = (HANDLE) _get_osfhandle(d->d);
  CHECK(INVALID_HANDLE_VALUE != hFile);

  if (!FlushFileBuffers(hFile)) {
    err = GetLastError();
    return -NaClXlateSystemError(err);
  }

  return 0;
}

int NaClHostDescFdatasync(struct NaClHostDesc *d) {
  HANDLE hFile;
  DWORD err;

  NaClHostDescCheckValidity("NaClHostDescFdatasync", d);

  hFile = (HANDLE) _get_osfhandle(d->d);
  CHECK(INVALID_HANDLE_VALUE != hFile);

  if (!FlushFileBuffers(hFile)) {
    err = GetLastError();
    return -NaClXlateSystemError(err);
  }

  return 0;
}

int NaClHostDescFtruncate(struct NaClHostDesc *d, nacl_off64_t length) {
  NaClHostDescCheckValidity("NaClHostDescFtruncate", d);

  return DoTruncate((HANDLE) _get_osfhandle(d->d), length);
}

int NaClHostDescTruncate(char const *path, nacl_abi_off_t length) {
  int retval;

  HANDLE hfile = CreateFileA(path,
      GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
      NULL,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_POSIX_SEMANTICS,
      NULL);

  if (INVALID_HANDLE_VALUE == hfile) {
    DWORD err = GetLastError();
    NaClLog(3, "NaClHostDescTruncate: CreateFileA failed %d\n", err);
    return -NaClXlateSystemError(err);
  }

  retval = DoTruncate(hfile, length);

  if (!CloseHandle(hfile)) {
    DWORD err = GetLastError();
    NaClLog(3, "NaClHostDescTruncate: CloseHandle failed %d\n", err);
    if (retval == 0)
      retval = -NaClXlateSystemError(err);
  }

  return retval;
}
