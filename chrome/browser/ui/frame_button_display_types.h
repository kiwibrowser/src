// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_FRAME_BUTTON_DISPLAY_TYPES_H_
#define CHROME_BROWSER_UI_FRAME_BUTTON_DISPLAY_TYPES_H_

namespace chrome {

// This enum is similar to views::FrameButton, except it partitions
// kMaximize and kRestore.  This is useful for when we want to be
// explicit about which buttons we want drawn, without having to
// implicitly determine if we should use kMaximize or kRestore
// depending on the browser window's maximized/restored state.
enum class FrameButtonDisplayType {
  kMinimize,
  kMaximize,
  kRestore,
  kClose,
};

}  // namespace chrome

#endif  // CHROME_BROWSER_UI_FRAME_BUTTON_DISPLAY_TYPES_H_
