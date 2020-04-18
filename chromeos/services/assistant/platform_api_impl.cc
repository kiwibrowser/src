// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/assistant/platform_api_impl.h"

#include <string>
#include <utility>
#include <vector>

#include "libassistant/shared/public/assistant_export.h"
#include "libassistant/shared/public/platform_api.h"
#include "libassistant/shared/public/platform_factory.h"

using assistant_client::AudioInputProvider;
using assistant_client::AudioOutputProvider;
using assistant_client::AuthProvider;
using assistant_client::FileProvider;
using assistant_client::NetworkProvider;
using assistant_client::ResourceProvider;
using assistant_client::SystemProvider;
using assistant_client::PlatformApi;
using assistant_client::ResourceProvider;

namespace chromeos {
namespace assistant {

////////////////////////////////////////////////////////////////////////////////
// DummyAuthProvider
////////////////////////////////////////////////////////////////////////////////

std::string PlatformApiImpl::DummyAuthProvider::GetAuthClientId() {
  return "kFakeClientId";
}

std::vector<std::string>
PlatformApiImpl::DummyAuthProvider::GetClientCertificateChain() {
  return {};
}

void PlatformApiImpl::DummyAuthProvider::CreateCredentialAttestationJwt(
    const std::string& authorization_code,
    const std::vector<std::pair<std::string, std::string>>& claims,
    CredentialCallback attestation_callback) {
  attestation_callback(Error::SUCCESS, "", "");
}

void PlatformApiImpl::DummyAuthProvider::CreateRefreshAssertionJwt(
    const std::string& key_identifier,
    const std::vector<std::pair<std::string, std::string>>& claims,
    AssertionCallback assertion_callback) {
  assertion_callback(Error::SUCCESS, "");
}

void PlatformApiImpl::DummyAuthProvider::CreateDeviceAttestationJwt(
    const std::vector<std::pair<std::string, std::string>>& claims,
    AssertionCallback attestation_callback) {
  attestation_callback(Error::SUCCESS, "");
}

std::string
PlatformApiImpl::DummyAuthProvider::GetAttestationCertFingerprint() {
  return "kFakeAttestationCertFingerprint";
}

void PlatformApiImpl::DummyAuthProvider::RemoveCredentialKey(
    const std::string& key_identifier) {}

void PlatformApiImpl::DummyAuthProvider::Reset() {}

////////////////////////////////////////////////////////////////////////////////
// PlatformApiImpl
////////////////////////////////////////////////////////////////////////////////

PlatformApiImpl::PlatformApiImpl(
    const std::string& config,
    mojom::AudioInputPtr audio_input,
    device::mojom::BatteryMonitorPtr battery_monitor)
    : audio_input_provider_(std::move(audio_input)),
      audio_output_provider_(config, this),
      resource_provider_(config),
      system_provider_(std::move(battery_monitor)) {}

PlatformApiImpl::~PlatformApiImpl() = default;

AudioInputProvider& PlatformApiImpl::GetAudioInputProvider() {
  return audio_input_provider_;
}

AudioOutputProvider& PlatformApiImpl::GetAudioOutputProvider() {
  return audio_output_provider_;
}

AuthProvider& PlatformApiImpl::GetAuthProvider() {
  return auth_provider_;
}

FileProvider& PlatformApiImpl::GetFileProvider() {
  return file_provider_;
}

NetworkProvider& PlatformApiImpl::GetNetworkProvider() {
  return network_provider_;
}

ResourceProvider& PlatformApiImpl::GetResourceProvider() {
  return resource_provider_;
}

SystemProvider& PlatformApiImpl::GetSystemProvider() {
  return system_provider_;
}

}  // namespace assistant
}  // namespace chromeos
