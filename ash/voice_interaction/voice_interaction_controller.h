// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_VOICE_INTERACTION_VOICE_INTERACTION_CONTROLLER_H_
#define ASH_VOICE_INTERACTION_VOICE_INTERACTION_CONTROLLER_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/public/interfaces/voice_interaction_controller.mojom.h"
#include "ash/voice_interaction/voice_interaction_observer.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace ash {

class ASH_EXPORT VoiceInteractionController
    : public mojom::VoiceInteractionController {
 public:
  VoiceInteractionController();
  ~VoiceInteractionController() override;

  void BindRequest(mojom::VoiceInteractionControllerRequest request);

  void AddObserver(VoiceInteractionObserver* observer);
  void RemoveObserver(VoiceInteractionObserver* observer);

  // ash::mojom::VoiceInteractionController:
  void NotifyStatusChanged(mojom::VoiceInteractionState state) override;
  void NotifySettingsEnabled(bool enabled) override;
  void NotifyContextEnabled(bool enabled) override;
  void NotifySetupCompleted(bool completed) override;
  void NotifyFeatureAllowed(mojom::AssistantAllowedState state) override;

  mojom::VoiceInteractionState voice_interaction_state() const {
    return voice_interaction_state_;
  }

  bool settings_enabled() const { return settings_enabled_; }

  bool setup_completed() const { return setup_completed_; }

  mojom::AssistantAllowedState allowed_state() const { return allowed_state_; }

 private:
  // Voice interaction state. The initial value should be set to STOPPED to make
  // sure the app list button burst animation could be correctly shown.
  mojom::VoiceInteractionState voice_interaction_state_ =
      mojom::VoiceInteractionState::STOPPED;

  // Whether voice interaction is enabled in system settings.
  bool settings_enabled_ = false;

  // Whether voice intearction setup flow has completed.
  bool setup_completed_ = false;

  // Whether voice intearction feature is allowed or disallowed for what reason.
  mojom::AssistantAllowedState allowed_state_ =
      mojom::AssistantAllowedState::ALLOWED;

  base::ObserverList<VoiceInteractionObserver> observers_;

  mojo::Binding<mojom::VoiceInteractionController> binding_;

  DISALLOW_COPY_AND_ASSIGN(VoiceInteractionController);
};

}  // namespace ash

#endif  // ASH_VOICE_INTERACTION_VOICE_INTERACTION_CONTROLLER_H_
