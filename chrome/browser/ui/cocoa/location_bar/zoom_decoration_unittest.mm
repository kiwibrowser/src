// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/location_bar/zoom_decoration.h"

#include "base/command_line.h"
#include "base/macros.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/zoom/zoom_controller.h"
#include "ui/base/ui_base_features.h"

namespace {

class ZoomDecorationTest : public ChromeRenderViewHostTestHarness,
                           public ::testing::WithParamInterface<bool> {
 public:
  ZoomDecorationTest() {}
  ~ZoomDecorationTest() override {}

 protected:
  // ChromeRenderViewHostTestHarness:
  void SetUp() override {
    // TODO(crbug.com/630357): Remove parameterized testing for this class when
    // secondary-ui-md is enabled by default on all platforms.
    if (GetParam())
      scoped_feature_list_.InitAndEnableFeature(features::kSecondaryUiMd);
    else
      scoped_feature_list_.InitAndDisableFeature(features::kSecondaryUiMd);
    ChromeRenderViewHostTestHarness::SetUp();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(ZoomDecorationTest);
};

class MockZoomDecoration : public ZoomDecoration {
 public:
  explicit MockZoomDecoration(LocationBarViewMac* owner)
      : ZoomDecoration(owner), update_ui_count_(0) {}
  bool ShouldShowDecoration() const override { return true; }
  void UpdateUI(zoom::ZoomController* zoom_controller,
                NSString* tooltip_string,
                bool location_bar_is_dark) override {
    ++update_ui_count_;
    ZoomDecoration::UpdateUI(zoom_controller, tooltip_string,
                             location_bar_is_dark);
  }

  int update_ui_count_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MockZoomDecoration);
};

class MockZoomController : public zoom::ZoomController {
 public:
  explicit MockZoomController(content::WebContents* web_contents)
      : zoom::ZoomController(web_contents) {}
  int GetZoomPercent() const override { return zoom_percent_; }
  bool IsAtDefaultZoom() const override { return zoom_percent_ == 100; }

  int zoom_percent_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MockZoomController);
};

// Test that UpdateIfNecessary performs redraws only when the zoom percent
// changes.
TEST_P(ZoomDecorationTest, ChangeZoomPercent) {
  MockZoomDecoration decoration(NULL);
  MockZoomController controller(web_contents());

  controller.zoom_percent_ = 100;
  decoration.UpdateIfNecessary(&controller,
                               /*default_zoom_changed=*/false,
                               false);
  EXPECT_EQ(1, decoration.update_ui_count_);

  decoration.UpdateIfNecessary(&controller,
                               /*default_zoom_changed=*/false,
                               false);
  EXPECT_EQ(1, decoration.update_ui_count_);

  controller.zoom_percent_ = 80;
  decoration.UpdateIfNecessary(&controller,
                               /*default_zoom_changed=*/false,
                               false);
  EXPECT_EQ(2, decoration.update_ui_count_);

  // Always redraw if the default zoom changes.
  decoration.UpdateIfNecessary(&controller,
                               /*default_zoom_changed=*/true,
                               false);
  EXPECT_EQ(3, decoration.update_ui_count_);
}

// Prefix for test instantiations intentionally left blank since the test
// fixture class has a single parameterization.
INSTANTIATE_TEST_CASE_P(, ZoomDecorationTest, testing::Bool());

}  // namespace
