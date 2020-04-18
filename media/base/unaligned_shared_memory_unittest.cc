// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/unaligned_shared_memory.h"

#include <stdint.h>
#include <string.h>

#include <limits>

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/shared_memory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

namespace {

const uint8_t kUnalignedData[] = "XXXhello";
const size_t kUnalignedDataSize = arraysize(kUnalignedData);
const off_t kUnalignedOffset = 3;

const uint8_t kData[] = "hello";
const size_t kDataSize = arraysize(kData);

base::SharedMemoryHandle CreateHandle(const uint8_t* data, size_t size) {
  base::SharedMemory shm;
  EXPECT_TRUE(shm.CreateAndMapAnonymous(size));
  memcpy(shm.memory(), data, size);
  return shm.TakeHandle();
}

}  // namespace

TEST(UnalignedSharedMemoryTest, CreateAndDestroy) {
  auto handle = CreateHandle(kData, kDataSize);
  UnalignedSharedMemory shm(handle, true);
}

TEST(UnalignedSharedMemoryTest, CreateAndDestroy_InvalidHandle) {
  base::SharedMemoryHandle handle;
  UnalignedSharedMemory shm(handle, true);
}

TEST(UnalignedSharedMemoryTest, Map) {
  auto handle = CreateHandle(kData, kDataSize);
  UnalignedSharedMemory shm(handle, true);
  ASSERT_TRUE(shm.MapAt(0, kDataSize));
  EXPECT_EQ(0, memcmp(shm.memory(), kData, kDataSize));
}

TEST(UnalignedSharedMemoryTest, Map_Unaligned) {
  auto handle = CreateHandle(kUnalignedData, kUnalignedDataSize);
  UnalignedSharedMemory shm(handle, true);
  ASSERT_TRUE(shm.MapAt(kUnalignedOffset, kDataSize));
  EXPECT_EQ(0, memcmp(shm.memory(), kData, kDataSize));
}

TEST(UnalignedSharedMemoryTest, Map_InvalidHandle) {
  base::SharedMemoryHandle handle;
  UnalignedSharedMemory shm(handle, true);
  ASSERT_FALSE(shm.MapAt(1, kDataSize));
  EXPECT_EQ(shm.memory(), nullptr);
}

TEST(UnalignedSharedMemoryTest, Map_NegativeOffset) {
  auto handle = CreateHandle(kData, kDataSize);
  UnalignedSharedMemory shm(handle, true);
  ASSERT_FALSE(shm.MapAt(-1, kDataSize));
}

TEST(UnalignedSharedMemoryTest, Map_SizeOverflow) {
  auto handle = CreateHandle(kData, kDataSize);
  UnalignedSharedMemory shm(handle, true);
  ASSERT_FALSE(shm.MapAt(1, std::numeric_limits<size_t>::max()));
}

TEST(UnalignedSharedMemoryTest, UnmappedIsNullptr) {
  auto handle = CreateHandle(kData, kDataSize);
  UnalignedSharedMemory shm(handle, true);
  ASSERT_EQ(shm.memory(), nullptr);
}

}  // namespace media
