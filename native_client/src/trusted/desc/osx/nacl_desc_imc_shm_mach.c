/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Transferrable Mach-based shared memory objects.
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_platform.h"

#include <mach/mach_vm.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/desc/osx/nacl_desc_imc_shm_mach.h"

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_find_addrsp.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"

#include "native_client/src/trusted/platform_qualify/nacl_os_qualify.h"

#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"
#include "native_client/src/trusted/service_runtime/internal_errno.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

/*
 * This file contains the implementation of the NaClDescImcShmMach
 * subclass of NaClDesc.
 *
 * NaClDescImcShmMach is the subclass that wraps Mach-based IMC shm descriptors.
 */

static mach_port_t NaClCreateMachMemoryObject(size_t length, int executable) {
  if (0 == length) {
    return MACH_PORT_NULL;
  }

  int mach_flags = MAP_MEM_NAMED_CREATE | VM_PROT_READ | VM_PROT_WRITE |
                   (executable ? VM_PROT_EXECUTE : 0);
  mach_port_t named_right;
  memory_object_size_t length_copy = length;
  kern_return_t kr =
      mach_make_memory_entry_64(mach_task_self(), &length_copy, 0, mach_flags,
                                &named_right, MACH_PORT_NULL);
  if (kr != KERN_SUCCESS) {
    return MACH_PORT_NULL;
  }

  return named_right;
}

static void *NaClMachMap(void *start, size_t length, int prot,
                         mach_port_t handle, off_t offset, int fixed) {
  /*
   * The flag VM_FLAGS_OVERWRITE isn't available before OSX 10.7.0.
   * https://code.google.com/p/chromium/issues/detail?id=547246#c8
   */
  CHECK(NaClOSX10Dot7OrLater());

  int copy = FALSE;
  int mach_flags =
      fixed ? VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE : VM_FLAGS_ANYWHERE;

  static const int kMachProt[] = {
      VM_PROT_NONE,
      VM_PROT_READ,
      VM_PROT_WRITE,
      VM_PROT_READ | VM_PROT_WRITE,
      VM_PROT_EXECUTE,
      VM_PROT_READ | VM_PROT_EXECUTE,
      VM_PROT_WRITE | VM_PROT_EXECUTE,
      VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE};

  /* Allow page permissions to be increased later. */
  int max_prot = VM_PROT_ALL | VM_PROT_IS_MASK;
  mach_vm_address_t address = (mach_vm_address_t) start;
  kern_return_t kr =
      mach_vm_map(mach_task_self(), &address, length, 0,
                  mach_flags, handle, offset, copy, kMachProt[prot & 7],
                  max_prot, VM_INHERIT_NONE);

  /*
   * On 32-bit architectures, this forces a down-cast.
   */
  uintptr_t address_downcasted = (uintptr_t) address;
  return kr == KERN_SUCCESS ? (void *) address_downcasted
                            : NACL_ABI_MAP_FAILED;
}

static void NaClMachClose(mach_port_t handle) {
  if (mach_port_deallocate(mach_task_self(), handle) != KERN_SUCCESS) {
    NaClLog(LOG_FATAL, "Failed to deallocate Mach port.\n");
  }
}

static struct NaClDescVtbl const kNaClDescImcShmMachVtbl; /* fwd */

static int NaClDescImcShmMachSubclassCtor(struct NaClDescImcShmMach *self,
                                          mach_port_t h, nacl_off64_t size) {
  struct NaClDesc *basep = (struct NaClDesc *) self;

  /*
   * off_t is signed, but size_t are not; historically size_t is for
   * sizeof and similar, and off_t is also used for stat structure
   * st_size member.  This runtime test detects large object sizes
   * that are silently converted to negative values.
   */
  if (size < 0 || SIZE_T_MAX < (uint64_t) size) {
    return 0;
  }
  self->h = h;
  self->size = size;
  basep->base.vtbl = (struct NaClRefCountVtbl const *) &kNaClDescImcShmMachVtbl;
  return 1;
}

static int NaClDescImcShmMachCtor(struct NaClDescImcShmMach *self,
                                  mach_port_t h, nacl_off64_t size) {
  struct NaClDesc *basep = (struct NaClDesc *) self;
  int rv;

  basep->base.vtbl = (struct NaClRefCountVtbl const *) NULL;

  if (!NaClDescCtor(basep)) {
    return 0;
  }
  rv = NaClDescImcShmMachSubclassCtor(self, h, size);
  if (!rv) {
    /* NaClDescImcShmMach construction failed, still a NaClDesc object */
    (*NACL_VTBL(NaClRefCount, basep)->Dtor)((struct NaClRefCount *) basep);
  }
  (*NACL_VTBL(NaClDesc, basep)->SetFlags)(basep, NACL_ABI_O_RDWR);
  return 1;
}

int NaClDescImcShmMachAllocCtor(struct NaClDescImcShmMach *self,
                                nacl_off64_t size, int executable) {
  mach_port_t h;
  int rv;

  if (size < 0 || SIZE_T_MAX < (uint64_t) size) {
    NaClLog(4, "NaClDescImcShmMachAllocCtor: requested size 0x%08" NACL_PRIx64
               " (0x%08" NACL_PRId64 ") too large\n",
            size, size);
    return 0;
  }
  h = NaClCreateMachMemoryObject((size_t) size, executable);
  if (MACH_PORT_NULL == h) {
    return 0;
  }
  if (0 == (rv = NaClDescImcShmMachCtor(self, h, size))) {
    NaClMachClose(h);
  }
  return rv;
}

struct NaClDesc *NaClDescImcShmMachMake(mach_port_t handle,
                                        nacl_off64_t size) {
  struct NaClDescImcShmMach *desc = malloc(sizeof(*desc));
  if (NULL == desc) {
    return NULL;
  }
  if (!NaClDescImcShmMachCtor(desc, handle, size)) {
    free(desc);
    return NULL;
  }
  return &desc->base;
}

static void NaClDescImcShmMachDtor(struct NaClRefCount *vself) {
  struct NaClDescImcShmMach *self = (struct NaClDescImcShmMach *) vself;

  NaClMachClose(self->h);
  self->h = MACH_PORT_NULL;
  vself->vtbl = (struct NaClRefCountVtbl const *) &kNaClDescVtbl;
  (*vself->vtbl->Dtor)(vself);
}

static uintptr_t NaClDescImcShmMachMap(struct NaClDesc *vself,
                                       struct NaClDescEffector *effp,
                                       void *start_addr, size_t len, int prot,
                                       int flags, nacl_off64_t offset) {
  UNREFERENCED_PARAMETER(effp);
  struct NaClDescImcShmMach *self = (struct NaClDescImcShmMach *) vself;

  void *result;
  nacl_off64_t tmp_off64;

  /*
   * shm must have NACL_ABI_MAP_SHARED in flags, and all calls through
   * this API must supply a start_addr, so NACL_ABI_MAP_FIXED is
   * assumed.
   */
  if (NACL_ABI_MAP_SHARED != (flags & NACL_ABI_MAP_SHARING_MASK)) {
    NaClLog(LOG_INFO, ("NaClDescImcShmMachMap: Mapping not NACL_ABI_MAP_SHARED,"
                       " flags 0x%x\n"),
            flags);
    return -NACL_ABI_EINVAL;
  }

  /*
   * prot must not contain bits other than PROT_{READ|WRITE|EXEC}.
   */
  if (0 != (~(NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE | NACL_ABI_PROT_EXEC) &
            prot)) {
    NaClLog(LOG_INFO,
            "NaClDescImcShmMachMap: prot has other bits than"
            " PROT_{READ|WRITE|EXEC}\n");
    return -NACL_ABI_EINVAL;
  }

  tmp_off64 = offset + len;
  /* just NaClRoundAllocPage, but in 64 bits */
  tmp_off64 = ((tmp_off64 + NACL_MAP_PAGESIZE - 1) &
               ~(uint64_t) (NACL_MAP_PAGESIZE - 1));
  if (tmp_off64 > INT32_MAX) {
    NaClLog(LOG_INFO, "NaClDescImcShmMachMap: total offset exceeds 32-bits\n");
    return -NACL_ABI_EOVERFLOW;
  }

  result = NaClMachMap(start_addr, len, prot, self->h, offset,
                       (flags & NACL_ABI_MAP_FIXED) != 0);
  if (NACL_ABI_MAP_FAILED == result) {
    return -NACL_ABI_E_MOVE_ADDRESS_SPACE;
  }
  if ((flags & NACL_ABI_MAP_FIXED) != 0 && result != start_addr) {
    NaClLog(
        LOG_FATAL,
        "NaClDescImcShmMachMap: NACL_ABI_MAP_FIXED but got %p instead of %p\n",
        result, start_addr);
  }
  return (uintptr_t) result;
}

static int NaClDescImcShmMachFstat(struct NaClDesc *vself,
                                   struct nacl_abi_stat *stbp) {
  struct NaClDescImcShmMach *self = (struct NaClDescImcShmMach *) vself;

  if (self->size > INT32_MAX) {
    return -NACL_ABI_EOVERFLOW;
  }

  stbp->nacl_abi_st_dev = 0;
  stbp->nacl_abi_st_ino = NACL_FAKE_INODE_NUM;
  stbp->nacl_abi_st_mode =
      (NACL_ABI_S_IFSHM | NACL_ABI_S_IRUSR | NACL_ABI_S_IWUSR);
  stbp->nacl_abi_st_nlink = 1;
  stbp->nacl_abi_st_uid = -1;
  stbp->nacl_abi_st_gid = -1;
  stbp->nacl_abi_st_rdev = 0;
  stbp->nacl_abi_st_size = (nacl_abi_off_t) self->size;
  stbp->nacl_abi_st_blksize = 0;
  stbp->nacl_abi_st_blocks = 0;
  stbp->nacl_abi_st_atime = 0;
  stbp->nacl_abi_st_mtime = 0;
  stbp->nacl_abi_st_ctime = 0;

  return 0;
}

static struct NaClDescVtbl const kNaClDescImcShmMachVtbl = {
    {
        NaClDescImcShmMachDtor,
    },
    NaClDescImcShmMachMap,
    NaClDescReadNotImplemented,
    NaClDescWriteNotImplemented,
    NaClDescSeekNotImplemented,
    NaClDescPReadNotImplemented,
    NaClDescPWriteNotImplemented,
    NaClDescImcShmMachFstat,
    NaClDescFchdirNotImplemented,
    NaClDescFchmodNotImplemented,
    NaClDescFsyncNotImplemented,
    NaClDescFdatasyncNotImplemented,
    NaClDescFtruncateNotImplemented,
    NaClDescGetdentsNotImplemented,
    NaClDescExternalizeSizeNotImplemented,
    NaClDescExternalizeNotImplemented,
    NaClDescLockNotImplemented,
    NaClDescTryLockNotImplemented,
    NaClDescUnlockNotImplemented,
    NaClDescWaitNotImplemented,
    NaClDescTimedWaitAbsNotImplemented,
    NaClDescSignalNotImplemented,
    NaClDescBroadcastNotImplemented,
    NaClDescSendMsgNotImplemented,
    NaClDescRecvMsgNotImplemented,
    NaClDescLowLevelSendMsgNotImplemented,
    NaClDescLowLevelRecvMsgNotImplemented,
    NaClDescConnectAddrNotImplemented,
    NaClDescAcceptConnNotImplemented,
    NaClDescPostNotImplemented,
    NaClDescSemWaitNotImplemented,
    NaClDescGetValueNotImplemented,
    NaClDescSetMetadata,
    NaClDescGetMetadata,
    NaClDescSetFlags,
    NaClDescGetFlags,
    NaClDescIsattyNotImplemented,
    NACL_DESC_SHM_MACH,
};
