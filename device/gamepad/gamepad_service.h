// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GAMEPAD_GAMEPAD_SERVICE_H_
#define DEVICE_GAMEPAD_GAMEPAD_SERVICE_H_

#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/shared_memory.h"
#include "base/memory/singleton.h"
#include "device/gamepad/gamepad_export.h"
#include "device/gamepad/gamepad_provider.h"
#include "device/gamepad/public/mojom/gamepad.mojom.h"

namespace {
class SingleThreadTaskRunner;
}

namespace content {
class GamepadServiceTestConstructor;
}

namespace device {
class GamepadConsumer;
class GamepadDataFetcher;
class GamepadProvider;

// Owns the GamepadProvider (the background polling thread) and keeps track of
// the number of consumers currently using the data (and pausing the provider
// when not in use).
class DEVICE_GAMEPAD_EXPORT GamepadService
    : public device::GamepadConnectionChangeClient {
 public:
  // Returns the GamepadService singleton.
  static GamepadService* GetInstance();

  // Increments the number of users of the provider. The Provider is running
  // when there's > 0 users, and is paused when the count drops to 0.
  // consumer is registered to listen for gamepad connections. If this is the
  // first time it is added to the set of consumers it will be treated
  // specially: it will not be informed about connections before a new user
  // gesture is observed at which point it will be notified for every connected
  // gamepads.
  //
  // Must be called on the I/O thread.
  void ConsumerBecameActive(device::GamepadConsumer* consumer);

  // Decrements the number of users of the provider. consumer will not be
  // informed about connections until it's added back via ConsumerBecameActive.
  // Must be matched with a ConsumerBecameActive call.
  //
  // Must be called on the I/O thread.
  void ConsumerBecameInactive(device::GamepadConsumer* consumer);

  // Decrements the number of users of the provider and removes consumer from
  // the set of consumers. Should be matched with a a ConsumerBecameActive
  // call.
  //
  // Must be called on the I/O thread.
  void RemoveConsumer(device::GamepadConsumer* consumer);

  // Registers the given closure for calling when the user has interacted with
  // the device. This callback will only be issued once. Should only be called
  // while a consumer is active.
  void RegisterForUserGesture(const base::Closure& closure);

  // Returns a duplicate of the shared memory handle of the gamepad data.
  base::SharedMemoryHandle DuplicateSharedMemoryHandle();

  // Returns a new mojo::ScopedSharedBuffer handle of the gamepad data.
  mojo::ScopedSharedBufferHandle GetSharedBufferHandle();

  // Stop/join with the background thread in GamepadProvider |provider_|.
  void Terminate();

  // Called on IO thread when a gamepad is connected.
  void OnGamepadConnected(int index, const Gamepad& pad);

  // Called on IO thread when a gamepad is disconnected.
  void OnGamepadDisconnected(int index, const Gamepad& pad);

  // Request playback of a haptic effect on the specified gamepad. Once effect
  // playback is complete or is preempted by a different effect, the callback
  // will be called.
  void PlayVibrationEffectOnce(
      int pad_index,
      mojom::GamepadHapticEffectType,
      mojom::GamepadEffectParametersPtr,
      mojom::GamepadHapticsManager::PlayVibrationEffectOnceCallback);

  // Resets the state of the vibration actuator on the specified gamepad. If any
  // effects are currently being played, they are preempted and vibration is
  // stopped.
  void ResetVibrationActuator(
      int pad_index,
      mojom::GamepadHapticsManager::ResetVibrationActuatorCallback);

 private:
  friend struct base::DefaultSingletonTraits<GamepadService>;
  friend class GamepadServiceTestConstructor;
  friend class GamepadServiceTest;

  GamepadService();

  // Constructor for testing. This specifies the data fetcher to use for a
  // provider, bypassing the default platform one.
  GamepadService(std::unique_ptr<device::GamepadDataFetcher> fetcher);

  virtual ~GamepadService();

  static void SetInstance(GamepadService*);

  void OnUserGesture();

  void OnGamepadConnectionChange(bool connected,
                                 int index,
                                 const Gamepad& pad) override;

  void SetSanitizationEnabled(bool sanitize);

  struct ConsumerInfo {
    ConsumerInfo(device::GamepadConsumer* consumer)
        : consumer(consumer), did_observe_user_gesture(false) {}

    bool operator<(const ConsumerInfo& other) const {
      return consumer < other.consumer;
    }

    device::GamepadConsumer* consumer;
    mutable bool is_active;
    mutable bool did_observe_user_gesture;
  };

  std::unique_ptr<device::GamepadProvider> provider_;

  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;

  typedef std::set<ConsumerInfo> ConsumerSet;
  ConsumerSet consumers_;

  typedef std::unordered_map<device::GamepadConsumer*, std::vector<bool>>
      ConsumerConnectedStateMap;

  ConsumerConnectedStateMap inactive_consumer_state_;

  int num_active_consumers_;

  bool gesture_callback_pending_;

  DISALLOW_COPY_AND_ASSIGN(GamepadService);
};

}  // namespace device

#endif  // DEVICE_GAMEPAD_GAMEPAD_SERVICE_H_
