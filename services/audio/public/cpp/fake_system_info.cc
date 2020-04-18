// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/public/cpp/fake_system_info.h"

#include "services/audio/public/mojom/constants.mojom.h"
#include "services/service_manager/public/cpp/bind_source_info.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace audio {

FakeSystemInfo::FakeSystemInfo() {}

FakeSystemInfo::~FakeSystemInfo() {}

// static
void FakeSystemInfo::OverrideGlobalBinderForAudioService(
    FakeSystemInfo* fake_system_info) {
  service_manager::ServiceContext::SetGlobalBinderForTesting(
      mojom::kServiceName, mojom::SystemInfo::Name_,
      base::BindRepeating(&FakeSystemInfo::Bind,
                          base::Unretained(fake_system_info)));
}

void FakeSystemInfo::GetInputStreamParameters(
    const std::string& device_id,
    GetInputStreamParametersCallback callback) {
  std::move(callback).Run(base::nullopt);
}

void FakeSystemInfo::GetOutputStreamParameters(
    const std::string& device_id,
    GetOutputStreamParametersCallback callback) {
  std::move(callback).Run(base::nullopt);
}

void FakeSystemInfo::HasInputDevices(HasInputDevicesCallback callback) {
  std::move(callback).Run(false);
}

void FakeSystemInfo::HasOutputDevices(HasOutputDevicesCallback callback) {
  std::move(callback).Run(false);
}

void FakeSystemInfo::GetInputDeviceDescriptions(
    GetInputDeviceDescriptionsCallback callback) {
  std::move(callback).Run(media::AudioDeviceDescriptions());
}

void FakeSystemInfo::GetOutputDeviceDescriptions(
    GetOutputDeviceDescriptionsCallback callback) {
  std::move(callback).Run(media::AudioDeviceDescriptions());
}

void FakeSystemInfo::GetAssociatedOutputDeviceID(
    const std::string& input_device_id,
    GetAssociatedOutputDeviceIDCallback callback) {
  std::move(callback).Run(base::nullopt);
}

void FakeSystemInfo::GetInputDeviceInfo(const std::string& input_device_id,
                                        GetInputDeviceInfoCallback callback) {
  std::move(callback).Run(base::nullopt, base::nullopt);
}

void FakeSystemInfo::Bind(const std::string& interface_name,
                          mojo::ScopedMessagePipeHandle handle,
                          const service_manager::BindSourceInfo& source_info) {
  DCHECK(interface_name == mojom::SystemInfo::Name_);
  bindings_.AddBinding(this, mojom::SystemInfoRequest(std::move(handle)));
}

}  // namespace audio
