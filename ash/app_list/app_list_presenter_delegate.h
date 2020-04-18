// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_APP_LIST_APP_LIST_PRESENTER_DELEGATE_H_
#define ASH_APP_LIST_APP_LIST_PRESENTER_DELEGATE_H_

#include <stdint.h>

#include "ash/app_list/presenter/app_list_presenter_delegate.h"
#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/events/event_handler.h"
#include "ui/keyboard/keyboard_controller_observer.h"

namespace app_list {
class AppListPresenterImpl;
class AppListView;
class AppListViewDelegateFactory;
}

namespace ui {
class LocatedEvent;
}

namespace ash {

// Non-Mus+ash implementation of AppListPresenterDelegate.
// Responsible for laying out the app list UI as well as updating the Shelf
// launch icon as the state of the app list changes. Listens to shell events
// and touches/mouse clicks outside the app list to auto dismiss the UI or
// update its layout as necessary.
class ASH_EXPORT AppListPresenterDelegate
    : public app_list::AppListPresenterDelegate,
      public ui::EventHandler {
 public:
  AppListPresenterDelegate(
      app_list::AppListPresenterImpl* presenter,
      app_list::AppListViewDelegateFactory* view_delegate_factory);
  ~AppListPresenterDelegate() override;

  // app_list::AppListPresenterDelegate:
  app_list::AppListViewDelegate* GetViewDelegate() override;
  void Init(app_list::AppListView* view,
            int64_t display_id,
            int current_apps_page) override;
  void OnShown(int64_t display_id) override;
  void OnDismissed() override;
  gfx::Vector2d GetVisibilityAnimationOffset(
      aura::Window* root_window) override;
  base::TimeDelta GetVisibilityAnimationDuration(aura::Window* root_window,
                                                 bool is_visible) override;

 private:
  void ProcessLocatedEvent(ui::LocatedEvent* event);

  // ui::EventHandler overrides:
  void OnMouseEvent(ui::MouseEvent* event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;

  // Whether the app list is visible (or in the process of being shown).
  bool is_visible_ = false;

  // Not owned. Pointer is guaranteed to be valid while this object is alive.
  app_list::AppListPresenterImpl* presenter_;

  // Not owned. Pointer is guaranteed to be valid while this object is alive.
  app_list::AppListViewDelegateFactory* view_delegate_factory_;

  // Owned by its widget.
  app_list::AppListView* view_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(AppListPresenterDelegate);
};

}  // namespace ash

#endif  // ASH_APP_LIST_APP_LIST_PRESENTER_DELEGATE_H_
