// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/try_chrome_dialog_win/button_layout.h"

#include "base/logging.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/view.h"

ButtonLayout::ButtonLayout(int view_width) : view_width_(view_width) {}

ButtonLayout::~ButtonLayout() = default;

void ButtonLayout::Layout(views::View* host) {
  const gfx::Size& host_size = host->bounds().size();

  // Layout of the host within its parent must size it based on the |view_width|
  // given to this layout manager at creation. If it happens to be different,
  // the buttons will be sized and positioned based on the host's true size.
  // This will result in either stretching or compressing the buttons, and may
  // lead to elision of their text.
  DCHECK_EQ(host_size.width(), view_width_);

  const gfx::Size max_child_size = GetMaxChildPreferredSize(host);
  const bool use_wide_buttons =
      UseWideButtons(host_size.width(), max_child_size.width());
  const bool has_two_buttons = HasTwoButtons(host);

  // The buttons are all equal-sized.
  gfx::Size button_size(0, max_child_size.height());
  if (use_wide_buttons)
    button_size.set_width(host_size.width());
  else
    button_size.set_width((host_size.width() - kPaddingBetweenButtons) / 2);

  if (!use_wide_buttons) {
    // The offset of the right-side narrow button.
    int right_x = button_size.width() + kPaddingBetweenButtons;
    if (has_two_buttons) {
      host->child_at(0)->SetBoundsRect({{0, 0}, button_size});
      host->child_at(1)->SetBoundsRect({{right_x, 0}, button_size});
    } else {
      // If there is only one narrow button, position it on the right.
      host->child_at(0)->SetBoundsRect({{right_x, 0}, button_size});
    }
  } else {
    host->child_at(0)->SetBoundsRect({{0, 0}, button_size});
    if (has_two_buttons) {
      int bottom_y = button_size.height() + kPaddingBetweenButtons;
      host->child_at(1)->SetBoundsRect({{0, bottom_y}, button_size});
    }
  }
}

gfx::Size ButtonLayout::GetPreferredSize(const views::View* host) const {
  const gfx::Size max_child_size = GetMaxChildPreferredSize(host);

  // |view_width_| is a hard limit; the buttons will be sized and positioned to
  // fill it.
  if (HasTwoButtons(host) &&
      UseWideButtons(view_width_, max_child_size.width())) {
    // Two rows of equal height with padding between them.
    return {view_width_, max_child_size.height() * 2 + kPaddingBetweenButtons};
  }

  // Only one button or the widest of two is sufficiently narrow, so only one
  // row is needed.
  return {view_width_, max_child_size.height()};
}

// static
bool ButtonLayout::HasTwoButtons(const views::View* host) {
  const int child_count = host->child_count();
  DCHECK_GE(child_count, 1);
  DCHECK_LE(child_count, 2);
  return child_count == 2;
}

// static
gfx::Size ButtonLayout::GetMaxChildPreferredSize(const views::View* host) {
  const bool has_two_buttons = HasTwoButtons(host);
  gfx::Size max_child_size(host->child_at(0)->GetPreferredSize());
  if (has_two_buttons)
    max_child_size.SetToMax(host->child_at(1)->GetPreferredSize());
  return max_child_size;
}

// static
bool ButtonLayout::UseWideButtons(int host_width, int max_child_width) {
  return max_child_width > (host_width - kPaddingBetweenButtons) / 2;
}
