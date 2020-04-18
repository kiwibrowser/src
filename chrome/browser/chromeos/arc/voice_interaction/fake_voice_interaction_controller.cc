// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/voice_interaction/fake_voice_interaction_controller.h"

namespace arc {

FakeVoiceInteractionController::FakeVoiceInteractionController()
    : binding_(this) {}

FakeVoiceInteractionController::~FakeVoiceInteractionController() = default;

ash::mojom::VoiceInteractionControllerPtr
FakeVoiceInteractionController::CreateInterfacePtrAndBind() {
  ash::mojom::VoiceInteractionControllerPtr ptr;
  binding_.Bind(mojo::MakeRequest(&ptr));
  return ptr;
}

void FakeVoiceInteractionController::NotifyStatusChanged(
    ash::mojom::VoiceInteractionState state) {
  voice_interaction_state_ = state;
}

void FakeVoiceInteractionController::NotifySettingsEnabled(bool enabled) {
  voice_interaction_settings_enabled_ = enabled;
}

void FakeVoiceInteractionController::NotifyContextEnabled(bool enabled) {
  voice_interaction_context_enabled_ = enabled;
}

void FakeVoiceInteractionController::NotifySetupCompleted(bool completed) {
  voice_interaction_setup_completed_ = completed;
}

void FakeVoiceInteractionController::NotifyFeatureAllowed(
    ash::mojom::AssistantAllowedState state) {
  assistant_allowed_state_ = state;
}

}  // namespace arc
