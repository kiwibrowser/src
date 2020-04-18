/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * NaCl inter-module communication primitives.
 *
 * This file implements common parts of IMC for "UNIX like systems" (i.e. not
 * used on Windows).
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "native_client/src/include/build_config.h"

#if NACL_ANDROID
#include <linux/ashmem.h>
#endif

#include <algorithm>

#include "native_client/src/include/atomic_ops.h"

#include "native_client/src/shared/imc/nacl_imc_c.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/service_runtime/include/bits/mman.h"


/*
 * The pathname or SHM-namespace prefixes for memory objects created
 * by CreateMemoryObject().
 */
static const char kShmOpenPrefix[] = "/google-nacl-shm-";
/*
 * Default path to use for temporary shared memory files when not using
 * shm_open. This is only used if TMPDIR is not found in the environment.
 */
static const char kDefaultTmp[] = "/tmp";

static NaClCreateMemoryObjectFunc g_create_memory_object_func = NULL;


/* Duplicate a file descriptor. */
NaClHandle NaClDuplicateNaClHandle(NaClHandle handle) {
  return dup(handle);
}

void NaClSetCreateMemoryObjectFunc(NaClCreateMemoryObjectFunc func) {
  g_create_memory_object_func = func;
}

#if NACL_ANDROID
#define ASHMEM_DEVICE "/dev/ashmem"

static int AshmemCreateRegion(size_t size) {
  int fd;

  fd = open(ASHMEM_DEVICE, O_RDWR);
  if (fd < 0)
    return -1;

  if (ioctl(fd, ASHMEM_SET_SIZE, size) < 0) {
    close(fd);
    return -1;
  }

  return fd;
}
#endif

int NaClWouldBlock(void) {
  return errno == EAGAIN;
}

#if !NACL_ANDROID
static Atomic32 memory_object_count = 0;

static const char *GetTempDir() {
  const char *tmpenv = getenv("TMPDIR");
  if (tmpenv)
    return tmpenv;
  return kDefaultTmp;
}

static int TryShmOrTempOpen(size_t length, bool use_temp) {
  char name[PATH_MAX];
  char tmpname[PATH_MAX];
  const char *prefix = kShmOpenPrefix;
  if (0 == length) {
    return -1;
  }

  if (use_temp) {
    snprintf(tmpname, sizeof tmpname, "%s%s", GetTempDir(), prefix);
    prefix = tmpname;
  }

  for (;;) {
    int m;
    snprintf(name, sizeof name, "%s-%u.%u", prefix,
             getpid(),
             (int) AtomicIncrement(&memory_object_count, 1));
    if (use_temp) {
      m = open(name, O_RDWR | O_CREAT | O_EXCL, 0);
    } else {
      /*
       * Using 0 for the mode causes shm_unlink to fail with EACCES on Mac
       * OS X 10.8. As of 10.8, the kernel requires the user to have write
       * permission to successfully shm_unlink.
       */
      m = shm_open(name, O_RDWR | O_CREAT | O_EXCL, S_IWUSR);
    }
    if (0 <= m) {
      if (use_temp) {
        int rc = unlink(name);
        DCHECK(rc == 0);
      } else {
        int rc = shm_unlink(name);
        DCHECK(rc == 0);
      }
      if (ftruncate(m, length) == -1) {
        close(m);
        m = -1;
      }
      return m;
    }
    if (errno != EEXIST) {
      return -1;
    }
    /* Retry only if we got EEXIST. */
  }
}

#if NACL_LINUX
/*
 * Attempt to set PROT_EXEC on memory mapped from a shm_open fd, and return
 * true if this is successful, false otherwise.  On many linux installations
 * /dev/shm is mounted with 'noexec' which causes this to fail.
 */
static bool DetermineDevShmExecutable() {
  size_t pagesize = sysconf(_SC_PAGESIZE);
  int fd = TryShmOrTempOpen(pagesize, false);
  if (fd < 0)
    return false;

  bool result = false;
  void *mapping = mmap(NULL, pagesize, PROT_READ, MAP_SHARED, fd, 0);
  if (mapping != MAP_FAILED) {
    if (mprotect(mapping, pagesize, PROT_READ | PROT_EXEC) == 0)
      result = true;
    CHECK(munmap(mapping, pagesize) == 0);
  }
  CHECK(close(fd) == 0);
  return result;
}
#endif
#endif

NaClHandle NaClCreateMemoryObject(size_t length, int executable) {
  int fd;

  if (0 == length) {
    return -1;
  }

  if (g_create_memory_object_func != NULL) {
    fd = g_create_memory_object_func(length, executable);
    if (fd >= 0)
      return fd;
  }

#if NACL_ANDROID
  return AshmemCreateRegion(length);
#else
  bool use_shm_open = true;
  if (executable) {
    /*
     * On Mac OS X, shm_open() gives us file descriptors that the OS
     * won't mmap() with PROT_EXEC, which is no good for the dynamic
     * code region, so we use open() in $TMPDIR instead.
     */
    if (NACL_OSX)
      use_shm_open = false;
#if NACL_LINUX
    /*
     * On Linux this depends on how the system is configured (usually the
     * presence of noexec on the /dev/shm mount point) so we need to determine
     * this via am empirical test.
     */
    static bool s_dev_shm_executable = DetermineDevShmExecutable();
    if (!s_dev_shm_executable) {
      NaClLog(1, "NaClCreateMemoryObjectFunc: PROT_EXEC not supported by "
                 "shm_open(), falling back to /tmp for shared exectuable "
                 "memory.\n");
    }
    use_shm_open = s_dev_shm_executable;
#endif
  }

  return TryShmOrTempOpen(length, !use_shm_open);
#endif  /* !NACL_ANDROID */
}

void* NaClMap(struct NaClDescEffector* effp,
              void* start, size_t length, int prot, int flags,
              NaClHandle memory, off_t offset) {
  static const int kPosixProt[] = {
    PROT_NONE,
    PROT_READ,
    PROT_WRITE,
    PROT_READ | PROT_WRITE,
    PROT_EXEC,
    PROT_READ | PROT_EXEC,
    PROT_WRITE | PROT_EXEC,
    PROT_READ | PROT_WRITE | PROT_EXEC
  };
  int adjusted = 0;
  UNREFERENCED_PARAMETER(effp);

  if (flags & NACL_ABI_MAP_SHARED) {
    adjusted |= MAP_SHARED;
  }
  if (flags & NACL_ABI_MAP_PRIVATE) {
    adjusted |= MAP_PRIVATE;
  }
  if (flags & NACL_ABI_MAP_FIXED) {
    adjusted |= MAP_FIXED;
  }
  return mmap(start, length, kPosixProt[prot & 7], adjusted, memory, offset);
}
