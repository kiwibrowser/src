// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/webui/version_handler.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/values.h"
#include "components/version_ui/version_handler_helper.h"
#include "components/version_ui/version_ui_constants.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "ui/base/l10n/l10n_util.h"

VersionHandler::VersionHandler() {}

VersionHandler::~VersionHandler() {}

void VersionHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      version_ui::kRequestVersionInfo,
      base::BindRepeating(&VersionHandler::HandleRequestVersionInfo,
                          base::Unretained(this)));
}

void VersionHandler::HandleRequestVersionInfo(const base::ListValue* args) {
  // Respond with the variations info immediately.
  web_ui()->CallJavascriptFunction(version_ui::kReturnVariationInfo,
                                   *version_ui::GetVariationsList());
}
