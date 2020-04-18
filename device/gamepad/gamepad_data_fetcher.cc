// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/gamepad/gamepad_data_fetcher.h"

namespace device {

GamepadDataFetcher::GamepadDataFetcher() : provider_(nullptr) {}

void GamepadDataFetcher::InitializeProvider(GamepadPadStateProvider* provider) {
  DCHECK(provider);

  provider_ = provider;
  OnAddedToProvider();
}

void GamepadDataFetcher::PlayEffect(
    int source_id,
    mojom::GamepadHapticEffectType type,
    mojom::GamepadEffectParametersPtr params,
    mojom::GamepadHapticsManager::PlayVibrationEffectOnceCallback callback) {
  std::move(callback).Run(
      mojom::GamepadHapticsResult::GamepadHapticsResultError);
}

void GamepadDataFetcher::ResetVibration(
    int source_id,
    mojom::GamepadHapticsManager::ResetVibrationActuatorCallback callback) {
  std::move(callback).Run(
      mojom::GamepadHapticsResult::GamepadHapticsResultError);
}

GamepadDataFetcherFactory::GamepadDataFetcherFactory() = default;

}  // namespace device
