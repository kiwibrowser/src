// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/virtual_authenticator.h"

#include <utility>
#include <vector>

#include "base/guid.h"
#include "crypto/ec_private_key.h"
#include "device/fido/virtual_u2f_device.h"

namespace content {

VirtualAuthenticator::VirtualAuthenticator(
    ::device::FidoTransportProtocol transport)
    : transport_(transport),
      unique_id_(base::GenerateGUID()),
      state_(base::MakeRefCounted<::device::VirtualFidoDevice::State>()) {}

VirtualAuthenticator::~VirtualAuthenticator() = default;

void VirtualAuthenticator::AddBinding(
    webauth::test::mojom::VirtualAuthenticatorRequest request) {
  binding_set_.AddBinding(this, std::move(request));
}

std::unique_ptr<::device::FidoDevice> VirtualAuthenticator::ConstructDevice() {
  return std::make_unique<::device::VirtualU2fDevice>(state_);
}

void VirtualAuthenticator::GetUniqueId(GetUniqueIdCallback callback) {
  std::move(callback).Run(unique_id_);
}

void VirtualAuthenticator::GetRegistrations(GetRegistrationsCallback callback) {
  std::vector<webauth::test::mojom::RegisteredKeyPtr> mojo_registered_keys;
  for (const auto& registration : state_->registrations) {
    auto mojo_registered_key = webauth::test::mojom::RegisteredKey::New();
    mojo_registered_key->key_handle = registration.first;
    mojo_registered_key->counter = registration.second.counter;
    mojo_registered_key->application_parameter =
        registration.second.application_parameter;
    registration.second.private_key->ExportPrivateKey(
        &mojo_registered_key->private_key);
    mojo_registered_keys.push_back(std::move(mojo_registered_key));
  }
  std::move(callback).Run(std::move(mojo_registered_keys));
}

void VirtualAuthenticator::AddRegistration(
    webauth::test::mojom::RegisteredKeyPtr registration,
    AddRegistrationCallback callback) {
  bool success = false;
  std::tie(std::ignore, success) = state_->registrations.emplace(
      std::move(registration->key_handle),
      ::device::VirtualFidoDevice::RegistrationData(
          crypto::ECPrivateKey::CreateFromPrivateKeyInfo(
              registration->private_key),
          std::move(registration->application_parameter),
          registration->counter));
  std::move(callback).Run(success);
}

void VirtualAuthenticator::ClearRegistrations(
    ClearRegistrationsCallback callback) {
  state_->registrations.clear();
  std::move(callback).Run();
}

void VirtualAuthenticator::SetUserPresence(bool present,
                                           SetUserPresenceCallback callback) {
  // TODO(https://crbug.com/785955): Implement once VirtualFidoDevice supports
  // this.
  std::move(callback).Run();
}

void VirtualAuthenticator::GetUserPresence(GetUserPresenceCallback callback) {
  std::move(callback).Run(false);
}

}  // namespace content
