// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/test/fake_voice_interaction_framework_instance.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"

namespace arc {

FakeVoiceInteractionFrameworkInstance::FakeVoiceInteractionFrameworkInstance() =
    default;

FakeVoiceInteractionFrameworkInstance::
    ~FakeVoiceInteractionFrameworkInstance() = default;

void FakeVoiceInteractionFrameworkInstance::InitDeprecated(
    mojom::VoiceInteractionFrameworkHostPtr host_ptr) {
  Init(std::move(host_ptr), base::DoNothing());
}

void FakeVoiceInteractionFrameworkInstance::Init(
    mojom::VoiceInteractionFrameworkHostPtr host_ptr,
    InitCallback callback) {
  host_ = std::move(host_ptr);
  std::move(callback).Run();
}

void FakeVoiceInteractionFrameworkInstance::StartVoiceInteractionSession(
    bool homescreen_is_active) {
  start_session_count_++;
  state_ = arc::mojom::VoiceInteractionState::RUNNING;
  host_->SetVoiceInteractionState(state_);
}

void FakeVoiceInteractionFrameworkInstance::ToggleVoiceInteractionSession(
    bool homescreen_is_active) {
  toggle_session_count_++;
  if (state_ == arc::mojom::VoiceInteractionState::RUNNING)
    state_ = arc::mojom::VoiceInteractionState::STOPPED;
  else
    state_ = arc::mojom::VoiceInteractionState::RUNNING;

  host_->SetVoiceInteractionState(state_);
}

void FakeVoiceInteractionFrameworkInstance::
    StartVoiceInteractionSessionForRegion(const gfx::Rect& region) {
  start_session_for_region_count_++;
  selected_region_ = region;
}

void FakeVoiceInteractionFrameworkInstance::SetMetalayerVisibility(
    bool visible) {
  set_metalayer_visibility_count_++;
  metalayer_visible_ = visible;
}

void FakeVoiceInteractionFrameworkInstance::SetVoiceInteractionEnabled(
    bool enable,
    SetVoiceInteractionEnabledCallback callback) {
  std::move(callback).Run();
}

void FakeVoiceInteractionFrameworkInstance::SetVoiceInteractionContextEnabled(
    bool enable) {}

void FakeVoiceInteractionFrameworkInstance::StartVoiceInteractionSetupWizard() {
  setup_wizard_count_++;
}

void FakeVoiceInteractionFrameworkInstance::ShowVoiceInteractionSettings() {
  show_settings_count_++;
}

void FakeVoiceInteractionFrameworkInstance::GetVoiceInteractionSettings(
    GetVoiceInteractionSettingsCallback callback) {}

void FakeVoiceInteractionFrameworkInstance::FlushMojoForTesting() {
  host_.FlushForTesting();
}

}  // namespace arc
