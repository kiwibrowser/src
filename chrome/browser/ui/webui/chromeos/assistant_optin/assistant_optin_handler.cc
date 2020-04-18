// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/assistant_optin/assistant_optin_handler.h"

#include "chrome/browser/browser_process.h"
#include "chrome/grit/generated_resources.h"
#include "components/login/localized_values_builder.h"

namespace {

constexpr char kJsScreenPath[] = "assistantOptin";

}  // namespace

namespace chromeos {

AssistantOptInHandler::AssistantOptInHandler(
    JSCallsContainer* js_calls_container)
    : BaseWebUIHandler(js_calls_container) {
  DCHECK(js_calls_container);
  set_call_js_prefix(kJsScreenPath);
}

AssistantOptInHandler::~AssistantOptInHandler() = default;

void AssistantOptInHandler::DeclareLocalizedValues(
    ::login::LocalizedValuesBuilder* builder) {}

void AssistantOptInHandler::RegisterMessages() {
  AddCallback("initialized", &AssistantOptInHandler::HandleInitialized);
}

void AssistantOptInHandler::Initialize() {}

void AssistantOptInHandler::ReloadContent(const base::DictionaryValue& dict) {
  CallJSOrDefer("reloadContent", dict);
}

void AssistantOptInHandler::AddSettingZippy(const base::ListValue& data) {
  CallJSOrDefer("addSettingZippy", data);
}

void AssistantOptInHandler::HandleInitialized() {
  ExecuteDeferredJSCalls();
}

}  // namespace chromeos
