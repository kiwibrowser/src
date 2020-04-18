// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/fake_cryptauth_device_id_provider.h"

namespace cryptauth {

FakeCryptAuthDeviceIdProvider::FakeCryptAuthDeviceIdProvider(
    const std::string& device_id)
    : device_id_(device_id) {}

FakeCryptAuthDeviceIdProvider::~FakeCryptAuthDeviceIdProvider() = default;

std::string FakeCryptAuthDeviceIdProvider::GetDeviceId() const {
  return device_id_;
}

}  // namespace cryptauth
