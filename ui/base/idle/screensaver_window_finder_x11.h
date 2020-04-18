// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IDLE_SCREENSAVER_WINDOW_FINDER_X11_H_
#define UI_BASE_IDLE_SCREENSAVER_WINDOW_FINDER_X11_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ui/base/x/x11_util.h"

namespace ui {

class ScreensaverWindowFinder : public ui::EnumerateWindowsDelegate {
 public:
  static bool ScreensaverWindowExists();

 protected:
  bool ShouldStopIterating(XID window) override;

 private:
  ScreensaverWindowFinder();

  bool IsScreensaverWindow(XID window) const;

  bool exists_;

  DISALLOW_COPY_AND_ASSIGN(ScreensaverWindowFinder);
};

}  // namespace ui

#endif  // UI_BASE_IDLE_SCREENSAVER_WINDOW_FINDER_X11_H_
