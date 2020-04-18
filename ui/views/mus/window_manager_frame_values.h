// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_MUS_WINDOW_MANAGER_FRAME_VALUES_H_
#define UI_VIEWS_MUS_WINDOW_MANAGER_FRAME_VALUES_H_

#include "ui/gfx/geometry/insets.h"
#include "ui/views/mus/mus_export.h"

namespace views {

// Provides constants used by the window manager in rendering frame
// decorations.
struct VIEWS_MUS_EXPORT WindowManagerFrameValues {
  WindowManagerFrameValues();
  ~WindowManagerFrameValues();

  static void SetInstance(const WindowManagerFrameValues& values);
  static const WindowManagerFrameValues& instance();

  bool operator==(const WindowManagerFrameValues& other) const {
    return normal_insets == other.normal_insets &&
           maximized_insets == other.maximized_insets &&
           max_title_bar_button_width == other.max_title_bar_button_width;
  }

  bool operator!=(const WindowManagerFrameValues& other) const {
    return !(*this == other);
  }

  // Ideal insets the window manager renders non-client frame decorations into.
  gfx::Insets normal_insets;
  gfx::Insets maximized_insets;

  // Max width of buttons in the title bar. This width assumes all buttons are
  // present.
  int max_title_bar_button_width;
};

}  // namespace views

#endif  // UI_VIEWS_MUS_WINDOW_MANAGER_FRAME_VALUES_H_
