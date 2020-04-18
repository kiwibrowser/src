// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_SCREEN_WIN_H_
#define UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_SCREEN_WIN_H_

#include "base/macros.h"
#include "ui/display/win/screen_win.h"
#include "ui/views/views_export.h"

namespace views {

class VIEWS_EXPORT DesktopScreenWin : public display::win::ScreenWin {
public:
  DesktopScreenWin();
  ~DesktopScreenWin() override;

 private:
  // Overridden from display::win::ScreenWin:
  display::Display GetDisplayMatching(
      const gfx::Rect& match_rect) const override;
  HWND GetHWNDFromNativeView(gfx::NativeView window) const override;
  gfx::NativeWindow GetNativeWindowFromHWND(HWND hwnd) const override;

  DISALLOW_COPY_AND_ASSIGN(DesktopScreenWin);
};

}  // namespace views

#endif  // UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_SCREEN_WIN_H_
