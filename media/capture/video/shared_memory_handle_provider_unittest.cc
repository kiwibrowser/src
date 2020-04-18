// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/shared_memory_handle_provider.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>

#include "base/memory/shared_memory.h"
#include "mojo/public/cpp/system/buffer.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

namespace {

const size_t kMemorySize = 1024;

}  // anonymous namespace

class SharedMemoryHandleProviderTest : public ::testing::Test {
 public:
  SharedMemoryHandleProviderTest() = default;
  ~SharedMemoryHandleProviderTest() override = default;

  void UnwrapAndVerifyMojoHandle(
      mojo::ScopedSharedBufferHandle buffer_handle,
      size_t expected_size,
      mojo::UnwrappedSharedMemoryHandleProtection expected_protection) {
    base::SharedMemoryHandle memory_handle;
    size_t memory_size = 0;
    mojo::UnwrappedSharedMemoryHandleProtection protection;
    const MojoResult result = mojo::UnwrapSharedMemoryHandle(
        std::move(buffer_handle), &memory_handle, &memory_size, &protection);
    EXPECT_EQ(MOJO_RESULT_OK, result);
    EXPECT_EQ(expected_size, memory_size);
    EXPECT_EQ(expected_protection, protection);
  }

 protected:
  SharedMemoryHandleProvider handle_provider_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SharedMemoryHandleProviderTest);
};

TEST_F(SharedMemoryHandleProviderTest,
       VerifyInterProcessTransitHandleForReadOnly) {
  handle_provider_.InitForSize(kMemorySize);

  auto mojo_handle =
      handle_provider_.GetHandleForInterProcessTransit(true /* read_only */);

  // TODO(https://crbug.com/803136): See comment within
  // GetHandleForInterProcessTransit() for an explanation of why this
  // intentionally read-write even though it ought to be read-only.
  UnwrapAndVerifyMojoHandle(
      std::move(mojo_handle), kMemorySize,
      mojo::UnwrappedSharedMemoryHandleProtection::kReadWrite);
}

TEST_F(SharedMemoryHandleProviderTest,
       VerifyInterProcessTransitHandleForReadWrite) {
  handle_provider_.InitForSize(kMemorySize);

  auto mojo_handle =
      handle_provider_.GetHandleForInterProcessTransit(false /* read_only */);
  UnwrapAndVerifyMojoHandle(
      std::move(mojo_handle), kMemorySize,
      mojo::UnwrappedSharedMemoryHandleProtection::kReadWrite);
}

}  // namespace media
