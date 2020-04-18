// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/render_thread_impl.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/memory/discardable_memory.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "components/discardable_memory/client/client_discardable_shared_memory_manager.h"
#include "components/discardable_memory/service/discardable_shared_memory_manager.h"
#include "content/browser/browser_main_loop.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "content/shell/browser/shell.h"
#include "gpu/ipc/common/gpu_memory_buffer_impl.h"
#include "ui/gfx/buffer_format_util.h"
#include "url/gurl.h"

namespace content {
namespace {

class RenderThreadImplDiscardableMemoryBrowserTest : public ContentBrowserTest {
 public:
  RenderThreadImplDiscardableMemoryBrowserTest()
      : child_discardable_shared_memory_manager_(nullptr) {}

  // Overridden from BrowserTestBase:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kSingleProcess);
  }

  void SetUpOnMainThread() override {
    NavigateToURL(shell(), GURL(url::kAboutBlankURL));
    PostTaskToInProcessRendererAndWait(base::Bind(
        &RenderThreadImplDiscardableMemoryBrowserTest::SetUpOnRenderThread,
        base::Unretained(this)));
  }

  discardable_memory::ClientDiscardableSharedMemoryManager*
  child_discardable_shared_memory_manager() {
    return child_discardable_shared_memory_manager_;
  }

 private:
  void SetUpOnRenderThread() {
    child_discardable_shared_memory_manager_ =
        RenderThreadImpl::current()->GetDiscardableSharedMemoryManagerForTest();
  }

  discardable_memory::ClientDiscardableSharedMemoryManager*
      child_discardable_shared_memory_manager_;
};

IN_PROC_BROWSER_TEST_F(RenderThreadImplDiscardableMemoryBrowserTest,
                       LockDiscardableMemory) {
  const size_t kSize = 1024 * 1024;  // 1MiB.

  std::unique_ptr<base::DiscardableMemory> memory =
      child_discardable_shared_memory_manager()
          ->AllocateLockedDiscardableMemory(kSize);

  ASSERT_TRUE(memory);
  void* addr = memory->data();
  ASSERT_NE(nullptr, addr);

  memory->Unlock();

  // Purge all unlocked memory.
  BrowserMainLoop::GetInstance()
      ->discardable_shared_memory_manager()
      ->SetMemoryLimit(0);

  // Should fail as memory should have been purged.
  EXPECT_FALSE(memory->Lock());
}

// Disable the test for the Android asan build.
// See http://crbug.com/667837 for detail.
#if !(defined(OS_ANDROID) && defined(ADDRESS_SANITIZER))
IN_PROC_BROWSER_TEST_F(RenderThreadImplDiscardableMemoryBrowserTest,
                       DiscardableMemoryAddressSpace) {
  const size_t kLargeSize = 4 * 1024 * 1024;   // 4MiB.
  const size_t kNumberOfInstances = 1024 + 1;  // >4GiB total.

  std::vector<std::unique_ptr<base::DiscardableMemory>> instances;
  for (size_t i = 0; i < kNumberOfInstances; ++i) {
    std::unique_ptr<base::DiscardableMemory> memory =
        child_discardable_shared_memory_manager()
            ->AllocateLockedDiscardableMemory(kLargeSize);
    ASSERT_TRUE(memory);
    void* addr = memory->data();
    ASSERT_NE(nullptr, addr);
    memory->Unlock();
    instances.push_back(std::move(memory));
  }
}
#endif

IN_PROC_BROWSER_TEST_F(RenderThreadImplDiscardableMemoryBrowserTest,
                       ReleaseFreeDiscardableMemory) {
  const size_t kSize = 1024 * 1024;  // 1MiB.

  std::unique_ptr<base::DiscardableMemory> memory =
      child_discardable_shared_memory_manager()
          ->AllocateLockedDiscardableMemory(kSize);

  EXPECT_TRUE(memory);
  memory.reset();

  EXPECT_GE(BrowserMainLoop::GetInstance()
                ->discardable_shared_memory_manager()
                ->GetBytesAllocated(),
            kSize);

  child_discardable_shared_memory_manager()->ReleaseFreeMemory();

  // Busy wait for host memory usage to be reduced.
  base::TimeTicks end =
      base::TimeTicks::Now() + base::TimeDelta::FromSeconds(5);
  while (base::TimeTicks::Now() < end) {
    if (!BrowserMainLoop::GetInstance()
             ->discardable_shared_memory_manager()
             ->GetBytesAllocated())
      break;
  }

  EXPECT_LT(base::TimeTicks::Now(), end);
}

IN_PROC_BROWSER_TEST_F(RenderThreadImplDiscardableMemoryBrowserTest,
                       ReleaseFreeMemory) {
  const size_t kSize = 1024 * 1024;  // 1MiB.

  std::unique_ptr<base::DiscardableMemory> memory =
      child_discardable_shared_memory_manager()
          ->AllocateLockedDiscardableMemory(kSize);

  EXPECT_TRUE(memory);
  memory.reset();

  EXPECT_GE(BrowserMainLoop::GetInstance()
                ->discardable_shared_memory_manager()
                ->GetBytesAllocated(),
            kSize);

  // Call RenderThreadImpl::ReleaseFreeMemory through a fake memory pressure
  // notification.
  base::MemoryPressureListener::SimulatePressureNotification(
      base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL);
  base::RunLoop().RunUntilIdle();
  RunAllTasksUntilIdle();

  EXPECT_EQ(0U, BrowserMainLoop::GetInstance()
                    ->discardable_shared_memory_manager()
                    ->GetBytesAllocated());
}

}  // namespace
}  // namespace content
