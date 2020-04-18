// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_FAKE_CRYPTAUTH_DEVICE_ID_PROVIDER_H_
#define COMPONENTS_CRYPTAUTH_FAKE_CRYPTAUTH_DEVICE_ID_PROVIDER_H_

#include "components/cryptauth/cryptauth_device_id_provider.h"

namespace cryptauth {

// Test implementation for CryptAuthDeviceIdProvider.
class FakeCryptAuthDeviceIdProvider : public CryptAuthDeviceIdProvider {
 public:
  FakeCryptAuthDeviceIdProvider(const std::string& device_id);
  ~FakeCryptAuthDeviceIdProvider() override;

  // CryptAuthDeviceIdProvider:
  std::string GetDeviceId() const override;

 private:
  const std::string device_id_;

  DISALLOW_COPY_AND_ASSIGN(FakeCryptAuthDeviceIdProvider);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_FAKE_CRYPTAUTH_DEVICE_ID_PROVIDER_H_
