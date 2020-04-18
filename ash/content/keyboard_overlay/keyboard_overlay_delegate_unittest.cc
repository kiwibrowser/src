// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/content/keyboard_overlay/keyboard_overlay_delegate.h"

#include "ash/public/cpp/shelf_types.h"
#include "ash/shelf/shelf.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/aura/window.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/widget/widget.h"

namespace ash {

class KeyboardOverlayDelegateTest
    : public AshTestBase,
      public testing::WithParamInterface<ShelfAlignment> {
 public:
  KeyboardOverlayDelegateTest() : shelf_alignment_(GetParam()) {}
  virtual ~KeyboardOverlayDelegateTest() = default;
  ShelfAlignment shelf_alignment() const { return shelf_alignment_; }

 private:
  ShelfAlignment shelf_alignment_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardOverlayDelegateTest);
};

// Verifies we can show and close the widget for the overlay dialog.
TEST_P(KeyboardOverlayDelegateTest, ShowAndClose) {
  UpdateDisplay("500x400,300x200");
  GetPrimaryShelf()->SetAlignment(shelf_alignment());
  KeyboardOverlayDelegate delegate(base::ASCIIToUTF16("Title"),
                                   GURL("chrome://keyboardoverlay/"));
  // Showing the dialog creates a widget.
  views::Widget* widget = delegate.Show(nullptr);
  EXPECT_TRUE(widget);

  // The widget is on the primary root window.
  EXPECT_EQ(Shell::GetPrimaryRootWindow(),
            widget->GetNativeWindow()->GetRootWindow());

  // The widget is horizontally and vertically centered in the work area.
  gfx::Rect work_area =
      display::Screen::GetScreen()->GetPrimaryDisplay().work_area();
  gfx::Rect bounds = widget->GetRestoredBounds();
  EXPECT_EQ(work_area.CenterPoint().x(), bounds.CenterPoint().x());
  EXPECT_EQ(work_area.y() + (work_area.height() - bounds.height()) / 2,
            bounds.y());

  // Clean up.
  widget->CloseNow();
}

// Tests run four times - for all possible values of shelf alignment
INSTANTIATE_TEST_CASE_P(ShelfAlignmentAny,
                        KeyboardOverlayDelegateTest,
                        testing::Values(SHELF_ALIGNMENT_BOTTOM,
                                        SHELF_ALIGNMENT_LEFT,
                                        SHELF_ALIGNMENT_RIGHT,
                                        SHELF_ALIGNMENT_BOTTOM_LOCKED));

}  // namespace ash
