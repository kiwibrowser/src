// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/secure_message_delegate_impl.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/no_destructor.h"
#include "chromeos/components/proximity_auth/logging/logging.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/easy_unlock_client.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

using cryptauth::SecureMessageDelegate;

namespace cryptauth {
namespace {

// Converts encryption type to a string representation used by EasyUnlock dbus
// client.
std::string EncSchemeToString(securemessage::EncScheme scheme) {
  switch (scheme) {
    case securemessage::AES_256_CBC:
      return easy_unlock::kEncryptionTypeAES256CBC;
    case securemessage::NONE:
      return easy_unlock::kEncryptionTypeNone;
  }

  NOTREACHED();
  return std::string();
}

// Converts signature type to a string representation used by EasyUnlock dbus
// client.
std::string SigSchemeToString(securemessage::SigScheme scheme) {
  switch (scheme) {
    case securemessage::ECDSA_P256_SHA256:
      return easy_unlock::kSignatureTypeECDSAP256SHA256;
    case securemessage::HMAC_SHA256:
      return easy_unlock::kSignatureTypeHMACSHA256;
    case securemessage::RSA2048_SHA256:
      // RSA2048_SHA256 is not supported by the daemon.
      NOTREACHED();
      return std::string();
  }

  NOTREACHED();
  return std::string();
}

// Parses the serialized HeaderAndBody string returned by the DBus client, and
// calls the corresponding SecureMessageDelegate unwrap callback.
void HandleUnwrapResult(
    const SecureMessageDelegate::UnwrapSecureMessageCallback& callback,
    const std::string& unwrap_result) {
  securemessage::HeaderAndBody header_and_body;
  if (!header_and_body.ParseFromString(unwrap_result)) {
    callback.Run(false, std::string(), securemessage::Header());
  } else {
    callback.Run(true, header_and_body.body(), header_and_body.header());
  }
}

// The SecureMessageDelegate expects the keys in the reverse order returned by
// the DBus client.
void HandleKeyPairResult(
    const SecureMessageDelegate::GenerateKeyPairCallback& callback,
    const std::string& private_key,
    const std::string& public_key) {
  callback.Run(public_key, private_key);
}

}  // namespace

// static
SecureMessageDelegateImpl::Factory*
    SecureMessageDelegateImpl::Factory::test_factory_instance_ = nullptr;

// static
std::unique_ptr<SecureMessageDelegate>
SecureMessageDelegateImpl::Factory::NewInstance() {
  if (test_factory_instance_)
    return test_factory_instance_->BuildInstance();

  static base::NoDestructor<SecureMessageDelegateImpl::Factory> factory;
  return factory->BuildInstance();
}

// static
void SecureMessageDelegateImpl::Factory::SetInstanceForTesting(
    Factory* test_factory) {
  test_factory_instance_ = test_factory;
}

SecureMessageDelegateImpl::Factory::~Factory() = default;

std::unique_ptr<SecureMessageDelegate>
SecureMessageDelegateImpl::Factory::BuildInstance() {
  return base::WrapUnique(new SecureMessageDelegateImpl());
}

SecureMessageDelegateImpl::SecureMessageDelegateImpl()
    : dbus_client_(chromeos::DBusThreadManager::Get()->GetEasyUnlockClient()) {}

SecureMessageDelegateImpl::~SecureMessageDelegateImpl() {}

void SecureMessageDelegateImpl::GenerateKeyPair(
    const GenerateKeyPairCallback& callback) {
  dbus_client_->GenerateEcP256KeyPair(
      base::Bind(HandleKeyPairResult, callback));
}

void SecureMessageDelegateImpl::DeriveKey(const std::string& private_key,
                                          const std::string& public_key,
                                          const DeriveKeyCallback& callback) {
  dbus_client_->PerformECDHKeyAgreement(private_key, public_key, callback);
}

void SecureMessageDelegateImpl::CreateSecureMessage(
    const std::string& payload,
    const std::string& key,
    const CreateOptions& create_options,
    const CreateSecureMessageCallback& callback) {
  if (create_options.signature_scheme == securemessage::RSA2048_SHA256) {
    PA_LOG(ERROR) << "Unable to create message: RSA2048_SHA256 not supported "
                  << "by the ChromeOS daemon.";
    callback.Run(std::string());
    return;
  }

  chromeos::EasyUnlockClient::CreateSecureMessageOptions options;
  options.key.assign(key);

  if (!create_options.associated_data.empty())
    options.associated_data.assign(create_options.associated_data);

  if (!create_options.public_metadata.empty())
    options.public_metadata.assign(create_options.public_metadata);

  if (!create_options.verification_key_id.empty())
    options.verification_key_id.assign(create_options.verification_key_id);

  if (!create_options.decryption_key_id.empty())
    options.decryption_key_id.assign(create_options.decryption_key_id);

  options.encryption_type = EncSchemeToString(create_options.encryption_scheme);
  options.signature_type = SigSchemeToString(create_options.signature_scheme);

  dbus_client_->CreateSecureMessage(payload, options, callback);
}

void SecureMessageDelegateImpl::UnwrapSecureMessage(
    const std::string& serialized_message,
    const std::string& key,
    const UnwrapOptions& unwrap_options,
    const UnwrapSecureMessageCallback& callback) {
  if (unwrap_options.signature_scheme == securemessage::RSA2048_SHA256) {
    PA_LOG(ERROR) << "Unable to unwrap message: RSA2048_SHA256 not supported "
                  << "by the ChromeOS daemon.";
    callback.Run(false, std::string(), securemessage::Header());
    return;
  }

  chromeos::EasyUnlockClient::UnwrapSecureMessageOptions options;
  options.key.assign(key);

  if (!unwrap_options.associated_data.empty())
    options.associated_data.assign(unwrap_options.associated_data);

  options.encryption_type = EncSchemeToString(unwrap_options.encryption_scheme);
  options.signature_type = SigSchemeToString(unwrap_options.signature_scheme);

  dbus_client_->UnwrapSecureMessage(serialized_message, options,
                                    base::Bind(&HandleUnwrapResult, callback));
}

}  // namespace cryptauth
