// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/memory_monitor_chromeos.h"

#include "content/browser/memory/test_memory_monitor.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

// A delegate that allows mocking the various inputs to MemoryMonitorChromeOS.
class TestMemoryMonitorChromeOSDelegate : public TestMemoryMonitorDelegate {
 public:
  TestMemoryMonitorChromeOSDelegate() {}

  void SetFreeMemoryKB(int free_kb,
                       int swap_kb,
                       int active_kb,
                       int inactive_kb,
                       int dirty_kb,
                       int available_kb) {
    mem_info_.free = free_kb;
    mem_info_.swap_free = swap_kb;
    mem_info_.active_file = active_kb;
    mem_info_.inactive_file = inactive_kb;
    mem_info_.dirty = dirty_kb;
    mem_info_.available = available_kb;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestMemoryMonitorChromeOSDelegate);
};

class TestMemoryMonitorChromeOS : public MemoryMonitorChromeOS {};

static const int kKBperMB = 1024;

}  // namespace

class MemoryMonitorChromeOSTest : public testing::Test {
 public:
  TestMemoryMonitorChromeOSDelegate delegate_;
  std::unique_ptr<MemoryMonitorChromeOS> monitor_;
};

TEST_F(MemoryMonitorChromeOSTest, Create) {
  delegate_.SetTotalMemoryKB(100000 * kKBperMB);
  monitor_ = MemoryMonitorChromeOS::Create(&delegate_);
  EXPECT_EQ(0U, delegate_.calls());
}

TEST_F(MemoryMonitorChromeOSTest, GetFreeMemoryUntilCriticalMB) {
  delegate_.SetTotalMemoryKB(1000 * kKBperMB);

  monitor_.reset(new MemoryMonitorChromeOS(&delegate_));
  EXPECT_EQ(0u, delegate_.calls());

  // |available| is supported.
  delegate_.SetFreeMemoryKB(1 * kKBperMB, 1 * kKBperMB, 1 * kKBperMB,
                            1 * kKBperMB, 1 * kKBperMB, 286 * kKBperMB);
  EXPECT_EQ(286, monitor_->GetFreeMemoryUntilCriticalMB());
  EXPECT_EQ(1U, delegate_.calls());

  // |available| is not supported.
  delegate_.SetFreeMemoryKB(256 * kKBperMB, 128 * kKBperMB, 64 * kKBperMB,
                            32 * kKBperMB, 16 * kKBperMB, 0 * kKBperMB);
  EXPECT_EQ(286, monitor_->GetFreeMemoryUntilCriticalMB());
  EXPECT_EQ(2U, delegate_.calls());
}

}  // namespace content
