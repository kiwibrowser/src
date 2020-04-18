/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>

#include "native_client/src/include/build_config.h"

#if NACL_LINUX
# include <sys/mman.h>
#elif NACL_OSX
# include <mach/mach.h>
# include <mach/mach_vm.h>
#endif

#include "gtest/gtest.h"

#include "native_client/src/include/portability_io.h"
#include "native_client/src/trusted/desc/nacl_desc_effector_trusted_mem.h"
#include "native_client/src/trusted/desc/nacl_desc_imc_shm.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/desc/nrd_all_modules.h"
#include "native_client/src/trusted/platform_qualify/nacl_os_qualify.h"
#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/service_runtime/mmap_test_check.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/sel_addrspace.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sys_memory.h"

#if NACL_OSX
#include "native_client/src/trusted/desc/osx/nacl_desc_imc_shm_mach.h"
#endif

static const int kTestFillByte = 0x42;

class MmapTest : public testing::Test {
 protected:
  virtual void SetUp();
  virtual void TearDown();
};

void MmapTest::SetUp() {
  NaClNrdAllModulesInit();
}

void MmapTest::TearDown() {
  NaClNrdAllModulesFini();
}

#if NACL_OSX
// OS X changes the flag used to indicate that mapped memory is shared based on
// its version.
static int SharedMapFlag() {
  return NaClOSX10Dot10OrLater() ? SM_TRUESHARED : SM_SHARED;
}
#endif

// Creates a temporary file and returns an FD for it.
static int MakeTempFileFd() {
  int host_fd;
#if NACL_WINDOWS
  // Open temporary file that is deleted automatically.
  const char *temp_prefix = "nacl_mmap_test_temp_";
  char *temp_filename = _tempnam("C:\\Windows\\Temp", temp_prefix);
  EXPECT_EQ(_sopen_s(&host_fd, temp_filename,
                     _O_RDWR | _O_CREAT | _O_TEMPORARY,
                     _SH_DENYNO, _S_IREAD | _S_IWRITE), 0);
#else
  char temp_filename[] = "/tmp/nacl_mmap_test_temp_XXXXXX";
  host_fd = mkstemp(temp_filename);
  EXPECT_GE(host_fd, 0);
#endif
  EXPECT_EQ(remove(temp_filename), 0);
  return host_fd;
}

// Creates a temporary file of the given size and returns a NaClDesc for it.
static struct NaClDesc *MakeTempFileNaClDesc(size_t file_size) {
  int host_fd = MakeTempFileFd();

  struct NaClHostDesc *host_desc =
      (struct NaClHostDesc *) malloc(sizeof(*host_desc));
  EXPECT_TRUE(host_desc);
  EXPECT_EQ(NaClHostDescPosixTake(host_desc, host_fd, NACL_ABI_O_RDWR), 0);
  struct NaClDesc *desc = (struct NaClDesc *) NaClDescIoDescMake(host_desc);

  // Fill the file.  On Windows, this is necessary to ensure that NaCl
  // gives us mappings from the file instead of PROT_NONE-fill.
  char *buf = new char[file_size];
  memset(buf, kTestFillByte, file_size);
  ssize_t written = NACL_VTBL(NaClDesc, desc)->Write(desc, buf, file_size);
  EXPECT_EQ(written, (ssize_t) file_size);
  delete[] buf;

  return desc;
}

// These tests are extremely slow in debug builds on Windows 8 (About ten
// minutes, vs. a few seconds otherwise.) Release builds should be enough.
// See http://crbug.com/452222.
#if !NACL_WINDOWS || defined(NDEBUG) || NACL_BUILD_SUBARCH != 64

// These tests are disabled for ARM/MIPS because the ARM/MIPS sandboxes are
// zero-based, and sel_addrspace_(arm/mips).c do not work when allocating a
// non-zero-based region.
// TODO(mseaborn): Change sel_addrspace_arm.c to work with this test.
// However, for now, testing the Linux memory mapping code under
// x86-32 and x86-64 gives us good coverage of the Linux code in
// general.
#if NACL_ARCH(NACL_BUILD_ARCH) != NACL_arm && \
    NACL_ARCH(NACL_BUILD_ARCH) != NACL_mips

// Check that the untrusted address space really gets freed by trying
// to allocate and free a sandbox multiple times.  On a 32-bit system,
// this would fail if NaClAddrSpaceFree() were a no-op because it
// would run out of host address space.
TEST_F(MmapTest, TestFreeingAddressSpace) {
  // We use a small number of iterations because address space
  // allocation can be very slow on Windows (on the order of 1 second
  // on the 32-bit Windows bots).
  for (int count = 0; count < 4; count++) {
    struct NaClApp app;
    ASSERT_EQ(NaClAppCtor(&app), 1);
    ASSERT_EQ(NaClAllocAddrSpace(&app), LOAD_OK);
    NaClAddrSpaceFree(&app);
  }
}

void MapShmFd(struct NaClApp *nap, uintptr_t addr, size_t shm_size) {
  struct NaClDescImcShm *shm_desc =
      (struct NaClDescImcShm *) malloc(sizeof(*shm_desc));
  ASSERT_TRUE(shm_desc);
  ASSERT_EQ(NaClDescImcShmAllocCtor(shm_desc, shm_size,
                                    /* executable= */ 0), 1);
  struct NaClDesc *desc = &shm_desc->base;
  int fd = NaClAppSetDescAvail(nap, desc);

  uintptr_t mapping_addr = (uint32_t) NaClSysMmapIntern(
      nap, (void *) addr, shm_size,
      NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
      NACL_ABI_MAP_FIXED | NACL_ABI_MAP_SHARED, fd, 0);
  ASSERT_EQ(mapping_addr, addr);
}

void MapFileFd(struct NaClApp *nap, uintptr_t addr, size_t file_size) {
  struct NaClDesc *desc = MakeTempFileNaClDesc(file_size);
  int fd = NaClAppSetDescAvail(nap, desc);

  uintptr_t mapping_addr = (uint32_t) NaClSysMmapIntern(
      nap, (void *) addr, file_size,
      NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
      NACL_ABI_MAP_FIXED | NACL_ABI_MAP_SHARED, fd, 0);
  ASSERT_EQ(mapping_addr, addr);
}

#if NACL_OSX
static void MapShmMach(struct NaClApp *nap, uintptr_t addr, size_t shm_size) {
  struct NaClDescImcShmMach *shm_desc =
      (struct NaClDescImcShmMach *) malloc(sizeof(*shm_desc));
  ASSERT_TRUE(shm_desc);
  ASSERT_EQ(NaClDescImcShmMachAllocCtor(shm_desc, shm_size,
                                        /* executable= */ 0), 1);
  struct NaClDesc *desc = &shm_desc->base;
  int fd = NaClAppSetDescAvail(nap, desc);

  uintptr_t mapping_addr = (uint32_t) NaClSysMmapIntern(
      nap, (void *) addr, shm_size,
      NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
      NACL_ABI_MAP_FIXED | NACL_ABI_MAP_SHARED, fd, 0);
  ASSERT_EQ(mapping_addr, addr);
}
#endif

void UnmapMemory(struct NaClApp *nap, uint32_t addr, uint32_t size) {
  // Create dummy NaClAppThread.
  // TODO(mseaborn): Clean up so that this is not necessary.
  struct NaClAppThread thread;
  memset(&thread, 0xff, sizeof(thread));
  thread.nap = nap;

  ASSERT_EQ(NaClSysMunmap(&thread, addr, size), 0);

  uintptr_t sysaddr = NaClUserToSys(nap, addr);
#if NACL_WINDOWS
  CheckMapping(sysaddr, size, MEM_RESERVE, 0, MEM_PRIVATE);
#elif NACL_LINUX
  CheckMapping(sysaddr, size, PROT_NONE, MAP_PRIVATE);
#elif NACL_OSX
  CheckMapping(sysaddr, size, VM_PROT_NONE, SM_EMPTY);
#else
# error Unsupported platform
#endif
}

// Test that shared memory mappings can be unmapped by
// NaClAddrSpaceFree().  These are different from private memory
// mappings because, on Windows, they must be freed with
// UnmapViewOfFile() rather than VirtualFree().
TEST_F(MmapTest, TestFreeingShmMappings) {
  struct NaClApp app;
  ASSERT_EQ(NaClAppCtor(&app), 1);
  ASSERT_EQ(NaClAllocAddrSpace(&app), LOAD_OK);

  // Create a shared memory descriptor of arbitrary size and map it at
  // an arbitrary address.
  MapShmFd(&app, 0x200000, 0x100000);

  // Check that we can deallocate the address space.
  NaClAddrSpaceFree(&app);
}

#if NACL_OSX
// Test that Mach shared memory mappings can be unmapped by
// NaClAddrSpaceFree().
TEST_F(MmapTest, TestFreeingShmMachMappings) {
  // Mach shared memory is only implemented on OSX 10.7+.
  if (!NaClOSX10Dot7OrLater())
    return;

  struct NaClApp app;
  ASSERT_EQ(NaClAppCtor(&app), 1);
  ASSERT_EQ(NaClAllocAddrSpace(&app), LOAD_OK);

  // Create a shared memory descriptor of arbitrary size and map it at
  // an arbitrary address.
  MapShmMach(&app, 0x200000, 0x100000);

  // Check that we can deallocate the address space.
  NaClAddrSpaceFree(&app);
}
#endif /* NACL_OSX */

// Test that unmapping a shared memory mapping does not leave behind
// an address space hole.
TEST_F(MmapTest, TestUnmapShmMapping) {
  struct NaClApp app;
  ASSERT_EQ(NaClAppCtor(&app), 1);
  ASSERT_EQ(NaClAllocAddrSpace(&app), LOAD_OK);

  // sel_mem.c does not like having an empty memory map, so create a
  // dummy entry that we do not later remove.
  // TODO(mseaborn): Clean up so that this is not necessary.
  MapShmFd(&app, 0x400000, 0x10000);

  uint32_t addr = 0x200000;
  uint32_t size = 0x100000;
  MapShmFd(&app, addr, size);

  uintptr_t sysaddr = NaClUserToSys(&app, addr);
#if NACL_WINDOWS
  CheckMapping(sysaddr, size, MEM_COMMIT, PAGE_READWRITE, MEM_MAPPED);
#elif NACL_LINUX
  CheckMapping(sysaddr, size, PROT_READ | PROT_WRITE, MAP_SHARED);
#elif NACL_OSX
  CheckMapping(sysaddr, size, VM_PROT_READ | VM_PROT_WRITE, SharedMapFlag());
#else
# error Unsupported platform
#endif

  UnmapMemory(&app, addr, size);

  NaClAddrSpaceFree(&app);
}

#if NACL_OSX
// Test that unmapping a Mach shared memory mapping does not leave behind
// an address space hole.
TEST_F(MmapTest, TestUnmapShmMachMappings) {
  // Mach shared memory is only implemented on OSX 10.7+.
  if (!NaClOSX10Dot7OrLater())
    return;

  struct NaClApp app;
  ASSERT_EQ(NaClAppCtor(&app), 1);
  ASSERT_EQ(NaClAllocAddrSpace(&app), LOAD_OK);

  // sel_mem.c does not like having an empty memory map, so create a
  // dummy entry that we do not later remove.
  // TODO(mseaborn): Clean up so that this is not necessary.
  MapShmMach(&app, 0x400000, 0x10000);

  uintptr_t addr = 0x200000;
  uint32_t size = 0x100000;
  MapShmMach(&app, addr, size);

  uintptr_t sysaddr = NaClUserToSys(&app, addr);
  CheckMapping(sysaddr, size, VM_PROT_READ | VM_PROT_WRITE, SharedMapFlag());

  UnmapMemory(&app, addr, size);

  // Check that we can deallocate the address space.
  NaClAddrSpaceFree(&app);
}
#endif /* NACL_OSX */

// Test that unmapping a file mapping does not leave behind an address
// space hole.
TEST_F(MmapTest, TestUnmapFileMapping) {
  struct NaClApp app;
  ASSERT_EQ(NaClAppCtor(&app), 1);
  ASSERT_EQ(NaClAllocAddrSpace(&app), LOAD_OK);

  // sel_mem.c does not like having an empty memory map, so create a
  // dummy entry that we do not later remove.
  // TODO(mseaborn): Clean up so that this is not necessary.
  MapShmFd(&app, 0x400000, 0x10000);

  uint32_t addr = 0x200000;
  uint32_t size = 0x100000;
  MapFileFd(&app, addr, size);

  uintptr_t sysaddr = NaClUserToSys(&app, addr);
#if NACL_WINDOWS
  CheckMapping(sysaddr, size, MEM_COMMIT, PAGE_READWRITE, MEM_MAPPED);
#elif NACL_LINUX
  CheckMapping(sysaddr, size, PROT_READ | PROT_WRITE, MAP_SHARED);
#elif NACL_OSX
  CheckMapping(sysaddr, size, VM_PROT_READ | VM_PROT_WRITE, SM_PRIVATE);
#else
# error Unsupported platform
#endif

  UnmapMemory(&app, addr, size);

  NaClAddrSpaceFree(&app);
}

// Test we can map an anonymous memory mapping and that unmapping it
// does not leave behind an address space hole.
TEST_F(MmapTest, TestUnmapAnonymousMemoryMapping) {
  struct NaClApp app;
  ASSERT_EQ(NaClAppCtor(&app), 1);
  ASSERT_EQ(NaClAllocAddrSpace(&app), LOAD_OK);

  // sel_mem.c does not like having an empty memory map, so create a
  // dummy entry that we do not later remove.
  // TODO(mseaborn): Clean up so that this is not necessary.
  MapShmFd(&app, 0x400000, 0x10000);

  // Create an anonymous memory mapping.
  uint32_t addr = 0x200000;
  uint32_t size = 0x100000;
  uintptr_t mapping_addr = (uint32_t) NaClSysMmapIntern(
      &app, (void *) (uintptr_t) addr, size,
      NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
      NACL_ABI_MAP_FIXED | NACL_ABI_MAP_PRIVATE | NACL_ABI_MAP_ANONYMOUS,
      -1, 0);
  ASSERT_EQ(mapping_addr, addr);

  uintptr_t sysaddr = NaClUserToSys(&app, addr);
#if NACL_WINDOWS
  CheckMapping(sysaddr, size, MEM_COMMIT, PAGE_READWRITE, MEM_PRIVATE);
#elif NACL_LINUX
  CheckMapping(sysaddr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE);
#elif NACL_OSX
  CheckMapping(sysaddr, size, VM_PROT_READ | VM_PROT_WRITE, SM_EMPTY);
#else
# error Unsupported platform
#endif

  UnmapMemory(&app, addr, size);

  NaClAddrSpaceFree(&app);
}

// Test that changing a protection of a shared memory mapping is reflected
// by the underlying OS memory mapping.
TEST_F(MmapTest, TestProtectShmMapping) {
  struct NaClApp app;
  ASSERT_EQ(NaClAppCtor(&app), 1);
  ASSERT_EQ(NaClAllocAddrSpace(&app), LOAD_OK);

  // sel_mem.c does not like having an empty memory map, so create a
  // dummy entry that we do not later remove.
  // TODO(mseaborn): Clean up so that this is not necessary.
  MapShmFd(&app, 0x400000, 0x10000);

  uintptr_t addr = 0x200000;
  size_t size = 0x100000;
  MapShmFd(&app, addr, size);

  uintptr_t sysaddr = NaClUserToSys(&app, addr);
#if NACL_WINDOWS
  CheckMapping(sysaddr, size, MEM_COMMIT, PAGE_READWRITE, MEM_MAPPED);
#elif NACL_LINUX
  CheckMapping(sysaddr, size, PROT_READ | PROT_WRITE, MAP_SHARED);
#elif NACL_OSX
  CheckMapping(sysaddr, size, VM_PROT_READ | VM_PROT_WRITE, SharedMapFlag());
#else
# error Unsupported platform
#endif

  ASSERT_EQ(0, NaClSysMprotectInternal(
                   &app, (uint32_t) addr, size, NACL_ABI_PROT_NONE));

#if NACL_WINDOWS
  CheckMapping(sysaddr, size, MEM_COMMIT, PAGE_NOACCESS, MEM_MAPPED);
#elif NACL_LINUX
  CheckMapping(sysaddr, size, PROT_NONE, MAP_SHARED);
#elif NACL_OSX
  CheckMapping(sysaddr, size, VM_PROT_NONE, SharedMapFlag());
#else
# error Unsupported platform
#endif

  NaClAddrSpaceFree(&app);
}

#if NACL_OSX
// Test that changing a protection of a Mach shared memory mapping is reflected
// by the underlying OS memory mapping.
TEST_F(MmapTest, TestProtectShmMachMapping) {
  // Mach shared memory is only implemented on OSX 10.7+.
  if (!NaClOSX10Dot7OrLater())
    return;

  struct NaClApp app;
  ASSERT_EQ(NaClAppCtor(&app), 1);
  ASSERT_EQ(NaClAllocAddrSpace(&app), LOAD_OK);

  // sel_mem.c does not like having an empty memory map, so create a
  // dummy entry that we do not later remove.
  // TODO(mseaborn): Clean up so that this is not necessary.
  MapShmMach(&app, 0x400000, 0x10000);

  uintptr_t addr = 0x200000;
  size_t size = 0x100000;
  MapShmMach(&app, addr, size);
  uintptr_t sysaddr = NaClUserToSys(&app, addr);
  CheckMapping(sysaddr, size, VM_PROT_READ | VM_PROT_WRITE, SharedMapFlag());

  ASSERT_EQ(0, NaClSysMprotectInternal(
                   &app, (uint32_t) addr, size, NACL_ABI_PROT_NONE));
  CheckMapping(sysaddr, size, VM_PROT_NONE, SharedMapFlag());

  NaClAddrSpaceFree(&app);
}
#endif /* NACL_OSX */

// Test that changing a protection of a file mapping is reflected
// by the underlying OS memory mapping.
TEST_F(MmapTest, TestProtectFileMapping) {
  struct NaClApp app;
  ASSERT_EQ(NaClAppCtor(&app), 1);
  ASSERT_EQ(NaClAllocAddrSpace(&app), LOAD_OK);

  // sel_mem.c does not like having an empty memory map, so create a
  // dummy entry that we do not later remove.
  // TODO(mseaborn): Clean up so that this is not necessary.
  MapShmFd(&app, 0x400000, 0x10000);

  uintptr_t addr = 0x200000;
  size_t size = 0x100000;
  MapFileFd(&app, addr, size);

  uintptr_t sysaddr = NaClUserToSys(&app, addr);
#if NACL_WINDOWS
  CheckMapping(sysaddr, size, MEM_COMMIT, PAGE_READWRITE, MEM_MAPPED);
#elif NACL_LINUX
  CheckMapping(sysaddr, size, PROT_READ | PROT_WRITE, MAP_SHARED);
#elif NACL_OSX
  CheckMapping(sysaddr, size, VM_PROT_READ | VM_PROT_WRITE, SM_PRIVATE);
#else
# error Unsupported platform
#endif

  ASSERT_EQ(0, NaClSysMprotectInternal(
                   &app, (uint32_t) addr, size, NACL_ABI_PROT_NONE));

#if NACL_WINDOWS
  CheckMapping(sysaddr, size, MEM_COMMIT, PAGE_NOACCESS, MEM_MAPPED);
#elif NACL_LINUX
  CheckMapping(sysaddr, size, PROT_NONE, MAP_SHARED);
#elif NACL_OSX
  CheckMapping(sysaddr, size, VM_PROT_NONE, SM_PRIVATE);
#else
# error Unsupported platform
#endif

  NaClAddrSpaceFree(&app);
}

// Test that changing a protection of a anonymous memory mapping is
// reflected by the underlying OS memory mapping.
TEST_F(MmapTest, TestProtectAnonymousMemory) {
  struct NaClApp app;
  ASSERT_EQ(NaClAppCtor(&app), 1);
  ASSERT_EQ(NaClAllocAddrSpace(&app), LOAD_OK);

  // sel_mem.c does not like having an empty memory map, so create a
  // dummy entry that we do not later remove.
  // TODO(mseaborn): Clean up so that this is not necessary.
  MapShmFd(&app, 0x400000, 0x10000);

  // Create an anonymous memory mapping.
  uintptr_t addr = 0x200000;
  size_t size = 0x100000;
  uintptr_t mapping_addr = (uint32_t) NaClSysMmapIntern(
      &app, (void *) addr, size,
      NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
      NACL_ABI_MAP_FIXED | NACL_ABI_MAP_PRIVATE | NACL_ABI_MAP_ANONYMOUS,
      -1, 0);
  ASSERT_EQ(mapping_addr, addr);

  uintptr_t sysaddr = NaClUserToSys(&app, addr);
#if NACL_WINDOWS
  CheckMapping(sysaddr, size, MEM_COMMIT, PAGE_READWRITE, MEM_PRIVATE);
#elif NACL_LINUX
  CheckMapping(sysaddr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE);
#elif NACL_OSX
  CheckMapping(sysaddr, size, VM_PROT_READ | VM_PROT_WRITE, SM_EMPTY);
#else
# error Unsupported platform
#endif

  ASSERT_EQ(0, NaClSysMprotectInternal(
                   &app, (uint32_t) addr, size, NACL_ABI_PROT_NONE));

#if NACL_WINDOWS
  CheckMapping(sysaddr, size, MEM_COMMIT, PAGE_NOACCESS, MEM_PRIVATE);
#elif NACL_LINUX
  CheckMapping(sysaddr, size, PROT_NONE, MAP_PRIVATE);
#elif NACL_OSX
  CheckMapping(sysaddr, size, VM_PROT_NONE, SM_EMPTY);
#else
# error Unsupported platform
#endif

  NaClAddrSpaceFree(&app);
}

#endif /* NACL_ARCH(NACL_BUILD_ARCH) != NACL_arm */

static void AssertArrayFilled(const char *array, size_t size) {
  for (size_t i = 0; i < size; ++i)
    EXPECT_EQ(array[i], kTestFillByte);
}

TEST_F(MmapTest, TestTrustedMapBeyondFileExtent) {
  int file_size = 0x10099;
  int file_size_rounded_up = 0x20000;
  int file_offset = 0;
  struct NaClDesc *desc = MakeTempFileNaClDesc(file_size);
  uintptr_t map_result;

  // Map() should work with a non-page-aligned size.
  map_result = NACL_VTBL(NaClDesc, desc)->Map(
      desc,
      NaClDescEffectorTrustedMem(),
      (void *) NULL,
      file_size,
      NACL_ABI_PROT_READ,
      NACL_ABI_MAP_PRIVATE,
      file_offset);
  ASSERT_FALSE(NaClPtrIsNegErrno(&map_result));
  AssertArrayFilled((char *) map_result, file_size);
  NaClHostDescUnmapUnsafe((void *) map_result, file_size);

  // Check the behaviour of Map() when given a rounded-up (page-aligned)
  // size that goes beyond the file's extent.
  map_result = NACL_VTBL(NaClDesc, desc)->Map(
      desc,
      NaClDescEffectorTrustedMem(),
      (void *) NULL,
      file_size_rounded_up,
      NACL_ABI_PROT_READ,
      NACL_ABI_MAP_PRIVATE,
      file_offset);
  if (NACL_WINDOWS) {
    // On Windows, using a size beyond the file's extent is not allowed,
    // even within a page, so we expect an error.
    //
    // This tests that we get a graceful error rather than a LOG_FATAL.
    // This is a partial regression test for https://crbug.com/406632:
    // NaClElfFileMapSegment() currently depends on the error being
    // graceful.
    ASSERT_TRUE(NaClPtrIsNegErrno(&map_result));
    ASSERT_EQ(-(intptr_t) map_result, NACL_ABI_EACCES);
  } else {
    // On Unix, the Map() should succeed.
    ASSERT_FALSE(NaClPtrIsNegErrno(&map_result));
    AssertArrayFilled((char *) map_result, file_size);
    NaClHostDescUnmapUnsafe((void *) map_result, file_size_rounded_up);
  }
}

#if NACL_OSX
// Test that Mach shared memory mappings can be unmapped by
// NaClHostDescUnmapUnsafe(), which uses munmap().
TEST_F(MmapTest, TestPosixUnmapShmMachMappings) {
  // Mach shared memory is only implemented on OSX 10.7+.
  if (!NaClOSX10Dot7OrLater())
    return;

  struct NaClApp app;
  ASSERT_EQ(NaClAppCtor(&app), 1);
  ASSERT_EQ(NaClAllocAddrSpace(&app), LOAD_OK);

  // Create a shared memory descriptor of arbitrary size and map it at
  // an arbitrary address.
  uintptr_t addr = 0x200000;
  uint32_t size = 0x100000;

  // Make a desc.
  struct NaClDescImcShmMach *shm_desc =
      (struct NaClDescImcShmMach *) malloc(sizeof(*shm_desc));
  ASSERT_TRUE(shm_desc);
  ASSERT_EQ(NaClDescImcShmMachAllocCtor(shm_desc, size,
                                        /* executable= */ 0),
            1);
  struct NaClDesc *desc = &shm_desc->base;
  int fd = NaClAppSetDescAvail(&app, desc);

  // Map it at a fixed location.
  uintptr_t user_addr = (uint32_t) NaClSysMmapIntern(
      &app, (void *) addr, size,
      NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
      NACL_ABI_MAP_FIXED | NACL_ABI_MAP_SHARED, fd, 0);
  ASSERT_EQ(user_addr, (uintptr_t) addr);

  uintptr_t sys_addr = NaClUserToSys(&app, user_addr);

  // Unmap the memory, leaving a hole.
  NaClHostDescUnmapUnsafe((void *) sys_addr, size);

  // Check that we can map the memory with VM_FLAGS_FIXED but without
  // VM_FLAGS_OVERWRITE. If the unmap failed, then this call will fail as well.
  mach_vm_address_t sys_addr_copy = (mach_vm_address_t) sys_addr;
  kern_return_t kr = mach_vm_map(mach_task_self(), &sys_addr_copy, size, 0,
                                 VM_FLAGS_FIXED, MACH_PORT_NULL, 0, FALSE,
                                 VM_PROT_READ, VM_PROT_READ, VM_INHERIT_NONE);
  ASSERT_EQ(KERN_SUCCESS, kr);
  NaClAddrSpaceFree(&app);
}
#endif /* NACL_OSX */

#endif /* !NACL_WINDOWS || defined(NDEBUG) ||
          NACL_ARCH(NACL_BUILD_SUBARCH) != 64 */
