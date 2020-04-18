// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_VOICE_INTERACTION_TEST_VOICE_INTERACTION_CONTROLLER_H_
#define CHROME_BROWSER_CHROMEOS_ARC_VOICE_INTERACTION_TEST_VOICE_INTERACTION_CONTROLLER_H_

#include "ash/public/interfaces/voice_interaction_controller.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace arc {

class FakeVoiceInteractionController
    : public ash::mojom::VoiceInteractionController {
 public:
  FakeVoiceInteractionController();
  ~FakeVoiceInteractionController() override;

  ash::mojom::VoiceInteractionControllerPtr CreateInterfacePtrAndBind();

  // ash::mojom::VoiceInteractionController:
  void NotifyStatusChanged(ash::mojom::VoiceInteractionState state) override;
  void NotifySettingsEnabled(bool enabled) override;
  void NotifyContextEnabled(bool enabled) override;
  void NotifySetupCompleted(bool completed) override;
  void NotifyFeatureAllowed(ash::mojom::AssistantAllowedState state) override;

  ash::mojom::VoiceInteractionState voice_interaction_state() const {
    return voice_interaction_state_;
  }
  bool voice_interaction_settings_enabled() const {
    return voice_interaction_settings_enabled_;
  }
  bool voice_interaction_context_enabled() const {
    return voice_interaction_context_enabled_;
  }
  bool voice_interaction_setup_completed() const {
    return voice_interaction_setup_completed_;
  }
  ash::mojom::AssistantAllowedState assistant_allowed_state() const {
    return assistant_allowed_state_;
  }

 private:
  ash::mojom::VoiceInteractionState voice_interaction_state_ =
      ash::mojom::VoiceInteractionState::STOPPED;
  bool voice_interaction_settings_enabled_ = false;
  bool voice_interaction_context_enabled_ = false;
  bool voice_interaction_setup_completed_ = false;
  ash::mojom::AssistantAllowedState assistant_allowed_state_ =
      ash::mojom::AssistantAllowedState::DISALLOWED_BY_INCOGNITO;

  mojo::Binding<ash::mojom::VoiceInteractionController> binding_;

  DISALLOW_COPY_AND_ASSIGN(FakeVoiceInteractionController);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_VOICE_INTERACTION_TEST_VOICE_INTERACTION_CONTROLLER_H_
