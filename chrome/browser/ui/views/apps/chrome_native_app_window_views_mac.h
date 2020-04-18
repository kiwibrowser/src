// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_APPS_CHROME_NATIVE_APP_WINDOW_VIEWS_MAC_H_
#define CHROME_BROWSER_UI_VIEWS_APPS_CHROME_NATIVE_APP_WINDOW_VIEWS_MAC_H_

#import <Foundation/Foundation.h>

#import "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "chrome/browser/ui/views/apps/chrome_native_app_window_views.h"

@class ResizeNotificationObserver;

// Mac-specific parts of ChromeNativeAppWindowViews.
class ChromeNativeAppWindowViewsMac : public ChromeNativeAppWindowViews {
 public:
  ChromeNativeAppWindowViewsMac();
  ~ChromeNativeAppWindowViewsMac() override;

  // Called by |nswindow_observer_| for window resize events.
  void OnWindowWillStartLiveResize();
  void OnWindowWillExitFullScreen();
  void OnWindowDidExitFullScreen();

 protected:
  // ChromeNativeAppWindowViews implementation.
  void OnBeforeWidgetInit(
      const extensions::AppWindow::CreateParams& create_params,
      views::Widget::InitParams* init_params,
      views::Widget* widget) override;
  views::NonClientFrameView* CreateStandardDesktopAppFrame() override;
  views::NonClientFrameView* CreateNonStandardAppFrame() override;

  // ui::BaseWindow implementation.
  bool IsMaximized() const override;
  gfx::Rect GetRestoredBounds() const override;
  void Show() override;
  void ShowInactive() override;
  void Activate() override;
  void Maximize() override;
  void Restore() override;
  void FlashFrame(bool flash) override;

  // WidgetObserver implementation.
  void OnWidgetCreated(views::Widget* widget) override;

  // NativeAppWindow implementation.
  // These are used to simulate Mac-style hide/show. Since windows can be hidden
  // and shown using the app.window API, this sets is_hidden_with_app_ to
  // differentiate the reason a window was hidden.
  void ShowWithApp() override;
  void HideWithApp() override;

 private:
  // Unset is_hidden_with_app_ and tell the shim to unhide.
  void UnhideWithoutActivation();

  // Used to notify us about certain NSWindow events.
  base::scoped_nsobject<ResizeNotificationObserver> nswindow_observer_;

  // The bounds of the window just before it was last maximized.
  NSRect bounds_before_maximize_;

  // Whether this window last became hidden due to a request to hide the entire
  // app, e.g. via the dock menu or Cmd+H. This is set by Hide/ShowWithApp.
  bool is_hidden_with_app_ = false;

  // Set true during an exit fullscreen transition, so that the live resize
  // event AppKit sends can be distinguished from a zoom-triggered live resize.
  bool in_fullscreen_transition_ = false;

  DISALLOW_COPY_AND_ASSIGN(ChromeNativeAppWindowViewsMac);
};

#endif  // CHROME_BROWSER_UI_VIEWS_APPS_CHROME_NATIVE_APP_WINDOW_VIEWS_MAC_H_
