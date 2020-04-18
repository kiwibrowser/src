// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/gamepad/gamepad_dispatcher.h"

#include "third_party/blink/public/platform/interface_provider.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/modules/gamepad/navigator_gamepad.h"

namespace blink {

using device::mojom::blink::GamepadHapticsManager;

GamepadDispatcher& GamepadDispatcher::Instance() {
  DEFINE_STATIC_LOCAL(GamepadDispatcher, gamepad_dispatcher,
                      (new GamepadDispatcher));
  return gamepad_dispatcher;
}

void GamepadDispatcher::SampleGamepads(device::Gamepads& gamepads) {
  Platform::Current()->SampleGamepads(gamepads);
}

void GamepadDispatcher::PlayVibrationEffectOnce(
    int pad_index,
    device::mojom::blink::GamepadHapticEffectType type,
    device::mojom::blink::GamepadEffectParametersPtr params,
    GamepadHapticsManager::PlayVibrationEffectOnceCallback callback) {
  InitializeHaptics();
  gamepad_haptics_manager_->PlayVibrationEffectOnce(
      pad_index, type, std::move(params), std::move(callback));
}

void GamepadDispatcher::ResetVibrationActuator(
    int pad_index,
    GamepadHapticsManager::ResetVibrationActuatorCallback callback) {
  InitializeHaptics();
  gamepad_haptics_manager_->ResetVibrationActuator(pad_index,
                                                   std::move(callback));
}

GamepadDispatcher::GamepadDispatcher() = default;

GamepadDispatcher::~GamepadDispatcher() = default;

void GamepadDispatcher::InitializeHaptics() {
  if (!gamepad_haptics_manager_) {
    Platform::Current()->GetInterfaceProvider()->GetInterface(
        mojo::MakeRequest(&gamepad_haptics_manager_));
  }
}

void GamepadDispatcher::Trace(blink::Visitor* visitor) {
  PlatformEventDispatcher::Trace(visitor);
}

void GamepadDispatcher::DidConnectGamepad(unsigned index,
                                          const device::Gamepad& gamepad) {
  DispatchDidConnectOrDisconnectGamepad(index, gamepad, true);
}

void GamepadDispatcher::DidDisconnectGamepad(unsigned index,
                                             const device::Gamepad& gamepad) {
  DispatchDidConnectOrDisconnectGamepad(index, gamepad, false);
}

void GamepadDispatcher::DispatchDidConnectOrDisconnectGamepad(
    unsigned index,
    const device::Gamepad& gamepad,
    bool connected) {
  DCHECK(index < device::Gamepads::kItemsLengthCap);
  DCHECK_EQ(connected, gamepad.connected);

  latest_change_.pad = gamepad;
  latest_change_.index = index;
  NotifyControllers();
}

void GamepadDispatcher::StartListening() {
  Platform::Current()->StartListening(kWebPlatformEventTypeGamepad, this);
}

void GamepadDispatcher::StopListening() {
  Platform::Current()->StopListening(kWebPlatformEventTypeGamepad);
}

}  // namespace blink
