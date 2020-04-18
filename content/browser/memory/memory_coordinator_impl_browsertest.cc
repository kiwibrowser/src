// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/memory_coordinator_impl.h"

#include "base/test/scoped_feature_list.h"
#include "content/browser/browser_main_loop.h"
#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/public/common/content_features.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"

namespace content {

class MemoryCoordinatorImplBrowserTest : public ContentBrowserTest {
 public:
  MemoryCoordinatorImplBrowserTest() {}

  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(features::kMemoryCoordinator);
    ContentBrowserTest::SetUp();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(MemoryCoordinatorImplBrowserTest);
};

// TODO(bashi): Enable these tests on macos when MemoryMonitorMac is
// implemented.
#if !defined(OS_MACOSX)

IN_PROC_BROWSER_TEST_F(MemoryCoordinatorImplBrowserTest, HandleAdded) {
  GURL url = GetTestUrl("", "simple_page.html");
  NavigateToURL(shell(), url);

  size_t num_children = 0;
  for (auto const& it : MemoryCoordinatorImpl::GetInstance()->children()) {
    int process_id = it.first;

    // Ignore the spare process.
    RenderProcessHost* spare_process =
        RenderProcessHostImpl::GetSpareRenderProcessHostForTesting();
    if (spare_process && process_id == spare_process->GetID())
      continue;

    num_children++;
  }

  EXPECT_EQ(1u, num_children);
}

IN_PROC_BROWSER_TEST_F(MemoryCoordinatorImplBrowserTest, GetStateForProcess) {
  GURL url = GetTestUrl("", "simple_page.html");
  NavigateToURL(shell(), url);
  auto* memory_coordinator = MemoryCoordinator::GetInstance();
  base::ProcessHandle handle = base::GetCurrentProcessHandle();
  EXPECT_NE(base::MemoryState::UNKNOWN,
            memory_coordinator->GetStateForProcess(handle));
}

#endif  // !defined(OS_MACOSX)

}  // namespace content
