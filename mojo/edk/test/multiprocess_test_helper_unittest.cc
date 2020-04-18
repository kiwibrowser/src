// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/test/multiprocess_test_helper.h"

#include <stddef.h>

#include <utility>

#include "base/logging.h"
#include "build/build_config.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/edk/system/test_utils.h"
#include "mojo/edk/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_POSIX)
#include <fcntl.h>
#endif

namespace mojo {
namespace edk {
namespace test {
namespace {

bool IsNonBlocking(const InternalPlatformHandle& handle) {
#if defined(OS_WIN)
  // Haven't figured out a way to query whether a HANDLE was created with
  // FILE_FLAG_OVERLAPPED.
  return true;
#else
  return fcntl(handle.handle, F_GETFL) & O_NONBLOCK;
#endif
}

bool WriteByte(const InternalPlatformHandle& handle, char c) {
  size_t bytes_written = 0;
  BlockingWrite(handle, &c, 1, &bytes_written);
  return bytes_written == 1;
}

bool ReadByte(const InternalPlatformHandle& handle, char* c) {
  size_t bytes_read = 0;
  BlockingRead(handle, c, 1, &bytes_read);
  return bytes_read == 1;
}

using MultiprocessTestHelperTest = testing::Test;

TEST_F(MultiprocessTestHelperTest, RunChild) {
  MultiprocessTestHelper helper;
  EXPECT_TRUE(helper.server_platform_handle.is_valid());

  helper.StartChild("RunChild");
  EXPECT_EQ(123, helper.WaitForChildShutdown());
}

MOJO_MULTIPROCESS_TEST_CHILD_MAIN(RunChild) {
  CHECK(MultiprocessTestHelper::client_platform_handle.is_valid());
  return 123;
}

TEST_F(MultiprocessTestHelperTest, TestChildMainNotFound) {
  MultiprocessTestHelper helper;
  helper.StartChild("NoSuchTestChildMain");
  int result = helper.WaitForChildShutdown();
  EXPECT_FALSE(result >= 0 && result <= 127);
}

TEST_F(MultiprocessTestHelperTest, PassedChannel) {
  MultiprocessTestHelper helper;
  EXPECT_TRUE(helper.server_platform_handle.is_valid());
  helper.StartChild("PassedChannel");

  // Take ownership of the handle.
  ScopedInternalPlatformHandle handle =
      std::move(helper.server_platform_handle);

  // The handle should be non-blocking.
  EXPECT_TRUE(IsNonBlocking(handle.get()));

  // Write a byte.
  const char c = 'X';
  EXPECT_TRUE(WriteByte(handle.get(), c));

  // It'll echo it back to us, incremented.
  char d = 0;
  EXPECT_TRUE(ReadByte(handle.get(), &d));
  EXPECT_EQ(c + 1, d);

  // And return it, incremented again.
  EXPECT_EQ(c + 2, helper.WaitForChildShutdown());
}

MOJO_MULTIPROCESS_TEST_CHILD_MAIN(PassedChannel) {
  CHECK(MultiprocessTestHelper::client_platform_handle.is_valid());

  // Take ownership of the handle.
  ScopedInternalPlatformHandle handle =
      std::move(MultiprocessTestHelper::client_platform_handle);

  // The handle should be non-blocking.
  EXPECT_TRUE(IsNonBlocking(handle.get()));

  // Read a byte.
  char c = 0;
  EXPECT_TRUE(ReadByte(handle.get(), &c));

  // Write it back, incremented.
  c++;
  EXPECT_TRUE(WriteByte(handle.get(), c));

  // And return it, incremented again.
  c++;
  return static_cast<int>(c);
}

TEST_F(MultiprocessTestHelperTest, ChildTestPasses) {
  MultiprocessTestHelper helper;
  EXPECT_TRUE(helper.server_platform_handle.is_valid());
  helper.StartChild("ChildTestPasses");
  EXPECT_TRUE(helper.WaitForChildTestShutdown());
}

MOJO_MULTIPROCESS_TEST_CHILD_TEST(ChildTestPasses) {
  ASSERT_TRUE(MultiprocessTestHelper::client_platform_handle.is_valid());
  EXPECT_TRUE(
      IsNonBlocking(MultiprocessTestHelper::client_platform_handle.get()));
}

TEST_F(MultiprocessTestHelperTest, ChildTestFailsAssert) {
  MultiprocessTestHelper helper;
  EXPECT_TRUE(helper.server_platform_handle.is_valid());
  helper.StartChild("ChildTestFailsAssert");
  EXPECT_FALSE(helper.WaitForChildTestShutdown());
}

MOJO_MULTIPROCESS_TEST_CHILD_TEST(ChildTestFailsAssert) {
  ASSERT_FALSE(MultiprocessTestHelper::client_platform_handle.is_valid())
      << "DISREGARD: Expected failure in child process";
  ASSERT_FALSE(
      IsNonBlocking(MultiprocessTestHelper::client_platform_handle.get()))
      << "Not reached";
  CHECK(false) << "Not reached";
}

TEST_F(MultiprocessTestHelperTest, ChildTestFailsExpect) {
  MultiprocessTestHelper helper;
  EXPECT_TRUE(helper.server_platform_handle.is_valid());
  helper.StartChild("ChildTestFailsExpect");
  EXPECT_FALSE(helper.WaitForChildTestShutdown());
}

MOJO_MULTIPROCESS_TEST_CHILD_TEST(ChildTestFailsExpect) {
  EXPECT_FALSE(MultiprocessTestHelper::client_platform_handle.is_valid())
      << "DISREGARD: Expected failure #1 in child process";
  EXPECT_FALSE(
      IsNonBlocking(MultiprocessTestHelper::client_platform_handle.get()))
      << "DISREGARD: Expected failure #2 in child process";
}

}  // namespace
}  // namespace test
}  // namespace edk
}  // namespace mojo
