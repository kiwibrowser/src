// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/heap_profiling/public/cpp/sender_pipe.h"

#include <vector>

#include "build/build_config.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(OS_MACOSX)
#include "mojo/edk/embedder/platform_channel_pair.h"
#endif

namespace heap_profiling {
namespace {

using Result = SenderPipe::Result;

class SenderPipeTest : public testing::Test {
 public:
  void SetUp() override {
    mojo::edk::ScopedInternalPlatformHandle write_handle;

    SenderPipe::PipePair pipes;
    read_handle_ = pipes.PassReceiver();

    base::ScopedPlatformFile file(pipes.PassSender().release().handle);
    sender_pipe_.reset(new SenderPipe(std::move(file)));

    // A large buffer for both writing and reading.
    buffer_.resize(64 * 1024);
  }

  Result Write(int size) { return sender_pipe_->Send(buffer_.data(), size, 1); }

  void Read(int size) {
#if defined(OS_POSIX)
    ssize_t bytes_read = read(read_handle_.get().handle, buffer_.data(), size);
    ASSERT_EQ(size, bytes_read);
#else
    OVERLAPPED overlapped;
    DWORD bytes_read = 0;
    memset(&overlapped, 0, sizeof(OVERLAPPED));
    BOOL result = ::ReadFile(read_handle_.get().handle, buffer_.data(), size,
                             &bytes_read, &overlapped);
    ASSERT_TRUE(result);
    ASSERT_EQ(static_cast<DWORD>(size), bytes_read);
#endif
  }

 private:
  mojo::edk::ScopedInternalPlatformHandle read_handle_;
  std::unique_ptr<SenderPipe> sender_pipe_;
  std::vector<char> buffer_;
};

TEST_F(SenderPipeTest, TimeoutNoRead) {
  // Writing 64k should not time out.
  Result result = Write(64 * 1024);
  ASSERT_EQ(Result::kSuccess, result);

  // Writing 64k more should time out, since the buffer size is 64k.
  result = Write(64 * 1024);
  ASSERT_EQ(Result::kTimeout, result);
}

TEST_F(SenderPipeTest, TimeoutSmallRead) {
  // Writing 64k should not time out.
  Result result = Write(64 * 1024);
  ASSERT_EQ(Result::kSuccess, result);

  // Read 32k out of the buffer.
  Read(32 * 1024);

  // Writing 64k more should still time out, since the buffer size should be
  // 64k.
  result = Write(64 * 1024);
  ASSERT_EQ(Result::kTimeout, result);
}

TEST_F(SenderPipeTest, NoTimeout) {
  // Writing 64k should not time out.
  Result result = Write(64 * 1024);
  ASSERT_EQ(Result::kSuccess, result);

  // Read 64k out of the buffer.
  Read(64 * 1024);

  // Writing 64k should not time out.
  result = Write(64 * 1024);
  ASSERT_EQ(Result::kSuccess, result);
}

}  // namespace
}  // namespace heap_profiling
