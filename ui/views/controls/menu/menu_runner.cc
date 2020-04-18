// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/menu/menu_runner.h"

#include <utility>

#include "ui/views/controls/menu/menu_runner_handler.h"
#include "ui/views/controls/menu/menu_runner_impl.h"
#include "ui/views/widget/widget.h"

namespace views {

MenuRunner::MenuRunner(ui::MenuModel* menu_model,
                       int32_t run_types,
                       const base::Closure& on_menu_closed_callback)
    : run_types_(run_types),
      impl_(
          internal::MenuRunnerImplInterface::Create(menu_model,
                                                    run_types,
                                                    on_menu_closed_callback)) {}

MenuRunner::MenuRunner(MenuItemView* menu_view, int32_t run_types)
    : run_types_(run_types), impl_(new internal::MenuRunnerImpl(menu_view)) {}

MenuRunner::~MenuRunner() {
  impl_->Release();
}

void MenuRunner::RunMenuAt(Widget* parent,
                           MenuButton* button,
                           const gfx::Rect& bounds,
                           MenuAnchorPosition anchor,
                           ui::MenuSourceType source_type) {
  // If we are shown on mouse press, we will eat the subsequent mouse down and
  // the parent widget will not be able to reset its state (it might have mouse
  // capture from the mouse down). So we clear its state here.
  if (parent && parent->GetRootView())
    parent->GetRootView()->SetMouseHandler(nullptr);

  if (runner_handler_.get()) {
    runner_handler_->RunMenuAt(parent, button, bounds, anchor, source_type,
                               run_types_);
    return;
  }

  // The parent of the nested menu will have created a DisplayChangeListener, so
  // we avoid creating a DisplayChangeListener if nested. Drop menus are
  // transient, so we don't cancel in that case.
  if ((run_types_ & (IS_NESTED | FOR_DROP)) == 0 && parent) {
    display_change_listener_.reset(
        internal::DisplayChangeListener::Create(parent, this));
  }

  if ((run_types_ & CONTEXT_MENU) && !(run_types_ & FIXED_ANCHOR)) {
    switch (source_type) {
      case ui::MENU_SOURCE_NONE:
      case ui::MENU_SOURCE_KEYBOARD:
      case ui::MENU_SOURCE_MOUSE:
        anchor = MENU_ANCHOR_TOPLEFT;
        break;
      case ui::MENU_SOURCE_TOUCH:
      case ui::MENU_SOURCE_TOUCH_EDIT_MENU:
        anchor = MENU_ANCHOR_BOTTOMCENTER;
        break;
      default:
        break;
    }
  }

  impl_->RunMenuAt(parent, button, bounds, anchor, run_types_);
}

bool MenuRunner::IsRunning() const {
  return impl_->IsRunning();
}

void MenuRunner::Cancel() {
  impl_->Cancel();
}

base::TimeTicks MenuRunner::closing_event_time() const {
  return impl_->GetClosingEventTime();
}

void MenuRunner::SetRunnerHandler(
    std::unique_ptr<MenuRunnerHandler> runner_handler) {
  runner_handler_ = std::move(runner_handler);
}

}  // namespace views
