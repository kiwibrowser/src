// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/menu/menu_pre_target_handler.h"

#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/views/controls/menu/menu_controller.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/public/activation_client.h"

namespace views {

namespace {

aura::Window* GetOwnerRootWindow(views::Widget* owner) {
  return owner ? owner->GetNativeWindow()->GetRootWindow() : nullptr;
}

}  // namespace

MenuPreTargetHandler::MenuPreTargetHandler(MenuController* controller,
                                           Widget* owner)
    : controller_(controller), root_(GetOwnerRootWindow(owner)) {
  aura::Env::GetInstanceDontCreate()->AddPreTargetHandler(
      this, ui::EventTarget::Priority::kSystem);
  if (root_) {
    wm::GetActivationClient(root_)->AddObserver(this);
    root_->AddObserver(this);
  }
}

MenuPreTargetHandler::~MenuPreTargetHandler() {
  aura::Env::GetInstanceDontCreate()->RemovePreTargetHandler(this);
  Cleanup();
}

void MenuPreTargetHandler::OnWindowActivated(
    wm::ActivationChangeObserver::ActivationReason reason,
    aura::Window* gained_active,
    aura::Window* lost_active) {
  if (!controller_->drag_in_progress())
    controller_->CancelAll();
}

void MenuPreTargetHandler::OnWindowDestroying(aura::Window* window) {
  Cleanup();
}

void MenuPreTargetHandler::OnCancelMode(ui::CancelModeEvent* event) {
  controller_->CancelAll();
}

void MenuPreTargetHandler::OnKeyEvent(ui::KeyEvent* event) {
  controller_->OnWillDispatchKeyEvent(event);
}

void MenuPreTargetHandler::Cleanup() {
  if (!root_)
    return;
  // The ActivationClient may have been destroyed by the time we get here.
  wm::ActivationClient* client = wm::GetActivationClient(root_);
  if (client)
    client->RemoveObserver(this);
  root_->RemoveObserver(this);
  root_ = nullptr;
}

}  // namespace views
