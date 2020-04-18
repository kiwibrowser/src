// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accessibility/accessibility_focus_ring_controller.h"
#include "ash/accessibility/accessibility_cursor_ring_layer.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/ime/dummy_text_input_client.h"
#include "ui/events/event.h"
#include "ui/events/event_utils.h"

namespace ash {

class TestableAccessibilityFocusRingController
    : public AccessibilityFocusRingController {
 public:
  TestableAccessibilityFocusRingController() {
    // By default use an easy round number for testing.
    margin_ = 10;
  }
  ~TestableAccessibilityFocusRingController() override = default;

  void RectsToRings(const std::vector<gfx::Rect>& rects,
                    std::vector<AccessibilityFocusRing>* rings) const {
    AccessibilityFocusRingController::RectsToRings(rects, rings);
  }

  int GetMargin() const override { return margin_; }

  static void GetColorAndOpacityFromColor(SkColor color,
                                          float default_opacity,
                                          SkColor* result_color,
                                          float* result_opacity) {
    AccessibilityFocusRingController::GetColorAndOpacityFromColor(
        color, default_opacity, result_color, result_opacity);
  }

 private:
  int margin_;
};

class AccessibilityFocusRingControllerTest : public AshTestBase {
 public:
  AccessibilityFocusRingControllerTest() = default;
  ~AccessibilityFocusRingControllerTest() override = default;

 protected:
  gfx::Rect AddMargin(gfx::Rect r) {
    r.Inset(-controller_.GetMargin(), -controller_.GetMargin());
    return r;
  }

  TestableAccessibilityFocusRingController controller_;
};

TEST_F(AccessibilityFocusRingControllerTest, RectsToRingsSimpleBoundsCheck) {
  // Easy sanity check. Given a single rectangle, make sure we get back
  // a focus ring with the same bounds.
  std::vector<gfx::Rect> rects;
  rects.push_back(gfx::Rect(10, 30, 70, 150));
  std::vector<AccessibilityFocusRing> rings;
  controller_.RectsToRings(rects, &rings);
  ASSERT_EQ(1U, rings.size());
  ASSERT_EQ(AddMargin(rects[0]), rings[0].GetBounds());
}

TEST_F(AccessibilityFocusRingControllerTest, RectsToRingsVerticalStack) {
  // Given two rects, one on top of each other, we should get back a
  // focus ring that surrounds them both.
  std::vector<gfx::Rect> rects;
  rects.push_back(gfx::Rect(10, 10, 60, 30));
  rects.push_back(gfx::Rect(10, 40, 60, 30));
  std::vector<AccessibilityFocusRing> rings;
  controller_.RectsToRings(rects, &rings);
  ASSERT_EQ(1U, rings.size());
  ASSERT_EQ(AddMargin(gfx::Rect(10, 10, 60, 60)), rings[0].GetBounds());
}

TEST_F(AccessibilityFocusRingControllerTest, RectsToRingsHorizontalStack) {
  // Given two rects, one next to the other horizontally, we should get back a
  // focus ring that surrounds them both.
  std::vector<gfx::Rect> rects;
  rects.push_back(gfx::Rect(10, 10, 60, 30));
  rects.push_back(gfx::Rect(70, 10, 60, 30));
  std::vector<AccessibilityFocusRing> rings;
  controller_.RectsToRings(rects, &rings);
  ASSERT_EQ(1U, rings.size());
  ASSERT_EQ(AddMargin(gfx::Rect(10, 10, 120, 30)), rings[0].GetBounds());
}

TEST_F(AccessibilityFocusRingControllerTest, RectsToRingsParagraphShape) {
  // Given a simple paragraph shape, make sure we get something that
  // outlines it correctly.
  std::vector<gfx::Rect> rects;
  rects.push_back(gfx::Rect(10, 10, 180, 80));
  rects.push_back(gfx::Rect(10, 110, 580, 280));
  rects.push_back(gfx::Rect(410, 410, 180, 80));
  std::vector<AccessibilityFocusRing> rings;
  controller_.RectsToRings(rects, &rings);
  ASSERT_EQ(1U, rings.size());
  EXPECT_EQ(gfx::Rect(0, 0, 600, 500), rings[0].GetBounds());

  const gfx::Point* points = rings[0].points;
  EXPECT_EQ(gfx::Point(0, 90), points[0]);
  EXPECT_EQ(gfx::Point(0, 10), points[1]);
  EXPECT_EQ(gfx::Point(0, 0), points[2]);
  EXPECT_EQ(gfx::Point(10, 0), points[3]);
  EXPECT_EQ(gfx::Point(190, 0), points[4]);
  EXPECT_EQ(gfx::Point(200, 0), points[5]);
  EXPECT_EQ(gfx::Point(200, 10), points[6]);
  EXPECT_EQ(gfx::Point(200, 90), points[7]);
  EXPECT_EQ(gfx::Point(200, 100), points[8]);
  EXPECT_EQ(gfx::Point(210, 100), points[9]);
  EXPECT_EQ(gfx::Point(590, 100), points[10]);
  EXPECT_EQ(gfx::Point(600, 100), points[11]);
  EXPECT_EQ(gfx::Point(600, 110), points[12]);
  EXPECT_EQ(gfx::Point(600, 390), points[13]);
  EXPECT_EQ(gfx::Point(600, 400), points[14]);
  EXPECT_EQ(gfx::Point(600, 400), points[15]);
  EXPECT_EQ(gfx::Point(600, 400), points[16]);
  EXPECT_EQ(gfx::Point(600, 400), points[17]);
  EXPECT_EQ(gfx::Point(600, 410), points[18]);
  EXPECT_EQ(gfx::Point(600, 490), points[19]);
  EXPECT_EQ(gfx::Point(600, 500), points[20]);
  EXPECT_EQ(gfx::Point(590, 500), points[21]);
  EXPECT_EQ(gfx::Point(410, 500), points[22]);
  EXPECT_EQ(gfx::Point(400, 500), points[23]);
  EXPECT_EQ(gfx::Point(400, 490), points[24]);
  EXPECT_EQ(gfx::Point(400, 410), points[25]);
  EXPECT_EQ(gfx::Point(400, 400), points[26]);
  EXPECT_EQ(gfx::Point(390, 400), points[27]);
  EXPECT_EQ(gfx::Point(10, 400), points[28]);
  EXPECT_EQ(gfx::Point(0, 400), points[29]);
  EXPECT_EQ(gfx::Point(0, 390), points[30]);
  EXPECT_EQ(gfx::Point(0, 110), points[31]);
  EXPECT_EQ(gfx::Point(0, 100), points[32]);
  EXPECT_EQ(gfx::Point(0, 100), points[33]);
  EXPECT_EQ(gfx::Point(0, 100), points[34]);
  EXPECT_EQ(gfx::Point(0, 100), points[35]);
}

TEST_F(AccessibilityFocusRingControllerTest, CursorWorksOnMultipleDisplays) {
  UpdateDisplay("400x400,500x500");
  aura::Window::Windows root_windows = Shell::Get()->GetAllRootWindows();
  ASSERT_EQ(2u, root_windows.size());

  // Simulate a mouse event on the primary display.
  AccessibilityFocusRingController* controller =
      Shell::Get()->accessibility_focus_ring_controller();
  gfx::Point location(90, 90);
  controller->SetCursorRing(location);
  AccessibilityCursorRingLayer* cursor_layer =
      controller->cursor_layer_for_testing();
  EXPECT_EQ(root_windows[0], cursor_layer->root_window());
  EXPECT_LT(abs(cursor_layer->layer()->GetTargetBounds().x() - location.x()),
            50);
  EXPECT_LT(abs(cursor_layer->layer()->GetTargetBounds().y() - location.y()),
            50);

  // Simulate a mouse event at the same local location on the secondary display.
  gfx::Point location_on_secondary = location;
  location_on_secondary.Offset(400, 0);
  controller->SetCursorRing(location_on_secondary);

  cursor_layer = controller->cursor_layer_for_testing();
  EXPECT_EQ(root_windows[1], cursor_layer->root_window());
  EXPECT_LT(abs(cursor_layer->layer()->GetTargetBounds().x() - location.x()),
            50);
  EXPECT_LT(abs(cursor_layer->layer()->GetTargetBounds().y() - location.y()),
            50);
}

TEST_F(AccessibilityFocusRingControllerTest, HighlightColorCalculation) {
  SkColor without_alpha = SkColorSetARGB(0xFF, 0x42, 0x42, 0x42);
  SkColor with_alpha = SkColorSetARGB(0x3D, 0x14, 0x15, 0x92);

  float default_opacity = 0.3f;
  SkColor result_color = SK_ColorWHITE;
  float result_opacity = 0.0f;

  TestableAccessibilityFocusRingController::GetColorAndOpacityFromColor(
      without_alpha, default_opacity, &result_color, &result_opacity);
  EXPECT_EQ(default_opacity, result_opacity);

  TestableAccessibilityFocusRingController::GetColorAndOpacityFromColor(
      with_alpha, default_opacity, &result_color, &result_opacity);
  EXPECT_NEAR(0.239f, result_opacity, .001);
}

}  // namespace ash
