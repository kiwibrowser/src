// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_ASSISTANT_PLATFORM_API_IMPL_H_
#define CHROMEOS_SERVICES_ASSISTANT_PLATFORM_API_IMPL_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "chromeos/services/assistant/platform/audio_input_provider_impl.h"
#include "chromeos/services/assistant/platform/file_provider_impl.h"
#include "chromeos/services/assistant/platform/network_provider_impl.h"
#include "chromeos/services/assistant/platform/system_provider_impl.h"
#include "chromeos/services/assistant/public/mojom/assistant.mojom.h"
// TODO(xiaohuic): replace with "base/macros.h" once we remove
// libassistant/contrib dependency.
#include "libassistant/contrib/core/macros.h"
#include "libassistant/contrib/platform/audio/output/audio_output_provider_impl.h"
#include "libassistant/contrib/platform/resources/resource_provider.h"
#include "libassistant/shared/public/platform_api.h"
#include "libassistant/shared/public/platform_auth.h"
#include "services/device/public/mojom/battery_monitor.mojom.h"

namespace chromeos {
namespace assistant {

// Platform API required by the voice assistant.
class PlatformApiImpl : public assistant_client::PlatformApi {
 public:
  PlatformApiImpl(const std::string& config,
                  mojom::AudioInputPtr audio_input,
                  device::mojom::BatteryMonitorPtr battery_monitor);
  ~PlatformApiImpl() override;

  // assistant_client::PlatformApi overrides
  assistant_client::AudioInputProvider& GetAudioInputProvider() override;
  assistant_client::AudioOutputProvider& GetAudioOutputProvider() override;
  assistant_client::AuthProvider& GetAuthProvider() override;
  assistant_client::FileProvider& GetFileProvider() override;
  assistant_client::NetworkProvider& GetNetworkProvider() override;
  assistant_client::ResourceProvider& GetResourceProvider() override;
  assistant_client::SystemProvider& GetSystemProvider() override;

 private:
  // ChromeOS does not use auth manager, so we don't yet need to implement a
  // real auth provider.
  class DummyAuthProvider : public assistant_client::AuthProvider {
   public:
    DummyAuthProvider() = default;
    ~DummyAuthProvider() override = default;

    // assistant_client::AuthProvider overrides
    std::string GetAuthClientId() override;
    std::vector<std::string> GetClientCertificateChain() override;

    void CreateCredentialAttestationJwt(
        const std::string& authorization_code,
        const std::vector<std::pair<std::string, std::string>>& claims,
        CredentialCallback attestation_callback) override;

    void CreateRefreshAssertionJwt(
        const std::string& key_identifier,
        const std::vector<std::pair<std::string, std::string>>& claims,
        AssertionCallback assertion_callback) override;

    void CreateDeviceAttestationJwt(
        const std::vector<std::pair<std::string, std::string>>& claims,
        AssertionCallback attestation_callback) override;

    std::string GetAttestationCertFingerprint() override;

    void RemoveCredentialKey(const std::string& key_identifier) override;

    void Reset() override;
  };

  AudioInputProviderImpl audio_input_provider_;
  assistant_contrib::AudioOutputProviderImpl audio_output_provider_;
  DummyAuthProvider auth_provider_;
  FileProviderImpl file_provider_;
  NetworkProviderImpl network_provider_;
  assistant_contrib::ResourceProviderImpl resource_provider_;
  SystemProviderImpl system_provider_;

  DISALLOW_COPY_AND_ASSIGN(PlatformApiImpl);
};

}  // namespace assistant
}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_ASSISTANT_PLATFORM_API_IMPL_H_
