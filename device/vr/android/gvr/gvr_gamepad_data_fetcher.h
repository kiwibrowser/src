// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_VR_ANDROID_GVR_GAMEPAD_DATA_FETCHER_H_
#define DEVICE_VR_ANDROID_GVR_GAMEPAD_DATA_FETCHER_H_

#include <string>

#include "device/gamepad/gamepad_data_fetcher.h"
#include "device/vr/android/gvr/gvr_gamepad_data_provider.h"
#include "device/vr/vr_export.h"

namespace device {

class DEVICE_VR_EXPORT GvrGamepadDataFetcher : public GamepadDataFetcher {
 public:
  class Factory : public GamepadDataFetcherFactory {
   public:
    Factory(GvrGamepadDataProvider*, unsigned int display_id);
    ~Factory() override;
    std::unique_ptr<GamepadDataFetcher> CreateDataFetcher() override;
    GamepadSource source() override;

   private:
    GvrGamepadDataProvider* data_provider_;
    unsigned int display_id_;
  };

  GvrGamepadDataFetcher(GvrGamepadDataProvider*, unsigned int display_id);
  ~GvrGamepadDataFetcher() override;

  GamepadSource source() override;

  void GetGamepadData(bool devices_changed_hint) override;
  void PauseHint(bool paused) override;
  void OnAddedToProvider() override;

  // Called from GvrGamepadDataProvider
  void SetGamepadData(GvrGamepadData);

 private:
  unsigned int display_id_;
  GvrGamepadData gamepad_data_;

  DISALLOW_COPY_AND_ASSIGN(GvrGamepadDataFetcher);
};

}  // namespace device
#endif  // DEVICE_VR_ANDROID_GVR_GAMEPAD_DATA_FETCHER_H_
