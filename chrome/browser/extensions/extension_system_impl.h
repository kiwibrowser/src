// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_SYSTEM_IMPL_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_SYSTEM_IMPL_H_

#include <string>

#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/extension_cookie_notifier.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/one_shot_event.h"

class Profile;

#if defined(OS_CHROMEOS)
namespace chromeos {
class DeviceLocalAccountManagementPolicyProvider;
class SigninScreenPolicyProvider;
}
#endif  // defined(OS_CHROMEOS)

namespace extensions {

class ExtensionSystemSharedFactory;
class NavigationObserver;
class StateStoreNotificationObserver;
class UninstallPingSender;
class InstallGate;
class ValueStoreFactory;
class ValueStoreFactoryImpl;

// The ExtensionSystem for ProfileImpl and OffTheRecordProfileImpl.
// Implementation details: non-shared services are owned by
// ExtensionSystemImpl, a KeyedService with separate incognito
// instances. A private Shared class (also a KeyedService,
// but with a shared instance for incognito) keeps the common services.
class ExtensionSystemImpl : public ExtensionSystem {
 public:
  using InstallUpdateCallback = ExtensionSystem::InstallUpdateCallback;

  explicit ExtensionSystemImpl(Profile* profile);
  ~ExtensionSystemImpl() override;

  // KeyedService implementation.
  void Shutdown() override;

  void InitForRegularProfile(bool extensions_enabled) override;
  void InitForIncognitoProfile() override;

  ExtensionService* extension_service() override;  // shared
  RuntimeData* runtime_data() override;            // shared
  ManagementPolicy* management_policy() override;  // shared
  ServiceWorkerManager* service_worker_manager() override;  // shared
  SharedUserScriptMaster* shared_user_script_master() override;  // shared
  StateStore* state_store() override;                              // shared
  StateStore* rules_store() override;                              // shared
  scoped_refptr<ValueStoreFactory> store_factory() override;       // shared
  InfoMap* info_map() override;                                    // shared
  QuotaService* quota_service() override;  // shared
  AppSorting* app_sorting() override;  // shared

  void RegisterExtensionWithRequestContexts(
      const Extension* extension,
      const base::Closure& callback) override;

  void UnregisterExtensionWithRequestContexts(
      const std::string& extension_id,
      const UnloadedExtensionReason reason) override;

  const OneShotEvent& ready() const override;
  ContentVerifier* content_verifier() override;  // shared
  std::unique_ptr<ExtensionSet> GetDependentExtensions(
      const Extension* extension) override;
  void InstallUpdate(const std::string& extension_id,
                     const std::string& public_key,
                     const base::FilePath& unpacked_dir,
                     InstallUpdateCallback install_update_callback) override;
  bool FinishDelayedInstallationIfReady(const std::string& extension_id,
                                        bool install_immediately) override;

 private:
  friend class ExtensionSystemSharedFactory;

  // Owns the Extension-related systems that have a single instance
  // shared between normal and incognito profiles.
  class Shared : public KeyedService, public content::NotificationObserver {
   public:
    explicit Shared(Profile* profile);
    ~Shared() override;

    // Initialization takes place in phases.
    virtual void InitPrefs();
    // This must not be called until all the providers have been created.
    void RegisterManagementPolicyProviders();
    void InitInstallGates();
    void Init(bool extensions_enabled);

    // KeyedService implementation.
    void Shutdown() override;

    StateStore* state_store();
    StateStore* rules_store();
    scoped_refptr<ValueStoreFactory> store_factory() const;
    ExtensionService* extension_service();
    RuntimeData* runtime_data();
    ManagementPolicy* management_policy();
    ServiceWorkerManager* service_worker_manager();
    SharedUserScriptMaster* shared_user_script_master();
    InfoMap* info_map();
    QuotaService* quota_service();
    AppSorting* app_sorting();
    const OneShotEvent& ready() const { return ready_; }
    ContentVerifier* content_verifier();

   private:
    // content::NotificationObserver implementation.
    void Observe(int type,
                 const content::NotificationSource& source,
                 const content::NotificationDetails& details) override;

    Profile* profile_;

    // The services that are shared between normal and incognito profiles.

    content::NotificationRegistrar registrar_;
    std::unique_ptr<StateStore> state_store_;
    std::unique_ptr<StateStoreNotificationObserver>
        state_store_notification_observer_;
    std::unique_ptr<StateStore> rules_store_;
    scoped_refptr<ValueStoreFactoryImpl> store_factory_;
    std::unique_ptr<NavigationObserver> navigation_observer_;
    std::unique_ptr<ServiceWorkerManager> service_worker_manager_;
    // Shared memory region manager for scripts statically declared in extension
    // manifests. This region is shared between all extensions.
    std::unique_ptr<SharedUserScriptMaster> shared_user_script_master_;
    std::unique_ptr<RuntimeData> runtime_data_;
    // ExtensionService depends on StateStore, Blacklist and RuntimeData.
    std::unique_ptr<ExtensionService> extension_service_;
    std::unique_ptr<ManagementPolicy> management_policy_;
    // extension_info_map_ needs to outlive process_manager_.
    scoped_refptr<InfoMap> extension_info_map_;
    std::unique_ptr<QuotaService> quota_service_;
    std::unique_ptr<AppSorting> app_sorting_;
    std::unique_ptr<InstallGate> update_install_gate_;

    // For verifying the contents of extensions read from disk.
    scoped_refptr<ContentVerifier> content_verifier_;

    std::unique_ptr<UninstallPingSender> uninstall_ping_sender_;

#if defined(OS_CHROMEOS)
    std::unique_ptr<chromeos::DeviceLocalAccountManagementPolicyProvider>
        device_local_account_management_policy_provider_;
    std::unique_ptr<chromeos::SigninScreenPolicyProvider>
        signin_screen_policy_provider_;
    std::unique_ptr<InstallGate> kiosk_app_update_install_gate_;
#endif

    OneShotEvent ready_;
  };

  std::unique_ptr<ExtensionCookieNotifier> cookie_notifier_;

  Profile* profile_;

  Shared* shared_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionSystemImpl);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_SYSTEM_IMPL_H_
