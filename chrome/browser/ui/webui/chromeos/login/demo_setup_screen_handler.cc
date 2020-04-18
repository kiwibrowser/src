// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/demo_setup_screen_handler.h"

#include "chrome/browser/chromeos/login/oobe_screen.h"
#include "chrome/browser/chromeos/login/screens/demo_setup_screen.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"

namespace {

constexpr char kJsScreenPath[] = "login.DemoSetupScreen";

}  // namespace

namespace chromeos {

DemoSetupScreenHandler::DemoSetupScreenHandler()
    : BaseScreenHandler(kScreenId) {
  set_call_js_prefix(kJsScreenPath);
}

DemoSetupScreenHandler::~DemoSetupScreenHandler() {
  if (screen_)
    screen_->OnViewDestroyed(this);
}

void DemoSetupScreenHandler::Show() {
  ShowScreen(kScreenId);
}

void DemoSetupScreenHandler::Hide() {}

void DemoSetupScreenHandler::Bind(DemoSetupScreen* screen) {
  screen_ = screen;
  BaseScreenHandler::SetBaseScreen(screen);
}

void DemoSetupScreenHandler::Initialize() {}

void DemoSetupScreenHandler::DeclareLocalizedValues(
    ::login::LocalizedValuesBuilder* builder) {}

}  // namespace chromeos
