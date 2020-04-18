// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_wifi/wifi_credential_syncable_service_factory.h"

#include <memory>
#include <string>

#include "base/logging.h"
#include "build/build_config.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/sync_wifi/wifi_config_delegate.h"
#include "components/sync_wifi/wifi_credential_syncable_service.h"
#include "content/public/browser/browser_context.h"

#if defined(OS_CHROMEOS)
#include "base/files/file_path.h"
#include "chromeos/login/login_state.h"
#include "chromeos/network/network_handler.h"
#include "components/sync_wifi/wifi_config_delegate_chromeos.h"
#endif

namespace sync_wifi {

namespace {

#if defined(OS_CHROMEOS)
// Returns a string identifying a ChromeOS network settings profile,
// by that profile's UserHash property. This value may be communicated
// to the ChromeOS connection manager ("Shill"), but must not be
// exposed to any untrusted code (e.g., via web APIs).
std::string GetUserHash(content::BrowserContext* context,
                        bool use_login_state) {
  if (use_login_state) {
    const chromeos::LoginState* login_state = chromeos::LoginState::Get();
    DCHECK(login_state->IsUserLoggedIn());
    DCHECK(!login_state->primary_user_hash().empty());
    // TODO(quiche): Verify that |context| is the primary user's context.
    return login_state->primary_user_hash();
  } else {
    // In WiFi credential sync tests, LoginState is not
    // available. Instead, those tests set their Shill profiles'
    // UserHashes based on the corresponding BrowserContexts' storage
    // paths.
    return context->GetPath().BaseName().value();
  }
}
#endif

}  // namespace

// static
WifiCredentialSyncableService*
WifiCredentialSyncableServiceFactory::GetForBrowserContext(
    content::BrowserContext* browser_context) {
  return static_cast<WifiCredentialSyncableService*>(
      GetInstance()->GetServiceForBrowserContext(browser_context, true));
}

// static
WifiCredentialSyncableServiceFactory*
WifiCredentialSyncableServiceFactory::GetInstance() {
  return base::Singleton<WifiCredentialSyncableServiceFactory>::get();
}

// Private methods.

WifiCredentialSyncableServiceFactory::WifiCredentialSyncableServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "WifiCredentialSyncableService",
          BrowserContextDependencyManager::GetInstance()) {}

WifiCredentialSyncableServiceFactory::~WifiCredentialSyncableServiceFactory() {}

KeyedService* WifiCredentialSyncableServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
// TODO(quiche): Figure out if this behaves properly for multi-profile.
// crbug.com/430681.
#if defined(OS_CHROMEOS)
  return new WifiCredentialSyncableService(
      BuildWifiConfigDelegateChromeOs(context));
#else
  NOTREACHED();
  return nullptr;
#endif
}

#if defined(OS_CHROMEOS)
std::unique_ptr<WifiConfigDelegate>
WifiCredentialSyncableServiceFactory::BuildWifiConfigDelegateChromeOs(
    content::BrowserContext* context) const {
  // Note: NetworkHandler is a singleton that is managed by
  // ChromeBrowserMainPartsChromeos, and destroyed after all
  // KeyedService instances are destroyed.
  chromeos::NetworkHandler* network_handler = chromeos::NetworkHandler::Get();
  return std::make_unique<WifiConfigDelegateChromeOs>(
      GetUserHash(context, !ignore_login_state_for_test_),
      network_handler->managed_network_configuration_handler());
}
#endif

}  // namespace sync_wifi
