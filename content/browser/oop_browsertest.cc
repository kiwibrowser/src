// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "content/shell/browser/shell.h"
#include "gpu/config/gpu_switches.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/ui_base_features.h"
#include "ui/base/ui_base_switches.h"
#include "ui/compositor/compositor_switches.h"
#include "ui/gl/gl_switches.h"

namespace content {
namespace {

class OOPBrowserTest : public ContentBrowserTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    ContentBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kEnableGpuRasterization);
    command_line->AppendSwitch(switches::kEnablePixelOutputInTests);
    command_line->AppendSwitch(switches::kEnableOOPRasterization);

    const bool use_gpu_in_tests = !features::IsMashEnabled();
    if (use_gpu_in_tests)
      command_line->AppendSwitch(switches::kUseGpuInTests);
  }

  void VerifyVisualStateUpdated(const base::Closure& done_cb,
                                bool visual_state_updated) {
    ASSERT_TRUE(visual_state_updated);
    done_cb.Run();
  }

  SkBitmap snapshot_;
};

// This test calls into system GL which is not instrumented with MSAN.
#if !defined(MEMORY_SANITIZER)
IN_PROC_BROWSER_TEST_F(OOPBrowserTest, Basic) {
  // Create a div to ensure we don't use solid color quads.
  GURL url = GURL(
      "data:text/html,"
      "<style>div{background-color:blue; width:100; height:100;}</style>"
      "<body bgcolor=blue><div></div></body>");
  NavigateToURLBlockUntilNavigationsComplete(shell(), url, 1);
  shell()->web_contents()->GetMainFrame()->InsertVisualStateCallback(base::Bind(
      &OOPBrowserTest::VerifyVisualStateUpdated, base::Unretained(this),
      base::RunLoop::QuitCurrentWhenIdleClosureDeprecated()));
  content::RunMessageLoop();

  auto* rwh = shell()->web_contents()->GetRenderViewHost()->GetWidget();
  ASSERT_TRUE(rwh->GetView()->IsSurfaceAvailableForCopy());
  base::RunLoop run_loop;
  SkBitmap snapshot;
  rwh->GetView()->CopyFromSurface(
      gfx::Rect(), gfx::Size(),
      base::BindOnce(
          [](SkBitmap* snapshot, base::OnceClosure done_cb,
             const SkBitmap& bitmap) {
            *snapshot = bitmap;
            std::move(done_cb).Run();
          },
          &snapshot, run_loop.QuitWhenIdleClosure()));
  run_loop.Run();

  EXPECT_FALSE(snapshot.drawsNothing());
  for (int i = 0; i < snapshot.width(); ++i) {
    for (int j = 0; j < snapshot.height(); ++j) {
      ASSERT_EQ(snapshot.getColor(i, j), SK_ColorBLUE);
    }
  }
};
#endif

}  // namespace
}  // namespace content
