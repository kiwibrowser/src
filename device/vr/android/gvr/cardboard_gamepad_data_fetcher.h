// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_VR_ANDROID_CARDBOARD_GAMEPAD_DATA_FETCHER_H_
#define DEVICE_VR_ANDROID_CARDBOARD_GAMEPAD_DATA_FETCHER_H_

#include <string>

#include "device/gamepad/gamepad_data_fetcher.h"
#include "device/vr/android/gvr/cardboard_gamepad_data_provider.h"
#include "device/vr/vr_export.h"

namespace device {

class DEVICE_VR_EXPORT CardboardGamepadDataFetcher : public GamepadDataFetcher {
 public:
  class Factory : public GamepadDataFetcherFactory {
   public:
    Factory(CardboardGamepadDataProvider*, unsigned int display_id);
    ~Factory() override;
    std::unique_ptr<GamepadDataFetcher> CreateDataFetcher() override;
    GamepadSource source() override;

   private:
    CardboardGamepadDataProvider* data_provider_;
    unsigned int display_id_;
  };

  CardboardGamepadDataFetcher(CardboardGamepadDataProvider*,
                              unsigned int display_id);
  ~CardboardGamepadDataFetcher() override;

  GamepadSource source() override;

  void GetGamepadData(bool devices_changed_hint) override;
  void PauseHint(bool paused) override;
  void OnAddedToProvider() override;

  // Called from CardboardGamepadDataProvider
  void SetGamepadData(CardboardGamepadData);

 private:
  unsigned int display_id_;
  CardboardGamepadData gamepad_data_;

  DISALLOW_COPY_AND_ASSIGN(CardboardGamepadDataFetcher);
};

}  // namespace device
#endif  // DEVICE_VR_ANDROID_CARDBOARD_GAMEPAD_DATA_FETCHER_H_
