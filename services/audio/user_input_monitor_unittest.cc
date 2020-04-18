// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/user_input_monitor.h"

#include <memory>
#include <utility>

#include "media/base/user_input_monitor.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace audio {

namespace {
constexpr uint32_t kKeyPressCount = 10;
}

TEST(AudioServiceUserInputMonitorTest, CreateWithValidHandle) {
  std::unique_ptr<base::MappedReadOnlyRegion> shmem =
      std::make_unique<base::MappedReadOnlyRegion>(
          base::ReadOnlySharedMemoryRegion::Create(sizeof(uint32_t)));
  ASSERT_TRUE(shmem->IsValid());

  mojo::ScopedSharedBufferHandle handle =
      mojo::WrapReadOnlySharedMemoryRegion(shmem->region.Duplicate());
  EXPECT_TRUE(UserInputMonitor::Create(std::move(handle)));
}

TEST(AudioServiceUserInputMonitorTest, CreateWithInvalidHandle_ReturnsNullptr) {
  EXPECT_EQ(nullptr,
            UserInputMonitor::Create(mojo::ScopedSharedBufferHandle()));
}

TEST(AudioServiceUserInputMonitorTest, GetKeyPressCount) {
  std::unique_ptr<base::MappedReadOnlyRegion> shmem =
      std::make_unique<base::MappedReadOnlyRegion>(
          base::ReadOnlySharedMemoryRegion::Create(sizeof(uint32_t)));
  ASSERT_TRUE(shmem->IsValid());

  mojo::ScopedSharedBufferHandle handle =
      mojo::WrapReadOnlySharedMemoryRegion(shmem->region.Duplicate());
  std::unique_ptr<UserInputMonitor> monitor =
      UserInputMonitor::Create(std::move(handle));
  EXPECT_TRUE(monitor);

  media::WriteKeyPressMonitorCount(shmem->mapping, kKeyPressCount);
  EXPECT_EQ(kKeyPressCount, monitor->GetKeyPressCount());
}

TEST(AudioServiceUserInputMonitorTest, GetKeyPressCountAfterMemoryUnmap) {
  std::unique_ptr<base::MappedReadOnlyRegion> shmem =
      std::make_unique<base::MappedReadOnlyRegion>(
          base::ReadOnlySharedMemoryRegion::Create(sizeof(uint32_t)));
  ASSERT_TRUE(shmem->IsValid());

  mojo::ScopedSharedBufferHandle handle =
      mojo::WrapReadOnlySharedMemoryRegion(shmem->region.Duplicate());
  std::unique_ptr<UserInputMonitor> monitor =
      UserInputMonitor::Create(std::move(handle));
  EXPECT_TRUE(monitor);

  media::WriteKeyPressMonitorCount(shmem->mapping, kKeyPressCount);
  EXPECT_EQ(kKeyPressCount, monitor->GetKeyPressCount());

  // ReadOnlyMapping should still be valid, containing the last updated value,
  // after shmem reset.
  shmem.reset();
  EXPECT_EQ(kKeyPressCount, monitor->GetKeyPressCount());
}

}  // namespace audio
