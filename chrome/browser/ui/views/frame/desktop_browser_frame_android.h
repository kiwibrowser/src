// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_FRAME_ANDROID_H_
#define CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_FRAME_ANDROID_H_

#include "base/macros.h"
#include "chrome/browser/ui/views/frame/native_browser_frame.h"

class BrowserFrame;
class BrowserView;

// BrowserFrameAndroid provides the frame for Chrome browser windows on Chrome OS
// under classic ash.
class BrowserFrameAndroid : public NativeBrowserFrame {
 public:
  BrowserFrameAndroid(BrowserFrame* browser_frame, BrowserView* browser_view);

 protected:
  ~BrowserFrameAndroid() override;

  // Overridden from NativeBrowserFrame:
  views::Widget::InitParams GetWidgetParams() override;
  bool UseCustomFrame() const override;
  bool UsesNativeSystemMenu() const override;
  int GetMinimizeButtonOffset() const override;
  bool ShouldSaveWindowPlacement() const override;
  void GetWindowPlacement(gfx::Rect* bounds,
                          ui::WindowShowState* show_state) const override;
#if 0
  content::KeyboardEventProcessingResult PreHandleKeyboardEvent(
      const content::NativeWebKeyboardEvent& event) override;
#endif
  bool HandleKeyboardEvent(
      const content::NativeWebKeyboardEvent& event) override;

 private:
  // Set the window into the auto managed mode.
  void SetWindowAutoManaged();

  DISALLOW_COPY_AND_ASSIGN(BrowserFrameAndroid);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_FRAME_ANDROID_H_
