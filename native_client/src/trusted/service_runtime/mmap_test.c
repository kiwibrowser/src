/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"

#include <stdio.h>
#include <string.h>
#if NACL_LINUX
# include <sys/mman.h>
#elif NACL_OSX
# include <mach/mach.h>
#endif

#include "native_client/src/shared/platform/nacl_log.h"

#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/trusted/service_runtime/load_file.h"
#include "native_client/src/trusted/service_runtime/mmap_test_check.h"
#include "native_client/src/trusted/service_runtime/nacl_all_modules.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/sel_addrspace.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sys_memory.h"

/*
 * Perform some minimal initialisation of the NaClAppThread based on
 * what we need for the test.  Reusing NaClAppThreadMake() here is
 * difficult because it launches an untrusted thread.
 */
static void InitThread(struct NaClApp *nap, struct NaClAppThread *natp) {
  memset(natp, 0xff, sizeof(*natp));

  natp->nap = nap;
}

void CheckLowerMappings(struct NaClVmmap *mem_map) {
  ASSERT(mem_map->nvalid >= 4);
  /* Zero page. */
  ASSERT_EQ(mem_map->vmentry[0]->prot, NACL_ABI_PROT_NONE);
  /* Trampolines and static code. */
  ASSERT_EQ(mem_map->vmentry[1]->prot,
            NACL_ABI_PROT_READ | NACL_ABI_PROT_EXEC);
  /* Read-only data segment. */
  ASSERT_EQ(mem_map->vmentry[2]->prot, NACL_ABI_PROT_READ);
  /* Writable data segment. */
  ASSERT_EQ(mem_map->vmentry[3]->prot,
            NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE);
}

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64
void CheckForGuardRegion(uintptr_t addr, size_t expected_size) {
#if NACL_WINDOWS
  CheckGuardMapping(addr, expected_size, MEM_RESERVE, 0, MEM_PRIVATE);
#elif NACL_LINUX
  CheckMapping(addr, expected_size, PROT_NONE, MAP_PRIVATE);
#elif NACL_OSX
  CheckMapping(addr, expected_size, VM_PROT_NONE, SM_EMPTY);
#else
# error "Unrecognized OS"
#endif
}
#endif

int main(int argc, char **argv) {
  char *nacl_file;
  struct NaClApp state;
  struct NaClApp *nap = &state;
  struct NaClAppThread nat, *natp = &nat;
  int errcode;
  uint32_t initial_addr;
  uint32_t addr;
  struct NaClVmmap *mem_map;
  struct NaClVmmapEntry *ent;
  char *nacl_verbosity = getenv("NACLVERBOSITY");

  NaClHandleBootstrapArgs(&argc, &argv);

  if (argc < 2) {
    printf("No nexe file!\n\nFAIL\n");
  }
  nacl_file = argv[1];

  NaClAllModulesInit();

  NaClLogSetVerbosity((NULL == nacl_verbosity)
                      ? 0
                      : strtol(nacl_verbosity, (char **) 0, 0));

  errcode = NaClAppCtor(&state);
  ASSERT_NE(errcode, 0);
  errcode = NaClAppLoadFileFromFilename(nap, nacl_file);
  ASSERT_EQ(errcode, LOAD_OK);

  InitThread(&state, natp);

  /*
   * Initial mappings:
   * 0. --  Zero page
   * 1. rx  Static code segment
   * 2. r   Read-only data segment
   * 3. rw  Writable data segment
   * 4. rw  Stack
   * There is no dynamic code area in this case.
   */
  /* Check the initial mappings. */
  mem_map = &state.mem_map;
  ASSERT_EQ(mem_map->nvalid, 5);
  CheckLowerMappings(mem_map);

  /* Allocate range */
  addr = NaClSysMmapIntern(nap, 0, 3 * NACL_MAP_PAGESIZE,
                           NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
                           NACL_ABI_MAP_ANONYMOUS | NACL_ABI_MAP_PRIVATE,
                           -1, 0);
  printf("addr=0x%"NACL_PRIx32"\n", addr);
  initial_addr = addr;
  /*
   * The mappings have changed to become:
   * 0. --  Zero page
   * 1. rx  Static code segment
   * 2. r   Read-only data segment
   * 3. rw  Writable data segment
   * 4. rw  mmap()'d anonymous, 3 pages (new)
   * 5. rw  Stack
   */

  /* Map to overwrite the start of the previously allocated range */
  addr = NaClSysMmapIntern(nap, (void *) (uintptr_t) initial_addr,
                           2 * NACL_MAP_PAGESIZE,
                           NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
                           NACL_ABI_MAP_ANONYMOUS | NACL_ABI_MAP_PRIVATE
                           | NACL_ABI_MAP_FIXED,
                           -1, 0);
  printf("addr=0x%"NACL_PRIx32"\n", addr);
  ASSERT_EQ(addr, initial_addr);
  /*
   * The mappings have changed to become:
   * 0. --  Zero page
   * 1. rx  Static code segment
   * 2. r   Read-only data segment
   * 3. rw  Writable data segment
   * 4. rw  mmap()'d anonymous, 2 pages (new)
   * 5. rw  mmap()'d anonymous, 1 pages (previous)
   * 6. rw  Stack
   */

  /* Allocate new page */
  addr = NaClSysMmapIntern(nap, 0, NACL_MAP_PAGESIZE,
                           NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
                           NACL_ABI_MAP_ANONYMOUS | NACL_ABI_MAP_PRIVATE,
                           -1, 0);
  printf("addr=0x%"NACL_PRIx32"\n", addr);
  /*
   * Our allocation strategy is to scan down from stack.  This is an
   * implementation detail and not part of the guaranteed semantics,
   * but it is good to test that what we expect of our implementation
   * didn't change.
   */
  ASSERT_EQ_MSG(addr, initial_addr - NACL_MAP_PAGESIZE,
                "Allocation strategy changed!");
  /*
   * The mappings have changed to become:
   * 0. --  Zero page
   * 1. rx  Static code segment
   * 2. r   Read-only data segment
   * 3. rw  Writable data segment
   * 4. rw  mmap()'d anonymous, 1 pages (new)
   * 5. rw  mmap()'d anonymous, 2 pages
   * 6. rw  mmap()'d anonymous, 1 pages
   * 7. rw  Stack
   */

  NaClVmmapMakeSorted(mem_map);
  ASSERT_EQ(mem_map->nvalid, 8);
  CheckLowerMappings(mem_map);
  NaClVmmapDebug(mem_map, "After allocations");
  /* Skip mappings 0, 1, 2 and 3. */
  ASSERT_EQ(mem_map->vmentry[4]->page_num,
            (initial_addr - NACL_MAP_PAGESIZE) >> NACL_PAGESHIFT);
  ASSERT_EQ(mem_map->vmentry[4]->npages,
            NACL_PAGES_PER_MAP);

  ASSERT_EQ(mem_map->vmentry[5]->page_num,
            initial_addr >> NACL_PAGESHIFT);
  ASSERT_EQ(mem_map->vmentry[5]->npages,
            2 * NACL_PAGES_PER_MAP);

  ASSERT_EQ(mem_map->vmentry[6]->page_num,
            (initial_addr +  2 * NACL_MAP_PAGESIZE) >> NACL_PAGESHIFT);
  ASSERT_EQ(mem_map->vmentry[6]->npages,
            NACL_PAGES_PER_MAP);

  /*
   * Undo effects of previous mmaps
   */
  errcode = NaClSysMunmap(natp,
                          initial_addr - NACL_MAP_PAGESIZE,
                          NACL_MAP_PAGESIZE * 4);
  ASSERT_EQ(errcode, 0);
  /*
   * Mappings return to being:
   * 0. --  Zero page
   * 1. rx  Static code segment
   * 2. r   Read-only data segment
   * 3. rw  Writable data segment
   * 4. rw  Stack
   */
  ASSERT_EQ(mem_map->nvalid, 5);
  CheckLowerMappings(mem_map);


  /* Allocate range */
  addr = NaClSysMmapIntern(nap, 0, 9 * NACL_MAP_PAGESIZE,
                           NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
                           NACL_ABI_MAP_ANONYMOUS | NACL_ABI_MAP_PRIVATE,
                           -1, 0);
  printf("addr=0x%"NACL_PRIx32"\n", addr);
  initial_addr = addr;
  /*
   * The mappings have changed to become:
   * 0. --  Zero page
   * 1. rx  Static code segment
   * 2. r   Read-only data segment
   * 3. rw  Writable data segment
   * 4. rw  mmap()'d anonymous, 9 pages (new)
   * 5. rw  Stack
   */

  /* Map into middle of previously allocated range */
  addr = NaClSysMmapIntern(nap,
                           (void *) (uintptr_t) (initial_addr
                                                 + 2 * NACL_MAP_PAGESIZE),
                           3 * NACL_MAP_PAGESIZE,
                           NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
                           NACL_ABI_MAP_ANONYMOUS | NACL_ABI_MAP_PRIVATE
                           | NACL_ABI_MAP_FIXED,
                           -1, 0);
  printf("addr=0x%"NACL_PRIx32"\n", addr);
  ASSERT_EQ(addr, initial_addr + NACL_MAP_PAGESIZE * 2);
  /*
   * The mappings have changed to become:
   * 0. --  Zero page
   * 1. rx  Static code segment
   * 2. r   Read-only data segment
   * 3. rw  Writable data segment
   * 4. rw  mmap()'d anonymous, 2 pages (previous)
   * 5. rw  mmap()'d anonymous, 3 pages (new)
   * 6. rw  mmap()'d anonymous, 4 pages (previous)
   * 7. rw  Stack
   */

  NaClVmmapMakeSorted(mem_map);
  ASSERT_EQ(mem_map->nvalid, 8);
  CheckLowerMappings(mem_map);

  ASSERT_EQ(mem_map->vmentry[4]->page_num,
            initial_addr >> NACL_PAGESHIFT);
  ASSERT_EQ(mem_map->vmentry[4]->npages,
            2 * NACL_PAGES_PER_MAP);

  ASSERT_EQ(mem_map->vmentry[5]->page_num,
            (initial_addr + 2 * NACL_MAP_PAGESIZE) >> NACL_PAGESHIFT);
  ASSERT_EQ(mem_map->vmentry[5]->npages,
            3 * NACL_PAGES_PER_MAP);

  ASSERT_EQ(mem_map->vmentry[6]->page_num,
            (initial_addr + 5 * NACL_MAP_PAGESIZE) >> NACL_PAGESHIFT);
  ASSERT_EQ(mem_map->vmentry[6]->npages,
            4 * NACL_PAGES_PER_MAP);


  /* Change the memory protection of previously allocated range */
  errcode = NaClSysMprotectInternal(nap, (initial_addr
                                          + 1 * NACL_MAP_PAGESIZE),
                                    5 * NACL_MAP_PAGESIZE,
                                    NACL_ABI_PROT_READ);
  ASSERT_EQ(errcode, 0);
  /*
   * The mappings have changed to become:
   * 0. --  Zero page
   * 1. rx  Static code segment
   * 2. r   Read-only data segment
   * 3. rw  Writable data segment
   * 4. rw  mmap()'d anonymous, 1 pages (previous)
   * 5. r   mmap()'d anonymous, 1 pages (new)
   * 6. r   mmap()'d anonymous, 3 pages (new)
   * 7. r   mmap()'d anonymous, 1 pages (new)
   * 8. rw  mmap()'d anonymous, 3 pages (previous)
   * 9. rw  Stack
   */

  NaClVmmapMakeSorted(mem_map);
  ASSERT_EQ(mem_map->nvalid, 10);
  CheckLowerMappings(mem_map);

  ASSERT_EQ(mem_map->vmentry[4]->npages,
            1 * NACL_PAGES_PER_MAP);
  ASSERT_EQ(mem_map->vmentry[4]->prot,
            NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE);

  ASSERT_EQ(mem_map->vmentry[5]->npages,
            1 * NACL_PAGES_PER_MAP);
  ASSERT_EQ(mem_map->vmentry[5]->prot,
            NACL_ABI_PROT_READ);

  ASSERT_EQ(mem_map->vmentry[6]->npages,
            3 * NACL_PAGES_PER_MAP);
  ASSERT_EQ(mem_map->vmentry[6]->prot,
            NACL_ABI_PROT_READ);

  ASSERT_EQ(mem_map->vmentry[7]->npages,
            1 * NACL_PAGES_PER_MAP);
  ASSERT_EQ(mem_map->vmentry[7]->prot,
            NACL_ABI_PROT_READ);

  ASSERT_EQ(mem_map->vmentry[8]->npages,
            3 * NACL_PAGES_PER_MAP);
  ASSERT_EQ(mem_map->vmentry[8]->prot,
            NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE);


  /* Change the memory protection of previously allocated range */
  errcode = NaClSysMprotectInternal(nap, (initial_addr
                                          + 2 * NACL_MAP_PAGESIZE),
                                    3 * NACL_MAP_PAGESIZE,
                                    NACL_ABI_PROT_NONE);
  ASSERT_EQ(errcode, 0);
  /*
   * The mappings have changed to become:
   * 0. --  Zero page
   * 1. rx  Static code segment
   * 2. r   Read-only data segment
   * 3. rw  Writable data segment
   * 4. rw  mmap()'d anonymous, 1 pages (previous)
   * 5. r   mmap()'d anonymous, 1 pages (previous)
   * 6. --  mmap()'d anonymous, 3 pages (new)
   * 7. r   mmap()'d anonymous, 1 pages (previous)
   * 8. rw  mmap()'d anonymous, 3 pages (previous)
   * 9. rw  Stack
   */

  NaClVmmapMakeSorted(mem_map);
  ASSERT_EQ(mem_map->nvalid, 10);
  CheckLowerMappings(mem_map);

  ASSERT_EQ(mem_map->vmentry[4]->npages,
            1 * NACL_PAGES_PER_MAP);
  ASSERT_EQ(mem_map->vmentry[4]->prot,
            NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE);

  ASSERT_EQ(mem_map->vmentry[5]->npages,
            1 * NACL_PAGES_PER_MAP);
  ASSERT_EQ(mem_map->vmentry[5]->prot,
            NACL_ABI_PROT_READ);

  ASSERT_EQ(mem_map->vmentry[6]->npages,
            3 * NACL_PAGES_PER_MAP);
  ASSERT_EQ(mem_map->vmentry[6]->prot,
            NACL_ABI_PROT_NONE);

  ASSERT_EQ(mem_map->vmentry[7]->npages,
            1 * NACL_PAGES_PER_MAP);
  ASSERT_EQ(mem_map->vmentry[7]->prot,
            NACL_ABI_PROT_READ);

  ASSERT_EQ(mem_map->vmentry[8]->npages,
            3 * NACL_PAGES_PER_MAP);
  ASSERT_EQ(mem_map->vmentry[8]->prot,
            NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE);


  /*
   * Undo effects of previous mmaps
   */
  errcode = NaClSysMunmap(natp, initial_addr, 9 * NACL_MAP_PAGESIZE);
  ASSERT_EQ(errcode, 0);
  ASSERT_EQ(mem_map->nvalid, 5);
  CheckLowerMappings(mem_map);
  /*
   * Mappings return to being:
   * 0. --  Zero page
   * 1. rx  Static code segment
   * 2. r   Read-only data segment
   * 3. rw  Writable data segment
   * 4. rw  Stack
   */

  /*
   * Check use of hint.
   */
  addr = NaClSysMmapIntern(nap, (void *) (uintptr_t) initial_addr,
                           NACL_MAP_PAGESIZE,
                           NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
                           NACL_ABI_MAP_ANONYMOUS | NACL_ABI_MAP_PRIVATE,
                           -1, 0);
  ASSERT_LE(addr, 0xffff0000u);
  printf("addr=0x%"NACL_PRIx32"\n", addr);
  ASSERT_LE_MSG(initial_addr, addr, "returned address not at or above hint");
  errcode = NaClSysMunmap(natp, addr, NACL_MAP_PAGESIZE);
  ASSERT_EQ(errcode, 0);

  /* Check handling of zero-sized mappings. */
  addr = NaClSysMmapIntern(nap, 0, 0,
                           NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
                           NACL_ABI_MAP_ANONYMOUS | NACL_ABI_MAP_PRIVATE,
                           -1, 0);
  ASSERT_EQ((int) addr, -NACL_ABI_EINVAL);

  errcode = NaClSysMunmap(natp, initial_addr, 0);
  ASSERT_EQ(errcode, -NACL_ABI_EINVAL);

  /* Check changing the memory protection of neighbouring mmaps */
  addr = NaClSysMmapIntern(nap, 0, NACL_MAP_PAGESIZE,
                           NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
                           NACL_ABI_MAP_ANONYMOUS | NACL_ABI_MAP_PRIVATE,
                           -1, 0);
  printf("addr=0x%"NACL_PRIx32"\n", addr);
  initial_addr = addr;
  addr = NaClSysMmapIntern(nap, (void *) (uintptr_t) (initial_addr +
                                                      NACL_MAP_PAGESIZE),
                           NACL_MAP_PAGESIZE,
                           NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
                           NACL_ABI_MAP_ANONYMOUS | NACL_ABI_MAP_PRIVATE
                           | NACL_ABI_MAP_FIXED,
                           -1, 0);
  printf("addr=0x%"NACL_PRIx32"\n", addr);
  ASSERT_EQ(addr, initial_addr + NACL_MAP_PAGESIZE);

  errcode = NaClSysMprotectInternal(nap, initial_addr,
                                    2 * NACL_MAP_PAGESIZE,
                                    NACL_ABI_PROT_READ);
  ASSERT_EQ(errcode, 0);

  /* Undo effects of previous mmaps */
  errcode = NaClSysMunmap(natp, initial_addr, 2 * NACL_MAP_PAGESIZE);
  ASSERT_EQ(errcode, 0);

  /* Check that we cannot make the read-only data segment writable */
  ent = mem_map->vmentry[2];
  errcode = NaClSysMprotectInternal(nap, (uint32_t) (ent->page_num <<
                                                     NACL_PAGESHIFT),
                                    ent->npages * NACL_MAP_PAGESIZE,
                                    NACL_ABI_PROT_WRITE);
  ASSERT_EQ(errcode, -NACL_ABI_EACCES);

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64
  CheckForGuardRegion(nap->mem_start - ((size_t) 40 << 30), (size_t) 40 << 30);
  CheckForGuardRegion(nap->mem_start + ((size_t) 4 << 30), (size_t) 40 << 30);
#endif

  NaClAddrSpaceFree(nap);

  printf("PASS\n");
  return 0;
}
