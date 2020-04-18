// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/app_menu_button.h"

#include <utility>

#include "chrome/browser/ui/toolbar/app_menu_model.h"
#include "chrome/browser/ui/views/toolbar/app_menu.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "ui/views/controls/menu/menu_listener.h"
#include "ui/views/style/platform_style.h"

AppMenuButton::AppMenuButton(views::MenuButtonListener* menu_button_listener)
    : views::MenuButton(base::string16(), menu_button_listener, false) {
  SetInstallFocusRingOnFocus(views::PlatformStyle::kPreferFocusRings);
}

AppMenuButton::~AppMenuButton() {}

SkColor AppMenuButton::GetInkDropBaseColor() const {
  return GetToolbarInkDropBaseColor(this);
}

void AppMenuButton::CloseMenu() {
  if (menu_)
    menu_->CloseMenu();
  menu_.reset();
}

bool AppMenuButton::IsMenuShowing() const {
  return menu_ && menu_->IsShowing();
}

void AppMenuButton::AddMenuListener(views::MenuListener* listener) {
  menu_listeners_.AddObserver(listener);
}

void AppMenuButton::RemoveMenuListener(views::MenuListener* listener) {
  menu_listeners_.RemoveObserver(listener);
}

void AppMenuButton::InitMenu(std::unique_ptr<AppMenuModel> menu_model,
                             Browser* browser,
                             int run_flags) {
  // |menu_| must be reset before |menu_model_| is destroyed, as per the comment
  // in the class declaration.
  menu_.reset();
  menu_model_ = std::move(menu_model);
  menu_model_->Init();
  menu_ = std::make_unique<AppMenu>(browser, run_flags);
  menu_->Init(menu_model_.get());

  for (views::MenuListener& observer : menu_listeners_)
    observer.OnMenuOpened();
}
