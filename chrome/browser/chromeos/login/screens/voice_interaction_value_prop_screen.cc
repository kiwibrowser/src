// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/voice_interaction_value_prop_screen.h"

#include "chrome/browser/chromeos/arc/voice_interaction/arc_voice_interaction_arc_home_service.h"
#include "chrome/browser/chromeos/login/screens/base_screen_delegate.h"
#include "chrome/browser/chromeos/login/screens/voice_interaction_value_prop_screen_view.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"

namespace chromeos {
namespace {

constexpr const char kUserActionSkipPressed[] = "skip-pressed";
constexpr const char kUserActionNextPressed[] = "next-pressed";

}  // namespace

VoiceInteractionValuePropScreen::VoiceInteractionValuePropScreen(
    BaseScreenDelegate* base_screen_delegate,
    VoiceInteractionValuePropScreenView* view)
    : BaseScreen(base_screen_delegate,
                 OobeScreen::SCREEN_VOICE_INTERACTION_VALUE_PROP),
      view_(view) {
  DCHECK(view_);
  if (view_)
    view_->Bind(this);
}

VoiceInteractionValuePropScreen::~VoiceInteractionValuePropScreen() {
  if (view_)
    view_->Unbind();
}

void VoiceInteractionValuePropScreen::Show() {
  if (!view_)
    return;

  view_->Show();
  GetVoiceInteractionHomeService()->OnAssistantStarted();
}

void VoiceInteractionValuePropScreen::Hide() {
  if (view_)
    view_->Hide();
}

void VoiceInteractionValuePropScreen::OnViewDestroyed(
    VoiceInteractionValuePropScreenView* view) {
  if (view_ == view)
    view_ = nullptr;
}

void VoiceInteractionValuePropScreen::OnUserAction(
    const std::string& action_id) {
  if (action_id == kUserActionSkipPressed)
    OnSkipPressed();
  else if (action_id == kUserActionNextPressed)
    OnNextPressed();
  else
    BaseScreen::OnUserAction(action_id);
}

void VoiceInteractionValuePropScreen::OnSkipPressed() {
  GetVoiceInteractionHomeService()->OnAssistantCanceled();
  Finish(ScreenExitCode::VOICE_INTERACTION_VALUE_PROP_SKIPPED);
}

void VoiceInteractionValuePropScreen::OnNextPressed() {
  GetVoiceInteractionHomeService()->OnAssistantAppRequested();
  Finish(ScreenExitCode::VOICE_INTERACTION_VALUE_PROP_ACCEPTED);
}

arc::ArcVoiceInteractionArcHomeService*
VoiceInteractionValuePropScreen::GetVoiceInteractionHomeService() {
  Profile* const profile = ProfileManager::GetActiveUserProfile();
  DCHECK(profile);
  arc::ArcVoiceInteractionArcHomeService* const home_service =
      arc::ArcVoiceInteractionArcHomeService::GetForBrowserContext(profile);
  DCHECK(home_service);
  return home_service;
}

}  // namespace chromeos
