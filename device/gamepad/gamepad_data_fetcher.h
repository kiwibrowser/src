// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GAMEPAD_GAMEPAD_DATA_FETCHER_H_
#define DEVICE_GAMEPAD_GAMEPAD_DATA_FETCHER_H_

#include "device/gamepad/gamepad_data_fetcher_manager.h"
#include "device/gamepad/gamepad_export.h"
#include "device/gamepad/gamepad_pad_state_provider.h"
#include "device/gamepad/public/cpp/gamepad.h"
#include "device/gamepad/public/mojom/gamepad.mojom.h"

namespace device {

// Abstract interface for imlementing platform- (and test-) specific behavior
// for getting the gamepad data.
class DEVICE_GAMEPAD_EXPORT GamepadDataFetcher {
 public:
  GamepadDataFetcher();
  virtual ~GamepadDataFetcher() {}
  virtual void GetGamepadData(bool devices_changed_hint) = 0;
  virtual void PauseHint(bool paused) {}
  virtual void PlayEffect(
      int source_id,
      mojom::GamepadHapticEffectType,
      mojom::GamepadEffectParametersPtr,
      mojom::GamepadHapticsManager::PlayVibrationEffectOnceCallback);
  virtual void ResetVibration(
      int source_id,
      mojom::GamepadHapticsManager::ResetVibrationActuatorCallback);

  virtual GamepadSource source() = 0;
  GamepadPadStateProvider* provider() { return provider_; }

  PadState* GetPadState(int source_id) {
    if (!provider_)
      return nullptr;

    return provider_->GetPadState(source(), source_id);
  }

 protected:
  friend GamepadPadStateProvider;

  // To be called by the GamepadPadStateProvider on the polling thread;
  void InitializeProvider(GamepadPadStateProvider* provider);

  // This call will happen on the gamepad polling thread. Any initialization
  // that needs to happen on that thread should be done here, not in the
  // constructor.
  virtual void OnAddedToProvider(){};

 private:
  GamepadPadStateProvider* provider_;
};

// Factory class for creating a GamepadDataFetcher. Used by the
// GamepadDataFetcherManager.
class DEVICE_GAMEPAD_EXPORT GamepadDataFetcherFactory {
 public:
  GamepadDataFetcherFactory();
  virtual ~GamepadDataFetcherFactory() {}
  virtual std::unique_ptr<GamepadDataFetcher> CreateDataFetcher() = 0;
  virtual GamepadSource source() = 0;
};

// Basic factory implementation for GamepadDataFetchers without a complex
// constructor.
template <typename DataFetcherType, GamepadSource DataFetcherSource>
class GamepadDataFetcherFactoryImpl : public GamepadDataFetcherFactory {
 public:
  ~GamepadDataFetcherFactoryImpl() override {}
  std::unique_ptr<GamepadDataFetcher> CreateDataFetcher() override {
    return std::unique_ptr<GamepadDataFetcher>(new DataFetcherType());
  }
  GamepadSource source() override { return DataFetcherSource; }
  static GamepadSource static_source() { return DataFetcherSource; }
};

}  // namespace device

#endif  // DEVICE_GAMEPAD_GAMEPAD_DATA_FETCHER_H_
