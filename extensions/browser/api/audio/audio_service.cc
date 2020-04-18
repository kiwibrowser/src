// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/audio/audio_service.h"

namespace extensions {

class AudioServiceImpl : public AudioService {
 public:
  AudioServiceImpl() {}
  ~AudioServiceImpl() override {}

  // Called by listeners to this service to add/remove themselves as observers.
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;

  // Start to query audio device information.
  bool GetInfo(OutputInfo* output_info_out, InputInfo* input_info_out) override;
  bool GetDevices(const api::audio::DeviceFilter* filter,
                  DeviceInfoList* devices_out) override;
  void SetActiveDevices(const DeviceIdList& device_list) override;
  bool SetActiveDeviceLists(
      const std::unique_ptr<DeviceIdList>& input_devices,
      const std::unique_ptr<DeviceIdList>& output_devives) override;
  bool SetDeviceSoundLevel(const std::string& device_id,
                           int volume,
                           int gain) override;
  bool SetMuteForDevice(const std::string& device_id, bool value) override;
  bool SetMute(bool is_input, bool value) override;
  bool GetMute(bool is_input, bool* value) override;
};

void AudioServiceImpl::AddObserver(Observer* observer) {
  // TODO: implement this for platforms other than Chrome OS.
}

void AudioServiceImpl::RemoveObserver(Observer* observer) {
  // TODO: implement this for platforms other than Chrome OS.
}

AudioService* AudioService::CreateInstance(
    AudioDeviceIdCalculator* id_calculator) {
  return new AudioServiceImpl;
}

bool AudioServiceImpl::GetInfo(OutputInfo* output_info_out,
                               InputInfo* input_info_out) {
  // TODO: implement this for platforms other than Chrome OS.
  return false;
}

bool AudioServiceImpl::GetDevices(const api::audio::DeviceFilter* filter,
                                  DeviceInfoList* devices_out) {
  return false;
}

bool AudioServiceImpl::SetActiveDeviceLists(
    const std::unique_ptr<DeviceIdList>& input_devices,
    const std::unique_ptr<DeviceIdList>& output_devives) {
  return false;
}

void AudioServiceImpl::SetActiveDevices(const DeviceIdList& device_list) {
}

bool AudioServiceImpl::SetDeviceSoundLevel(const std::string& device_id,
                                           int volume,
                                           int gain) {
  return false;
}

bool AudioServiceImpl::SetMuteForDevice(const std::string& device_id,
                                        bool value) {
  return false;
}

bool AudioServiceImpl::SetMute(bool is_input, bool value) {
  return false;
}

bool AudioServiceImpl::GetMute(bool is_input, bool* value) {
  return false;
}

}  // namespace extensions
