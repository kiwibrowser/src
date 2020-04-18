// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/settings/chromeos/google_assistant_handler.h"

#include <utility>

#include "base/bind.h"
#include "base/values.h"
#include "chrome/browser/chromeos/arc/voice_interaction/arc_voice_interaction_framework_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/arc/arc_service_manager.h"
#include "ui/gfx/geometry/rect.h"

namespace chromeos {
namespace settings {

GoogleAssistantHandler::GoogleAssistantHandler(Profile* profile)
    : profile_(profile) {}

GoogleAssistantHandler::~GoogleAssistantHandler() {}

void GoogleAssistantHandler::OnJavascriptAllowed() {}
void GoogleAssistantHandler::OnJavascriptDisallowed() {}

void GoogleAssistantHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "setGoogleAssistantEnabled",
      base::BindRepeating(
          &GoogleAssistantHandler::HandleSetGoogleAssistantEnabled,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setGoogleAssistantContextEnabled",
      base::BindRepeating(
          &GoogleAssistantHandler::HandleSetGoogleAssistantContextEnabled,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "showGoogleAssistantSettings",
      base::BindRepeating(
          &GoogleAssistantHandler::HandleShowGoogleAssistantSettings,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "turnOnGoogleAssistant",
      base::BindRepeating(&GoogleAssistantHandler::HandleTurnOnGoogleAssistant,
                          base::Unretained(this)));
}

void GoogleAssistantHandler::HandleSetGoogleAssistantEnabled(
    const base::ListValue* args) {
  CHECK_EQ(1U, args->GetSize());
  bool enabled;
  CHECK(args->GetBoolean(0, &enabled));

  auto* service =
      arc::ArcVoiceInteractionFrameworkService::GetForBrowserContext(profile_);
  if (service)
    service->SetVoiceInteractionEnabled(enabled, base::BindOnce([](bool) {}));
}

void GoogleAssistantHandler::HandleSetGoogleAssistantContextEnabled(
    const base::ListValue* args) {
  CHECK_EQ(1U, args->GetSize());
  bool enabled;
  CHECK(args->GetBoolean(0, &enabled));

  auto* service =
      arc::ArcVoiceInteractionFrameworkService::GetForBrowserContext(profile_);
  if (service)
    service->SetVoiceInteractionContextEnabled(enabled);
}

void GoogleAssistantHandler::HandleShowGoogleAssistantSettings(
    const base::ListValue* args) {
  auto* service =
      arc::ArcVoiceInteractionFrameworkService::GetForBrowserContext(profile_);
  if (service)
    service->ShowVoiceInteractionSettings();
}

void GoogleAssistantHandler::HandleTurnOnGoogleAssistant(
    const base::ListValue* args) {
  auto* service =
      arc::ArcVoiceInteractionFrameworkService::GetForBrowserContext(profile_);
  if (service)
    service->StartSessionFromUserInteraction(gfx::Rect());
}

}  // namespace settings
}  // namespace chromeos
