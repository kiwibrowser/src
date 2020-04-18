// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_MENU_PRE_TARGET_HANDLER_H_
#define UI_VIEWS_CONTROLS_MENU_PRE_TARGET_HANDLER_H_

#include "ui/aura/window_observer.h"
#include "ui/events/event_handler.h"
#include "ui/views/views_export.h"
#include "ui/wm/public/activation_change_observer.h"

namespace aura {
class Window;
}  // namespace aura

namespace views {

class MenuController;
class Widget;

// MenuPreTargetHandler is used to observe activation changes, cancel events,
// and root window destruction, to shutdown the menu.
//
// Additionally handles key events to provide accelerator support to menus.
class VIEWS_EXPORT MenuPreTargetHandler : public wm::ActivationChangeObserver,
                                          public aura::WindowObserver,
                                          public ui::EventHandler {
 public:
  MenuPreTargetHandler(MenuController* controller, Widget* owner);
  ~MenuPreTargetHandler() override;

  // aura::client:ActivationChangeObserver:
  void OnWindowActivated(wm::ActivationChangeObserver::ActivationReason reason,
                         aura::Window* gained_active,
                         aura::Window* lost_active) override;

  // aura::WindowObserver:
  void OnWindowDestroying(aura::Window* window) override;

  // ui::EventHandler:
  void OnCancelMode(ui::CancelModeEvent* event) override;
  void OnKeyEvent(ui::KeyEvent* event) override;

 private:
  void Cleanup();

  MenuController* controller_;
  aura::Window* root_;

  DISALLOW_COPY_AND_ASSIGN(MenuPreTargetHandler);
};

}  // namespace views

#endif  // UI_VIEWS_CONTROLS_MENU_PRE_TARGET_HANDLER_H_
