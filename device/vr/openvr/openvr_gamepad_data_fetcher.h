// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_VR_OPENVR_GAMEPAD_DATA_FETCHER_H_
#define DEVICE_VR_OPENVR_GAMEPAD_DATA_FETCHER_H_

#include "device/gamepad/gamepad_data_fetcher.h"

namespace vr {
class IVRSystem;
}  // namespace vr

namespace device {

class OpenVRGamepadDataFetcher : public GamepadDataFetcher {
 public:
  class Factory : public GamepadDataFetcherFactory {
   public:
    Factory(unsigned int display_id, vr::IVRSystem* vr);
    ~Factory() override;
    std::unique_ptr<GamepadDataFetcher> CreateDataFetcher() override;
    GamepadSource source() override;

   private:
    unsigned int display_id_;
    vr::IVRSystem* vr_system_;
  };

  OpenVRGamepadDataFetcher(unsigned int display_id, vr::IVRSystem* vr);
  ~OpenVRGamepadDataFetcher() override;

  GamepadSource source() override;

  void GetGamepadData(bool devices_changed_hint) override;
  void PauseHint(bool paused) override;
  void OnAddedToProvider() override;

 private:
  unsigned int display_id_;
  vr::IVRSystem* vr_system_;

  DISALLOW_COPY_AND_ASSIGN(OpenVRGamepadDataFetcher);
};

}  // namespace device
#endif  // DEVICE_VR_OPENVR_GAMEPAD_DATA_FETCHER_H_
