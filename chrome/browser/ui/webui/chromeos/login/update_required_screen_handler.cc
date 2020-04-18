// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/update_required_screen_handler.h"

#include <memory>

#include "base/values.h"
#include "chrome/browser/chromeos/login/oobe_screen.h"
#include "chrome/browser/chromeos/login/screens/update_required_screen.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/login/localized_values_builder.h"

namespace {

const char kJsScreenPath[] = "login.UpdateRequiredScreen";

}  // namespace

namespace chromeos {

UpdateRequiredScreenHandler::UpdateRequiredScreenHandler()
    : BaseScreenHandler(kScreenId) {
  set_call_js_prefix(kJsScreenPath);
}

UpdateRequiredScreenHandler::~UpdateRequiredScreenHandler() {
  if (screen_)
    screen_->OnViewDestroyed(this);
}

void UpdateRequiredScreenHandler::DeclareLocalizedValues(
    ::login::LocalizedValuesBuilder* builder) {
  builder->Add("updateRequiredMessage",
               IDS_UPDATE_REQUIRED_LOGIN_SCREEN_MESSAGE);
}

void UpdateRequiredScreenHandler::Initialize() {
  if (show_on_init_) {
    Show();
    show_on_init_ = false;
  }
}

void UpdateRequiredScreenHandler::Show() {
  if (!page_is_ready()) {
    show_on_init_ = true;
    return;
  }
  ShowScreen(kScreenId);
}

void UpdateRequiredScreenHandler::Hide() {}

void UpdateRequiredScreenHandler::Bind(UpdateRequiredScreen* screen) {
  screen_ = screen;
  BaseScreenHandler::SetBaseScreen(screen_);
}

void UpdateRequiredScreenHandler::Unbind() {
  screen_ = nullptr;
  BaseScreenHandler::SetBaseScreen(nullptr);
}

}  // namespace chromeos
