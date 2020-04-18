// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/window_open_disposition.h"

#include "build/build_config.h"
#include "ui/events/event_constants.h"

namespace ui {

WindowOpenDisposition DispositionFromClick(bool middle_button,
                                           bool alt_key,
                                           bool ctrl_key,
                                           bool meta_key,
                                           bool shift_key) {
  // MacOS uses meta key (Command key) to spawn new tabs.
#if defined(OS_MACOSX)
  if (middle_button || meta_key)
#else
  if (middle_button || ctrl_key)
#endif
    return shift_key ? WindowOpenDisposition::NEW_FOREGROUND_TAB
                     : WindowOpenDisposition::NEW_BACKGROUND_TAB;
  if (shift_key)
    return WindowOpenDisposition::NEW_WINDOW;
  if (alt_key)
    return WindowOpenDisposition::SAVE_TO_DISK;
  return WindowOpenDisposition::CURRENT_TAB;
}

WindowOpenDisposition DispositionFromEventFlags(int event_flags) {
  return DispositionFromClick(
      (event_flags & ui::EF_MIDDLE_MOUSE_BUTTON) != 0,
      (event_flags & ui::EF_ALT_DOWN) != 0,
      (event_flags & ui::EF_CONTROL_DOWN) != 0,
      (event_flags & ui::EF_COMMAND_DOWN) != 0,
      (event_flags & ui::EF_SHIFT_DOWN) != 0);
}

}  // namespace ui
