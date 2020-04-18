/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/mmap_test_check.h"

#include <windows.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

void CheckMapping(uintptr_t addr, size_t size, int state, int protect,
                  int map_type) {
  for (size_t offset = 0; offset < size; offset += NACL_MAP_PAGESIZE) {
    MEMORY_BASIC_INFORMATION info;
    void *sysaddr = (void *) (addr + offset);
    size_t result = VirtualQuery(sysaddr, &info, sizeof(info));
    ASSERT_EQ(result, sizeof(info));
    ASSERT_EQ(info.BaseAddress, sysaddr);
    ASSERT_EQ(info.AllocationBase, sysaddr);
    ASSERT_EQ(info.RegionSize, NACL_MAP_PAGESIZE);
    ASSERT_EQ(info.State, state);
    ASSERT_EQ(info.Protect, protect);
    ASSERT_EQ(info.Type, map_type);
  }
}

void CheckGuardMapping(uintptr_t addr, size_t size, int state, int protect,
                       int map_type) {
  MEMORY_BASIC_INFORMATION info;
  size_t result = VirtualQuery((void *) addr, &info, sizeof(info));
  ASSERT_EQ(result, sizeof(info));
  ASSERT_EQ(info.BaseAddress, (void *) addr);
  ASSERT_EQ(info.AllocationBase, (void *) addr);
  ASSERT_EQ(info.RegionSize, size);
  ASSERT_EQ(info.State, state);
  ASSERT_EQ(info.Protect, protect);
  ASSERT_EQ(info.Type, map_type);
}

