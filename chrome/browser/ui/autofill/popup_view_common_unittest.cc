// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/autofill/popup_view_common.h"

#include <stddef.h>

#include <memory>

#include "base/macros.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/browser/web_contents.h"
#include "ui/display/display.h"
#include "ui/gfx/geometry/rect.h"

namespace autofill {

namespace {

// Test class which overrides specific behavior for testing.
class TestPopupViewCommon : public PopupViewCommon {
 public:
  explicit TestPopupViewCommon(const display::Display& display)
      : display_(display) {}

  display::Display GetDisplayNearestPoint(
      const gfx::Point& point,
      gfx::NativeView container_view) override {
    return display_;
  }

 private:
  display::Display display_;
};

}  // namespace

class PopupViewCommonTest : public ChromeRenderViewHostTestHarness {
 public:
  PopupViewCommonTest() {}
  ~PopupViewCommonTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(PopupViewCommonTest);
};

TEST_F(PopupViewCommonTest, CalculatePopupBounds) {
  int desired_width = 40;
  int desired_height = 16;

  // Set up the visible screen space.
  display::Display display(
      0, gfx::Rect(0, 0, 2 * desired_width, 2 * desired_height));
  TestPopupViewCommon view_common(display);

  struct {
    gfx::Rect element_bounds;
    gfx::Rect expected_popup_bounds_ltr;
    // Non-empty only when it differs from the ltr expectation.
    gfx::Rect expected_popup_bounds_rtl;
  } test_cases[] = {
      // The popup grows down and to the end.
      {gfx::Rect(38, 0, 5, 0), gfx::Rect(38, 0, desired_width, desired_height),
       gfx::Rect(3, 0, desired_width, desired_height)},

      // The popup grows down and to the left when there's no room on the right.
      {gfx::Rect(2 * desired_width, 0, 5, 0),
       gfx::Rect(desired_width, 0, desired_width, desired_height)},

      // The popup grows up and to the right.
      {gfx::Rect(0, 2 * desired_height, 5, 0),
       gfx::Rect(0, desired_height, desired_width, desired_height)},

      // The popup grows up and to the left.
      {gfx::Rect(2 * desired_width, 2 * desired_height, 5, 0),
       gfx::Rect(desired_width, desired_height, desired_width, desired_height)},

      // The popup would be partial off the top and left side of the screen.
      {gfx::Rect(-desired_width / 2, -desired_height / 2, 5, 0),
       gfx::Rect(0, 0, desired_width, desired_height)},

      // The popup would be partially off the bottom and the right side of
      // the screen.
      {gfx::Rect(desired_width * 1.5, desired_height * 1.5, 5, 0),
       gfx::Rect((desired_width * 1.5 + 5 - desired_width),
                 (desired_height * 1.5 - desired_height), desired_width,
                 desired_height)},
  };

  for (size_t i = 0; i < arraysize(test_cases); ++i) {
    gfx::Rect actual_popup_bounds = view_common.CalculatePopupBounds(
        desired_width, desired_height, test_cases[i].element_bounds,
        web_contents()->GetNativeView(), /* is_rtl= */ false);
    EXPECT_EQ(test_cases[i].expected_popup_bounds_ltr.ToString(),
              actual_popup_bounds.ToString())
        << "Popup bounds failed to match for ltr test " << i;

    actual_popup_bounds = view_common.CalculatePopupBounds(
        desired_width, desired_height, test_cases[i].element_bounds,
        web_contents()->GetNativeView(), /* is_rtl= */ true);
    gfx::Rect expected_popup_bounds = test_cases[i].expected_popup_bounds_rtl;
    if (expected_popup_bounds.IsEmpty())
      expected_popup_bounds = test_cases[i].expected_popup_bounds_ltr;
    EXPECT_EQ(expected_popup_bounds.ToString(), actual_popup_bounds.ToString())
        << "Popup bounds failed to match for rtl test " << i;
  }
}

}  // namespace autofill
