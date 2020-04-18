// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GAMEPAD_GAMEPAD_MONITOR_H_
#define DEVICE_GAMEPAD_GAMEPAD_MONITOR_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "device/gamepad/gamepad_consumer.h"
#include "device/gamepad/gamepad_export.h"
#include "device/gamepad/public/mojom/gamepad.mojom.h"

namespace device {

class DEVICE_GAMEPAD_EXPORT GamepadMonitor : public GamepadConsumer,
                                             public mojom::GamepadMonitor {
 public:
  GamepadMonitor();
  ~GamepadMonitor() override;

  static void Create(mojom::GamepadMonitorRequest request);

  // GamepadConsumer implementation.
  void OnGamepadConnected(unsigned index, const Gamepad& gamepad) override;
  void OnGamepadDisconnected(unsigned index, const Gamepad& gamepad) override;

  // mojom::GamepadMonitor implementation.
  void GamepadStartPolling(GamepadStartPollingCallback callback) override;
  void GamepadStopPolling(GamepadStopPollingCallback callback) override;
  void SetObserver(mojom::GamepadObserverPtr gamepad_observer) override;

 private:
  mojom::GamepadObserverPtr gamepad_observer_;
  bool is_started_;

  DISALLOW_COPY_AND_ASSIGN(GamepadMonitor);
};

}  // namespace device

#endif  // DEVICE_GAMEPAD_GAMEPAD_MONITOR_H_
