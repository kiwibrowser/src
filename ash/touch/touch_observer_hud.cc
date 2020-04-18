// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/touch/touch_observer_hud.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/root_window_controller.h"
#include "ash/root_window_settings.h"
#include "ash/shell.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/widget/widget.h"

namespace ash {

TouchObserverHUD::TouchObserverHUD(aura::Window* initial_root,
                                   const std::string& widget_name)
    : display_id_(GetRootWindowSettings(initial_root)->display_id),
      root_window_(initial_root),
      widget_(NULL) {
  const display::Display& display =
      Shell::Get()->display_manager()->GetDisplayForId(display_id_);

  views::View* content = new views::View;

  const gfx::Size& display_size = display.size();
  content->SetSize(display_size);

  widget_ = new views::Widget();
  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
  params.activatable = views::Widget::InitParams::ACTIVATABLE_NO;
  params.accept_events = false;
  params.bounds = display.bounds();
  params.parent =
      Shell::GetContainer(root_window_, kShellWindowId_OverlayContainer);
  params.name = widget_name;
  widget_->Init(params);
  widget_->SetContentsView(content);
  widget_->StackAtTop();
  widget_->Show();

  widget_->AddObserver(this);

  // Observe changes in display size and mode to update touch HUD.
  display::Screen::GetScreen()->AddObserver(this);
  Shell::Get()->display_configurator()->AddObserver(this);
  Shell::Get()->window_tree_host_manager()->AddObserver(this);
  root_window_->AddPreTargetHandler(this);
}

TouchObserverHUD::~TouchObserverHUD() {
  Shell::Get()->window_tree_host_manager()->RemoveObserver(this);
  Shell::Get()->display_configurator()->RemoveObserver(this);
  display::Screen::GetScreen()->RemoveObserver(this);

  widget_->RemoveObserver(this);
}

void TouchObserverHUD::Clear() {}

void TouchObserverHUD::Remove() {
  root_window_->RemovePreTargetHandler(this);

  RootWindowController* controller =
      RootWindowController::ForWindow(root_window_);
  UnsetHudForRootWindowController(controller);

  widget_->CloseNow();
}

void TouchObserverHUD::OnTouchEvent(ui::TouchEvent* /*event*/) {}

void TouchObserverHUD::OnWidgetDestroying(views::Widget* widget) {
  DCHECK_EQ(widget, widget_);
  delete this;
}

void TouchObserverHUD::OnDisplayAdded(const display::Display& new_display) {}

void TouchObserverHUD::OnDisplayRemoved(const display::Display& old_display) {
  if (old_display.id() != display_id_)
    return;
  widget_->CloseNow();
}

void TouchObserverHUD::OnDisplayMetricsChanged(const display::Display& display,
                                               uint32_t metrics) {
  if (display.id() != display_id_ || !(metrics & DISPLAY_METRIC_BOUNDS))
    return;

  widget_->SetSize(display.size());
}

void TouchObserverHUD::OnDisplayModeChanged(
    const display::DisplayConfigurator::DisplayStateList& outputs) {
  // Clear touch HUD for any change in display mode (single, dual extended, dual
  // mirrored, ...).
  Clear();
}

void TouchObserverHUD::OnDisplaysInitialized() {
  OnDisplayConfigurationChanged();
}

void TouchObserverHUD::OnDisplayConfigurationChanging() {
  if (!root_window_)
    return;

  root_window_->RemovePreTargetHandler(this);

  RootWindowController* controller =
      RootWindowController::ForWindow(root_window_);
  UnsetHudForRootWindowController(controller);

  views::Widget::ReparentNativeView(
      widget_->GetNativeView(),
      Shell::GetContainer(root_window_,
                          kShellWindowId_UnparentedControlContainer));

  root_window_ = NULL;
}

void TouchObserverHUD::OnDisplayConfigurationChanged() {
  if (root_window_)
    return;

  root_window_ = Shell::GetRootWindowForDisplayId(display_id_);

  views::Widget::ReparentNativeView(
      widget_->GetNativeView(),
      Shell::GetContainer(root_window_, kShellWindowId_OverlayContainer));

  RootWindowController* controller =
      RootWindowController::ForWindow(root_window_);
  SetHudForRootWindowController(controller);

  root_window_->AddPreTargetHandler(this);
}

}  // namespace ash
