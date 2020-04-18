// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_UTIL_LZMA_FILE_ALLOCATOR_H_
#define CHROME_INSTALLER_UTIL_LZMA_FILE_ALLOCATOR_H_

#include <stddef.h>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/win/scoped_handle.h"
#include "third_party/lzma_sdk/7zTypes.h"

// File mapping memory management class which supports multiple allocations in
// series, but not in parallel. It creates a unique temporary file within
// |temp_directory| to back subsequent allocations and falls back to normal
// management in case of error.
// Example:
//   LzmaFileAllocator allocator(mmp_file_path);
//   char* s = reinterpret_cast<char*>(IAlloc_Alloc(&allocator, size));
//   IAlloc_Free(&allocator, s);
class LzmaFileAllocator : public ISzAlloc {
 public:
  explicit LzmaFileAllocator(const base::FilePath& temp_directory);
  ~LzmaFileAllocator();
  bool IsAddressMapped(uintptr_t address) const;

 private:
  FRIEND_TEST_ALL_PREFIXES(LzmaFileAllocatorTest, ErrorAndFallbackTest);
  FRIEND_TEST_ALL_PREFIXES(LzmaFileAllocatorTest, DeleteAfterCloseTest);

  // Allocates |size| bytes backed by the instance's temporary file. The heap
  // is used if file allocation fails for any reason.
  void* DoAllocate(size_t size);

  // Frees the memory from disk or heap depending on the type of allocation.
  void DoFree(void* address);

  // ISzAlloc hook functions
  static void* SzFileAlloc(void* p, size_t size);
  static void SzFileFree(void* p, void* address);

  base::FilePath mapped_file_path_;
  base::File mapped_file_;
  base::win::ScopedHandle file_mapping_handle_;

  uintptr_t mapped_start_address_ = 0;
  size_t mapped_size_ = 0;

  DISALLOW_COPY_AND_ASSIGN(LzmaFileAllocator);
};

#endif  // CHROME_INSTALLER_UTIL_LZMA_FILE_ALLOCATOR_H_
