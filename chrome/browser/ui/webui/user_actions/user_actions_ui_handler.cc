// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/user_actions/user_actions_ui_handler.h"

#include "base/bind.h"
#include "base/metrics/user_metrics.h"
#include "base/values.h"
#include "content/public/browser/web_ui.h"

UserActionsUIHandler::UserActionsUIHandler()
    : action_callback_(base::Bind(&UserActionsUIHandler::OnUserAction,
                                  base::Unretained(this))) {
  base::AddActionCallback(action_callback_);
}

UserActionsUIHandler::~UserActionsUIHandler() {
  base::RemoveActionCallback(action_callback_);
}

void UserActionsUIHandler::RegisterMessages() {}

void UserActionsUIHandler::OnUserAction(const std::string& action) {
  base::Value user_action_name(action);
  web_ui()->CallJavascriptFunctionUnsafe("userActions.observeUserAction",
                                         user_action_name);
}

