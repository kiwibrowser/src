// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/util/lzma_file_allocator.h"

#include <stddef.h>

#include <memory>
#include <string>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "testing/gtest/include/gtest/gtest.h"

#include <windows.h>

class LzmaFileAllocatorTest : public testing::Test {
 protected:
  void SetUp() override { ASSERT_TRUE(temp_dir_.CreateUniqueTempDir()); }
  // Returns the type of the memory page identified by |address|; one of
  // MEM_IMAGE, MEM_MAPPED or MEM_PRIVATE.
  static DWORD GetMemoryType(void* address);

  base::ScopedTempDir temp_dir_;
};

DWORD LzmaFileAllocatorTest::GetMemoryType(void* address) {
  MEMORY_BASIC_INFORMATION memory_info = {};
  EXPECT_NE(0U, ::VirtualQuery(address, &memory_info,
                               sizeof(memory_info)));
  return memory_info.Type;
}

TEST_F(LzmaFileAllocatorTest, ReadAndWriteWithMultipleSizeTest) {
  static const char kSampleExpectedCharacter = 'a';
  SYSTEM_INFO sysinfo;
  ::GetSystemInfo(&sysinfo);
  EXPECT_GT(sysinfo.dwPageSize, 0U);

  size_t size_list[] = {1, 10, sysinfo.dwPageSize - 1, sysinfo.dwPageSize,
                        sysinfo.dwPageSize + 1};

  for (size_t size : size_list) {
    LzmaFileAllocator allocator(temp_dir_.GetPath());
    char* s = reinterpret_cast<char*>(IAlloc_Alloc(&allocator, size));
    std::fill_n(s, size, kSampleExpectedCharacter);
    char* ret = std::find_if(s, s + size, [](char c) {
      return c != kSampleExpectedCharacter;
    });
    EXPECT_EQ(s + size, ret);
    EXPECT_EQ(static_cast<DWORD>(MEM_MAPPED), GetMemoryType(s));

    IAlloc_Free(&allocator, s);
  }
}

TEST_F(LzmaFileAllocatorTest, SizeIsZeroTest) {
  LzmaFileAllocator allocator(temp_dir_.GetPath());
  char* s = reinterpret_cast<char*>(IAlloc_Alloc(&allocator, 0));
  EXPECT_EQ(s, nullptr);

  IAlloc_Free(&allocator, s);
}

TEST_F(LzmaFileAllocatorTest, DeleteAfterCloseTest) {
  std::unique_ptr<LzmaFileAllocator> allocator =
      std::make_unique<LzmaFileAllocator>(temp_dir_.GetPath());
  base::FilePath file_path = allocator->mapped_file_path_;
  ASSERT_TRUE(base::PathExists(file_path));
  allocator.reset();
  ASSERT_FALSE(base::PathExists(file_path));
}

TEST_F(LzmaFileAllocatorTest, ErrorAndFallbackTest) {
  LzmaFileAllocator allocator(temp_dir_.GetPath());
  allocator.mapped_file_.Close();
  char* s = reinterpret_cast<char*>(IAlloc_Alloc(&allocator, 10));
  EXPECT_NE(nullptr, s);
  ASSERT_FALSE(allocator.file_mapping_handle_.IsValid());
  EXPECT_EQ(static_cast<DWORD>(MEM_PRIVATE), GetMemoryType(s));

  IAlloc_Free(&allocator, s);
}

TEST_F(LzmaFileAllocatorTest, IsAddressMappedTest) {
  LzmaFileAllocator allocator(temp_dir_.GetPath());
  size_t size = 10;
  uintptr_t address =
      reinterpret_cast<uintptr_t>(IAlloc_Alloc(&allocator, size));
  ASSERT_TRUE(allocator.IsAddressMapped(address));
  ASSERT_TRUE(allocator.IsAddressMapped(address + 1));
  ASSERT_TRUE(allocator.IsAddressMapped(address + 9));
  ASSERT_FALSE(allocator.IsAddressMapped(address + 10));
  ASSERT_FALSE(allocator.IsAddressMapped(address + 11));
  ASSERT_FALSE(allocator.IsAddressMapped(address - 1));
  IAlloc_Free(&allocator, reinterpret_cast<void*>(address));
}
