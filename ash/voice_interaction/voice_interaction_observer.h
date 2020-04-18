// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_VOICE_INTERACTION_VOICE_INTERACTION_OBSERVER_H_
#define ASH_VOICE_INTERACTION_VOICE_INTERACTION_OBSERVER_H_

namespace ash {

namespace mojom {

enum class AssistantAllowedState;
enum class VoiceInteractionState;

}  // namespace mojom

class VoiceInteractionObserver {
 public:
  // Called when voice interaction session state changes.
  virtual void OnVoiceInteractionStatusChanged(
      mojom::VoiceInteractionState state) {}

  // Called when voice interaction is enabled/disabled in settings.
  virtual void OnVoiceInteractionSettingsEnabled(bool enabled) {}

  // Called when voice interaction service is allowed/disallowed to access
  // the "context" (text and graphic content that is currently on screen).
  virtual void OnVoiceInteractionContextEnabled(bool enabled) {}

  // Called when voice interaction setup flow completed.
  virtual void OnVoiceInteractionSetupCompleted(bool completed) {}

  // Called when assistant feature allowed state has changed.
  virtual void OnAssistantFeatureAllowedChanged(
      mojom::AssistantAllowedState state) {}

 protected:
  virtual ~VoiceInteractionObserver() = default;
};

}  // namespace ash

#endif  // ASH_VOICE_INTERACTION_VOICE_INTERACTION_OBSERVER_H_
