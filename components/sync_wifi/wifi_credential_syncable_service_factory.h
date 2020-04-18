// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_WIFI_WIFI_CREDENTIAL_SYNCABLE_SERVICE_FACTORY_H_
#define COMPONENTS_SYNC_WIFI_WIFI_CREDENTIAL_SYNCABLE_SERVICE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "build/build_config.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace content {
class BrowserContext;
}

namespace sync_wifi {

class WifiConfigDelegate;
class WifiCredentialSyncableService;

// Singleton that owns all WifiCredentialSyncableServices and
// associates them with Profiles. Listens for the Profile's
// destruction notification and cleans up the associated
// WifiCredentialSyncableServices.
class WifiCredentialSyncableServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  // Returns the SyncableService for |browser_context|, creating the
  // SyncableService if one does not already exist.
  static WifiCredentialSyncableService* GetForBrowserContext(
      content::BrowserContext* browser_context);

  // Returns the singleton instance.
  static WifiCredentialSyncableServiceFactory* GetInstance();

#if defined(OS_CHROMEOS)
  void set_ignore_login_state_for_test(bool new_value) {
    ignore_login_state_for_test_ = new_value;
  }
#endif

 private:
  friend struct base::DefaultSingletonTraits<
      WifiCredentialSyncableServiceFactory>;

  WifiCredentialSyncableServiceFactory();
  ~WifiCredentialSyncableServiceFactory() override;

  // BrowserContextKeyedServiceFactory implementation.
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

#if defined(OS_CHROMEOS)
  // Returns a scoped pointer to a WifiConfigDelegate, which can be
  // used to configure the ChromeOS Wi-Fi settings associated with
  // |context|.
  std::unique_ptr<WifiConfigDelegate> BuildWifiConfigDelegateChromeOs(
      content::BrowserContext* context) const;
#endif

#if defined(OS_CHROMEOS)
  // Whether or not we should use LoginState to associate a new
  // SyncableService with a Shill profile. Should be set to true in
  // sync integration tests, where it is not possible to control
  // LoginState at the time SyncableServices are constructed.
  bool ignore_login_state_for_test_ = false;
#endif

  DISALLOW_COPY_AND_ASSIGN(WifiCredentialSyncableServiceFactory);
};

}  // namespace sync_wifi

#endif  // COMPONENTS_SYNC_WIFI_WIFI_CREDENTIAL_SYNCABLE_SERVICE_FACTORY_H_
