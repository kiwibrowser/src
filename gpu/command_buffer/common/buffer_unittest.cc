// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/common/buffer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gpu {

TEST(Buffer, SharedMemoryHandle) {
  const size_t kSize = 1024;
  std::unique_ptr<base::SharedMemory> shared_memory(new base::SharedMemory);
  shared_memory->CreateAndMapAnonymous(kSize);
  auto shared_memory_guid = shared_memory->handle().GetGUID();
  scoped_refptr<Buffer> buffer =
      MakeBufferFromSharedMemory(std::move(shared_memory), kSize);
  EXPECT_EQ(buffer->backing()->shared_memory_handle().GetGUID(),
            shared_memory_guid);
}

}  // namespace gpu
