// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_NETWORK_QUALITY_PROVIDER_STUB_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_NETWORK_QUALITY_PROVIDER_STUB_H_

#include "base/supports_user_data.h"
#include "net/nqe/effective_connection_type.h"
#include "net/nqe/network_quality_provider.h"

namespace offline_pages {

// Test class stubbing out the functionality of NQE::NetworkQualityProvider.
// It is only used for test support.
class NetworkQualityProviderStub : public net::NetworkQualityProvider,
                                   public base::SupportsUserData::Data {
 public:
  NetworkQualityProviderStub();
  ~NetworkQualityProviderStub() override;

  static NetworkQualityProviderStub* GetUserData(
      base::SupportsUserData* supports_user_data);
  static void SetUserData(base::SupportsUserData* supports_user_data,
                          std::unique_ptr<NetworkQualityProviderStub> stub);

  net::EffectiveConnectionType GetEffectiveConnectionType() const override;

  void SetEffectiveConnectionTypeForTest(net::EffectiveConnectionType type) {
    connection_type_ = type;
  }

 private:
  net::EffectiveConnectionType connection_type_;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_NETWORK_QUALITY_PROVIDER_STUB_H_
