// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/networking_cast_private/networking_cast_private_api.h"

#include <utility>

#include "base/bind.h"
#include "chrome/common/extensions/api/networking_cast_private.h"
#include "extensions/browser/api/extensions_api_client.h"
#include "extensions/browser/api/networking_private/networking_cast_private_delegate.h"

#if defined(OS_CHROMEOS)
#include "chromeos/network/network_device_handler.h"
#include "chromeos/network/network_handler.h"
#include "third_party/cros_system_api/dbus/shill/dbus-constants.h"
#endif

namespace private_api = extensions::api::networking_private;
namespace cast_api = extensions::api::networking_cast_private;

namespace extensions {

namespace {

#if defined(OS_CHROMEOS)
// Parses TDLS status returned by network handler to networking_cast_private
// TDLS status type.
cast_api::TDLSStatus ParseTDLSStatus(const std::string& status) {
  if (status == shill::kTDLSConnectedState)
    return cast_api::TDLS_STATUS_CONNECTED;
  if (status == shill::kTDLSNonexistentState)
    return cast_api::TDLS_STATUS_NONEXISTENT;
  if (status == shill::kTDLSDisabledState)
    return cast_api::TDLS_STATUS_DISABLED;
  if (status == shill::kTDLSDisconnectedState)
    return cast_api::TDLS_STATUS_DISCONNECTED;
  if (status == shill::kTDLSUnknownState)
    return cast_api::TDLS_STATUS_UNKNOWN;

  NOTREACHED() << "Unknown TDLS status " << status;
  return cast_api::TDLS_STATUS_UNKNOWN;
}
#endif

std::unique_ptr<NetworkingCastPrivateDelegate::Credentials> AsCastCredentials(
    api::networking_cast_private::VerificationProperties& properties) {
  return std::make_unique<NetworkingCastPrivateDelegate::Credentials>(
      properties.certificate,
      properties.intermediate_certificates
          ? *properties.intermediate_certificates
          : std::vector<std::string>(),
      properties.signed_data, properties.device_ssid, properties.device_serial,
      properties.device_bssid, properties.public_key, properties.nonce);
}

}  // namespace

NetworkingCastPrivateVerifyDestinationFunction::
    ~NetworkingCastPrivateVerifyDestinationFunction() {}

ExtensionFunction::ResponseAction
NetworkingCastPrivateVerifyDestinationFunction::Run() {
  std::unique_ptr<cast_api::VerifyDestination::Params> params =
      cast_api::VerifyDestination::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  NetworkingCastPrivateDelegate* delegate =
      ExtensionsAPIClient::Get()->GetNetworkingCastPrivateDelegate();
  delegate->VerifyDestination(
      AsCastCredentials(params->properties),
      base::Bind(&NetworkingCastPrivateVerifyDestinationFunction::Success,
                 this),
      base::Bind(&NetworkingCastPrivateVerifyDestinationFunction::Failure,
                 this));

  // VerifyDestination might respond synchronously, e.g. in tests.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingCastPrivateVerifyDestinationFunction::Success(bool result) {
  Respond(ArgumentList(cast_api::VerifyDestination::Results::Create(result)));
}

void NetworkingCastPrivateVerifyDestinationFunction::Failure(
    const std::string& error) {
  Respond(Error(error));
}

NetworkingCastPrivateVerifyAndEncryptCredentialsFunction::
    ~NetworkingCastPrivateVerifyAndEncryptCredentialsFunction() {}

ExtensionFunction::ResponseAction
NetworkingCastPrivateVerifyAndEncryptCredentialsFunction::Run() {
  std::unique_ptr<cast_api::VerifyAndEncryptCredentials::Params> params =
      cast_api::VerifyAndEncryptCredentials::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  NetworkingCastPrivateDelegate* delegate =
      ExtensionsAPIClient::Get()->GetNetworkingCastPrivateDelegate();
  delegate->VerifyAndEncryptCredentials(
      params->network_guid, AsCastCredentials(params->properties),
      base::Bind(
          &NetworkingCastPrivateVerifyAndEncryptCredentialsFunction::Success,
          this),
      base::Bind(
          &NetworkingCastPrivateVerifyAndEncryptCredentialsFunction::Failure,
          this));

  // VerifyAndEncryptCredentials might respond synchronously, e.g. in tests.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingCastPrivateVerifyAndEncryptCredentialsFunction::Success(
    const std::string& result) {
  Respond(ArgumentList(
      cast_api::VerifyAndEncryptCredentials::Results::Create(result)));
}

void NetworkingCastPrivateVerifyAndEncryptCredentialsFunction::Failure(
    const std::string& error) {
  Respond(Error(error));
}

NetworkingCastPrivateVerifyAndEncryptDataFunction::
    ~NetworkingCastPrivateVerifyAndEncryptDataFunction() {}

ExtensionFunction::ResponseAction
NetworkingCastPrivateVerifyAndEncryptDataFunction::Run() {
  std::unique_ptr<cast_api::VerifyAndEncryptData::Params> params =
      cast_api::VerifyAndEncryptData::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  NetworkingCastPrivateDelegate* delegate =
      ExtensionsAPIClient::Get()->GetNetworkingCastPrivateDelegate();
  delegate->VerifyAndEncryptData(
      params->data, AsCastCredentials(params->properties),
      base::Bind(&NetworkingCastPrivateVerifyAndEncryptDataFunction::Success,
                 this),
      base::Bind(&NetworkingCastPrivateVerifyAndEncryptDataFunction::Failure,
                 this));

  // VerifyAndEncryptData might respond synchronously, e.g. in tests.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingCastPrivateVerifyAndEncryptDataFunction::Success(
    const std::string& result) {
  Respond(
      ArgumentList(cast_api::VerifyAndEncryptData::Results::Create(result)));
}

void NetworkingCastPrivateVerifyAndEncryptDataFunction::Failure(
    const std::string& error) {
  Respond(Error(error));
}

NetworkingCastPrivateSetWifiTDLSEnabledStateFunction::
    ~NetworkingCastPrivateSetWifiTDLSEnabledStateFunction() {}

ExtensionFunction::ResponseAction
NetworkingCastPrivateSetWifiTDLSEnabledStateFunction::Run() {
  std::unique_ptr<cast_api::SetWifiTDLSEnabledState::Params> params =
      cast_api::SetWifiTDLSEnabledState::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

#if defined(OS_CHROMEOS)
  chromeos::NetworkHandler::Get()->network_device_handler()->SetWifiTDLSEnabled(
      params->ip_or_mac_address, params->enabled,
      base::Bind(&NetworkingCastPrivateSetWifiTDLSEnabledStateFunction::Success,
                 this),
      base::Bind(&NetworkingCastPrivateSetWifiTDLSEnabledStateFunction::Failure,
                 this));

  // SetWifiTDLSEnabled might respond synchronously, e.g. in tests.
  return did_respond() ? AlreadyResponded() : RespondLater();
#else
  return RespondNow(Error("Not supported"));
#endif
}

#if defined(OS_CHROMEOS)
void NetworkingCastPrivateSetWifiTDLSEnabledStateFunction::Success(
    const std::string& result) {
  Respond(ArgumentList(cast_api::SetWifiTDLSEnabledState::Results::Create(
      ParseTDLSStatus(result))));
}

void NetworkingCastPrivateSetWifiTDLSEnabledStateFunction::Failure(
    const std::string& error,
    std::unique_ptr<base::DictionaryValue> error_data) {
  Respond(Error(error));
}
#endif

NetworkingCastPrivateGetWifiTDLSStatusFunction::
    ~NetworkingCastPrivateGetWifiTDLSStatusFunction() {}

ExtensionFunction::ResponseAction
NetworkingCastPrivateGetWifiTDLSStatusFunction::Run() {
  std::unique_ptr<cast_api::GetWifiTDLSStatus::Params> params =
      cast_api::GetWifiTDLSStatus::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

#if defined(OS_CHROMEOS)
  chromeos::NetworkHandler::Get()->network_device_handler()->GetWifiTDLSStatus(
      params->ip_or_mac_address,
      base::Bind(&NetworkingCastPrivateGetWifiTDLSStatusFunction::Success,
                 this),
      base::Bind(&NetworkingCastPrivateGetWifiTDLSStatusFunction::Failure,
                 this));

  // GetWifiTDLSStatus might respond synchronously, e.g. in tests.
  return did_respond() ? AlreadyResponded() : RespondLater();
#else
  return RespondNow(Error("Not supported"));
#endif
}

#if defined(OS_CHROMEOS)
void NetworkingCastPrivateGetWifiTDLSStatusFunction::Success(
    const std::string& result) {
  Respond(ArgumentList(
      cast_api::GetWifiTDLSStatus::Results::Create(ParseTDLSStatus(result))));
}

void NetworkingCastPrivateGetWifiTDLSStatusFunction::Failure(
    const std::string& error,
    std::unique_ptr<base::DictionaryValue> error_data) {
  Respond(Error(error));
}
#endif

}  // namespace extensions
