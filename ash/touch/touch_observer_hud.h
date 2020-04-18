// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_TOUCH_TOUCH_OBSERVER_HUD_H_
#define ASH_TOUCH_TOUCH_OBSERVER_HUD_H_

#include <stdint.h>

#include <string>

#include "ash/ash_export.h"
#include "ash/display/window_tree_host_manager.h"
#include "base/macros.h"
#include "ui/display/display_observer.h"
#include "ui/display/manager/display_configurator.h"
#include "ui/events/event_handler.h"
#include "ui/views/widget/widget_observer.h"

namespace views {
class Widget;
}

namespace ash {

// An event filter which handles system level gesture events. Objects of this
// class manage their own lifetime.
class ASH_EXPORT TouchObserverHUD
    : public ui::EventHandler,
      public views::WidgetObserver,
      public display::DisplayObserver,
      public display::DisplayConfigurator::Observer,
      public WindowTreeHostManager::Observer {
 public:
  // Called to clear touch points and traces from the screen. Default
  // implementation does nothing. Sub-classes should implement appropriately.
  virtual void Clear();

  // Removes the HUD from the screen.
  void Remove();

  int64_t display_id() const { return display_id_; }

 protected:
  // |widget_name| is set on Widget::InitParams::name, and is used purely for
  // debugging.
  TouchObserverHUD(aura::Window* initial_root, const std::string& widget_name);

  ~TouchObserverHUD() override;

  virtual void SetHudForRootWindowController(
      RootWindowController* controller) = 0;
  virtual void UnsetHudForRootWindowController(
      RootWindowController* controller) = 0;

  views::Widget* widget() { return widget_; }

  // Overriden from ui::EventHandler.
  void OnTouchEvent(ui::TouchEvent* event) override;

  // Overridden from views::WidgetObserver.
  void OnWidgetDestroying(views::Widget* widget) override;

  // Overridden from display::DisplayObserver.
  void OnDisplayAdded(const display::Display& new_display) override;
  void OnDisplayRemoved(const display::Display& old_display) override;
  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t metrics) override;

  // Overriden from display::DisplayConfigurator::Observer.
  void OnDisplayModeChanged(
      const display::DisplayConfigurator::DisplayStateList& outputs) override;

  // Overriden form WindowTreeHostManager::Observer.
  void OnDisplaysInitialized() override;
  void OnDisplayConfigurationChanging() override;
  void OnDisplayConfigurationChanged() override;

 private:
  friend class TouchHudTestBase;

  const int64_t display_id_;
  aura::Window* root_window_;

  views::Widget* widget_;

  DISALLOW_COPY_AND_ASSIGN(TouchObserverHUD);
};

}  // namespace ash

#endif  // ASH_TOUCH_TOUCH_OBSERVER_HUD_H_
