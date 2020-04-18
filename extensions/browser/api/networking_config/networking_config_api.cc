// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "extensions/browser/api/networking_config/networking_config_api.h"
#include "extensions/browser/api/networking_config/networking_config_service.h"
#include "extensions/browser/api/networking_config/networking_config_service_factory.h"
#include "ui/base/l10n/l10n_util.h"

namespace extensions {

namespace {

const char kAuthenticationResultFailed[] =
    "Failed to set AuthenticationResult.";
const char kMalformedFilterDescription[] = "Malformed filter description.";
const char kMalformedFilterDescriptionWithSSID[] =
    "Malformed filter description. Failed to register network with SSID "
    "(hex): *";
const char kUnsupportedNetworkType[] = "Unsupported network type.";

}  // namespace

NetworkingConfigSetNetworkFilterFunction::
    NetworkingConfigSetNetworkFilterFunction() {
}

ExtensionFunction::ResponseAction
NetworkingConfigSetNetworkFilterFunction::Run() {
  parameters_ =
      api::networking_config::SetNetworkFilter::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(parameters_.get());

  NetworkingConfigService* service =
      NetworkingConfigServiceFactory::GetForBrowserContext(browser_context());
  DCHECK(service);

  // Remove previously registered networks.
  service->UnregisterExtension(extension_id());

  for (const api::networking_config::NetworkInfo& ni : parameters_->networks) {
    // |Type| field must be set to |WiFi|
    if (ni.type != api::networking_config::NETWORK_TYPE_WIFI)
      return RespondNow(Error(kUnsupportedNetworkType));

    // Either |ssid| or |hex_ssid| must be set.
    if (!ni.ssid.get() && !ni.hex_ssid.get())
      return RespondNow(Error(kMalformedFilterDescription));

    std::string hex_ssid;
    if (ni.ssid.get()) {
      auto* ssid_field = ni.ssid.get();
      hex_ssid = base::HexEncode(ssid_field->c_str(), ssid_field->size());
    }
    if (ni.hex_ssid.get())
      hex_ssid = *ni.hex_ssid.get();

    if (!service->RegisterHexSsid(hex_ssid, extension_id()))
      return RespondNow(Error(kMalformedFilterDescriptionWithSSID, hex_ssid));
  }

  return RespondNow(NoArguments());
}

NetworkingConfigSetNetworkFilterFunction::
    ~NetworkingConfigSetNetworkFilterFunction() {
}

NetworkingConfigFinishAuthenticationFunction::
    NetworkingConfigFinishAuthenticationFunction() {
}

ExtensionFunction::ResponseAction
NetworkingConfigFinishAuthenticationFunction::Run() {
  parameters_ =
      api::networking_config::FinishAuthentication::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(parameters_.get());

  NetworkingConfigService* service =
      NetworkingConfigServiceFactory::GetForBrowserContext(browser_context());
  DCHECK(service);

  const NetworkingConfigService::AuthenticationResult& last_result =
      service->GetAuthenticationResult();
  if (last_result.authentication_state != NetworkingConfigService::NOTRY ||
      last_result.guid != parameters_->guid) {
    return RespondNow(Error(kAuthenticationResultFailed));
  }

  // Populate NetworkingCaptivePortalAPI::AuthenticationResult.
  NetworkingConfigService::AuthenticationResult authentication_result = {
      extension_id(), parameters_->guid, NetworkingConfigService::FAILED,
  };
  switch (parameters_->result) {
    case api::networking_config::AUTHENTICATION_RESULT_NONE:
      NOTREACHED();
      break;
    case api::networking_config::AUTHENTICATION_RESULT_UNHANDLED:
      authentication_result.authentication_state =
          NetworkingConfigService::FAILED;
      break;
    case api::networking_config::AUTHENTICATION_RESULT_REJECTED:
      authentication_result.authentication_state =
          NetworkingConfigService::REJECTED;
      break;
    case api::networking_config::AUTHENTICATION_RESULT_FAILED:
      authentication_result.authentication_state =
          NetworkingConfigService::FAILED;
      break;
    case api::networking_config::AUTHENTICATION_RESULT_SUCCEEDED:
      authentication_result.authentication_state =
          NetworkingConfigService::SUCCESS;
      break;
  }
  service->SetAuthenticationResult(authentication_result);
  return RespondNow(NoArguments());
}

NetworkingConfigFinishAuthenticationFunction::
    ~NetworkingConfigFinishAuthenticationFunction() {
}

}  // namespace extensions
