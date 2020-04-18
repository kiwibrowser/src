// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/util/lzma_file_allocator.h"

#include "base/files/file_util.h"
#include "base/logging.h"

#include <windows.h>

extern "C" {
#include "third_party/lzma_sdk/7zAlloc.h"
}

LzmaFileAllocator::LzmaFileAllocator(const base::FilePath& temp_directory) {
  DCHECK(base::DirectoryExists(temp_directory));

  // Direct the ISzAlloc functions to the LZMA SDK's default allocator.
  Alloc = SzAlloc;
  Free = SzFree;

  if (!base::CreateTemporaryFileInDir(temp_directory, &mapped_file_path_))
    return;

  mapped_file_.Initialize(mapped_file_path_,
                          base::File::FLAG_CREATE_ALWAYS |
                              base::File::FLAG_READ | base::File::FLAG_WRITE |
                              base::File::FLAG_TEMPORARY |
                              base::File::FLAG_DELETE_ON_CLOSE);
  if (!mapped_file_.IsValid()) {
    DPLOG(ERROR) << "Failed to initialize mapped file";
    return;
  }

  // Direct the ISzAlloc functions to this class's allocator.
  Alloc = SzFileAlloc;
  Free = SzFileFree;
}

LzmaFileAllocator::~LzmaFileAllocator() {
  DCHECK(!file_mapping_handle_.IsValid());
  Alloc = nullptr;
  Free = nullptr;
}

bool LzmaFileAllocator::IsAddressMapped(uintptr_t address) const {
  return (address >= mapped_start_address_) &&
         (address < mapped_start_address_ + mapped_size_);
}

void* LzmaFileAllocator::DoAllocate(size_t size) {
  DCHECK(!file_mapping_handle_.IsValid());

  if (size == 0)
    return nullptr;

  if (!mapped_file_.IsValid())
    return SzAlloc(this, size);

  if (!mapped_file_.SetLength(size)) {
    DPLOG(ERROR) << "Failed to set length of mapped file";
    return SzAlloc(this, size);
  }

  file_mapping_handle_.Set(::CreateFileMapping(
      mapped_file_.GetPlatformFile(), nullptr, PAGE_READWRITE, 0, 0, nullptr));
  if (!file_mapping_handle_.IsValid()) {
    DPLOG(ERROR) << "Failed to create file mapping handle";
    return SzAlloc(this, size);
  }

  void* ret =
      ::MapViewOfFile(file_mapping_handle_.Get(), FILE_MAP_WRITE, 0, 0, 0);
  if (!ret) {
    DPLOG(ERROR) << "Failed to map view of file";
    file_mapping_handle_.Close();
    return SzAlloc(this, size);
  }
  mapped_start_address_ = reinterpret_cast<uintptr_t>(ret);
  mapped_size_ = size;
  return ret;
}

void LzmaFileAllocator::DoFree(void* address) {
  if (address == nullptr)
    return;
  if (!file_mapping_handle_.IsValid()) {
    SzFree(this, address);
    return;
  }
  int ret = ::UnmapViewOfFile(address);
  DPCHECK(ret != 0) << "Failed to unmap view of file";
  mapped_start_address_ = 0;
  mapped_size_ = 0;
  file_mapping_handle_.Close();
}

void* LzmaFileAllocator::SzFileAlloc(void* p, size_t size) {
  return static_cast<LzmaFileAllocator*>(p)->DoAllocate(size);
}

void LzmaFileAllocator::SzFileFree(void* p, void* address) {
  static_cast<LzmaFileAllocator*>(p)->DoFree(address);
}
