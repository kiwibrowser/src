// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_ACCESSIBILITY_ACCESSIBILITY_CONTROLLER_CLIENT_H_
#define CHROME_BROWSER_UI_ASH_ACCESSIBILITY_ACCESSIBILITY_CONTROLLER_CLIENT_H_

#include "ash/public/interfaces/accessibility_controller.mojom.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "ui/accessibility/ax_enums.mojom.h"

// Handles method calls from ash to do accessibility related service in chrome.
class AccessibilityControllerClient
    : public ash::mojom::AccessibilityControllerClient {
 public:
  AccessibilityControllerClient();
  ~AccessibilityControllerClient() override;

  // Initializes and connects to ash.
  void Init();

  // Tests can provide a mock mojo interface for the ash controller.
  void InitForTesting(ash::mojom::AccessibilityControllerPtr controller);

  // ash::mojom::AccessibilityControllerClient:
  void TriggerAccessibilityAlert(ash::mojom::AccessibilityAlert alert) override;
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

  // Flushes the mojo pipe to ash.
  void FlushForTesting();

 private:
  // Binds this object to its mojo interface and sets it as the ash client.
  void BindAndSetClient();

  // Binds to the client interface.
  mojo::Binding<ash::mojom::AccessibilityControllerClient> binding_;

  // AccessibilityController interface in ash. Holding the interface pointer
  // keeps the pipe alive to receive mojo return values.
  ash::mojom::AccessibilityControllerPtr accessibility_controller_;

  DISALLOW_COPY_AND_ASSIGN(AccessibilityControllerClient);
};

#endif  // CHROME_BROWSER_UI_ASH_ACCESSIBILITY_ACCESSIBILITY_CONTROLLER_CLIENT_H_
