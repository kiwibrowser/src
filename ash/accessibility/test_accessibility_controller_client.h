// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ACCESSIBILITY_TEST_ACCESSIBILITY_CONTROLLER_CLEINT_H_
#define ASH_ACCESSIBILITY_TEST_ACCESSIBILITY_CONTROLLER_CLEINT_H_

#include "ash/public/interfaces/accessibility_controller.mojom.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "ui/accessibility/ax_enums.mojom.h"

namespace ash {

// Implement AccessibilityControllerClient mojo interface to simulate chrome
// behavior in tests. This breaks the ash/chrome dependency to allow testing ash
// code in isolation.
class TestAccessibilityControllerClient
    : public mojom::AccessibilityControllerClient {
 public:
  TestAccessibilityControllerClient();
  ~TestAccessibilityControllerClient() override;

  static constexpr base::TimeDelta kShutdownSoundDuration =
      base::TimeDelta::FromMilliseconds(1000);

  mojom::AccessibilityControllerClientPtr CreateInterfacePtrAndBind();

  // mojom::AccessibilityControllerClient:
  void TriggerAccessibilityAlert(mojom::AccessibilityAlert alert) override;
  void PlayEarcon(int32_t sound_key) override;
  void PlayShutdownSound(PlayShutdownSoundCallback callback) override;
  void HandleAccessibilityGesture(ax::mojom::Gesture gesture) override;
  void ToggleDictation(ToggleDictationCallback callback) override;
  void SilenceSpokenFeedback() override;
  void OnTwoFingerTouchStart() override;
  void OnTwoFingerTouchStop() override;
  void ShouldToggleSpokenFeedbackViaTouch(
      ShouldToggleSpokenFeedbackViaTouchCallback callback) override;
  void PlaySpokenFeedbackToggleCountdown(int tick_count) override;
  void RequestSelectToSpeakStateChange() override;

  int32_t GetPlayedEarconAndReset();

  mojom::AccessibilityAlert last_a11y_alert() const { return last_a11y_alert_; }
  ax::mojom::Gesture last_a11y_gesture() const { return last_a11y_gesture_; }
  int select_to_speak_change_change_requests() const {
    return select_to_speak_state_change_requests_;
  }

 private:
  mojom::AccessibilityAlert last_a11y_alert_ = mojom::AccessibilityAlert::NONE;

  int32_t sound_key_ = -1;
  bool is_dictation_active_ = false;

  ax::mojom::Gesture last_a11y_gesture_ = ax::mojom::Gesture::kNone;

  int select_to_speak_state_change_requests_ = 0;

  mojo::Binding<mojom::AccessibilityControllerClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestAccessibilityControllerClient);
};

}  // namespace ash

#endif  // ASH_ACCESSIBILITY_TEST_ACCESSIBILITY_CONTROLLER_CLEINT_H_
