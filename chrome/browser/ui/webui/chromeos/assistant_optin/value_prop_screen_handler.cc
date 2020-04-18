// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/assistant_optin/value_prop_screen_handler.h"

#include "chrome/browser/browser_process.h"
#include "chrome/grit/generated_resources.h"
#include "components/login/localized_values_builder.h"

namespace {

const char kJsScreenPath[] = "AssistantValuePropScreen";

constexpr const char kUserActionSkipPressed[] = "skip-pressed";
constexpr const char kUserActionNextPressed[] = "next-pressed";

}  // namespace

namespace chromeos {

ValuePropScreenHandler::ValuePropScreenHandler(
    OnAssistantOptInScreenExitCallback callback)
    : BaseWebUIHandler(), exit_callback_(std::move(callback)) {
  set_call_js_prefix(kJsScreenPath);
}

ValuePropScreenHandler::~ValuePropScreenHandler() = default;

void ValuePropScreenHandler::DeclareLocalizedValues(
    ::login::LocalizedValuesBuilder* builder) {
  builder->Add("locale", g_browser_process->GetApplicationLocale());
  // TODO(updowndota) Replace this with new string constants.
  // Use string constant for old flow for now before we have final UX.
  builder->Add("valuePropLoading", IDS_VOICE_INTERACTION_VALUE_PROP_LOADING);
  builder->Add("valuePropLoadErrorTitle",
               IDS_VOICE_INTERACTION_VALUE_PROP_LOAD_ERROR_TITLE);
  builder->Add("valuePropLoadErrorMessage",
               IDS_VOICE_INTERACTION_VALUE_PROP_LOAD_ERROR_MESSAGE);
  builder->Add("valuePropRetryButton",
               IDS_VOICE_INTERACTION_VALUE_PROP_RETRY_BUTTON);
  builder->Add("valuePropMoreButton",
               IDS_VOICE_INTERACTION_VALUE_PROP_MORE_BUTTION);
  builder->Add("back", IDS_EULA_BACK_BUTTON);
  builder->Add("next", IDS_EULA_NEXT_BUTTON);
}

void ValuePropScreenHandler::RegisterMessages() {
  AddPrefixedCallback("userActed", &ValuePropScreenHandler::HandleUserAction);
}

void ValuePropScreenHandler::Initialize() {}

void ValuePropScreenHandler::HandleUserAction(const std::string& action) {
  DCHECK(exit_callback_);
  if (action == kUserActionSkipPressed)
    std::move(exit_callback_)
        .Run(AssistantOptInScreenExitCode::VALUE_PROP_SKIPPED);
  else if (action == kUserActionNextPressed)
    std::move(exit_callback_)
        .Run(AssistantOptInScreenExitCode::VALUE_PROP_ACCEPTED);
}

}  // namespace chromeos
