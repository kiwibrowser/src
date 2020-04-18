// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_JAVASCRIPT_APP_MODAL_EVENT_BLOCKER_X11_H_
#define CHROME_BROWSER_UI_VIEWS_JAVASCRIPT_APP_MODAL_EVENT_BLOCKER_X11_H_

#include "base/macros.h"
#include "ui/events/event_handler.h"

class BrowserView;

namespace aura {
class Window;
}

namespace ui {
class EventTarget;
}

// JavascriptAppModalEventBlockerX11 blocks events to all browser windows except
// the browser window which hosts |app_modal_window| for the duration of its
// lifetime. JavascriptAppModalEventBlockerX11 should not outlive
// |app_modal_window|.
// TODO(pkotwicz): Merge this class into WindowModalityController.
class JavascriptAppModalEventBlockerX11 : public ui::EventHandler {
 public:
  explicit JavascriptAppModalEventBlockerX11(aura::Window* app_modal_window);
  ~JavascriptAppModalEventBlockerX11() override;

 private:
  // Returns true if the propagation of events to |target| should be stopped.
  bool ShouldStopPropagationTo(ui::EventTarget* target);

  // ui::EventHandler:
  void OnKeyEvent(ui::KeyEvent* event) override;
  void OnMouseEvent(ui::MouseEvent* event) override;
  void OnScrollEvent(ui::ScrollEvent* event) override;
  void OnTouchEvent(ui::TouchEvent* event) override;

  // The app modal dialog.
  aura::Window* modal_window_;

  // The BrowserView which hosts the app modal dialog.
  BrowserView* browser_view_with_modal_dialog_;

  DISALLOW_COPY_AND_ASSIGN(JavascriptAppModalEventBlockerX11);
};

#endif  // CHROME_BROWSER_UI_VIEWS_JAVASCRIPT_APP_MODAL_EVENT_BLOCKER_X11_H_
