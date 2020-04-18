// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_POLICY_CHROME_BROWSER_POLICY_CONNECTOR_H_
#define CHROME_BROWSER_POLICY_CHROME_BROWSER_POLICY_CONNECTOR_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "build/build_config.h"
#include "components/policy/core/browser/browser_policy_connector.h"

class PrefService;

namespace net {
class URLRequestContextGetter;
}

namespace policy {

class ConfigurationPolicyProvider;
class MachineLevelUserCloudPolicyManager;
class MachineLevelUserCloudPolicyFetcher;
class MachineLevelUserCloudPolicyRegistrar;

// Extends BrowserPolicyConnector with the setup shared among the desktop
// implementations and Android.
class ChromeBrowserPolicyConnector : public BrowserPolicyConnector {
 public:
  // Service initialization delay time in millisecond on startup. (So that
  // displaying Chrome's GUI does not get delayed.)
  static const int64_t kServiceInitializationStartupDelay = 5000;

  // Directory name under the user-data-dir where machine level user cloud
  // policy data is stored.
  static const base::FilePath::CharType kPolicyDir[];

  // Builds an uninitialized ChromeBrowserPolicyConnector, suitable for testing.
  // Init() should be called to create and start the policy machinery.
  ChromeBrowserPolicyConnector();

  ~ChromeBrowserPolicyConnector() override;

  // Called once the resource bundle has been created. Calls through to super
  // class to notify observers.
  void OnResourceBundleCreated();

  void Init(
      PrefService* local_state,
      scoped_refptr<net::URLRequestContextGetter> request_context) override;

  bool IsEnterpriseManaged() const override;

  void Shutdown() override;

  ConfigurationPolicyProvider* GetPlatformProvider();

  class Observer {
   public:
    virtual ~Observer() {}

    // Called when machine level user cloud policy enrollment is finished.
    // |succeeded| is true if |dm_token| is returned from the server.
    virtual void OnMachineLevelUserCloudPolicyRegisterFinished(bool succeeded) {
    }
  };

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
  MachineLevelUserCloudPolicyManager* GetMachineLevelUserCloudPolicyManager();
#endif

 protected:
  // BrowserPolicyConnector:
  std::vector<std::unique_ptr<policy::ConfigurationPolicyProvider>>
  CreatePolicyProviders() override;

 private:
  std::unique_ptr<ConfigurationPolicyProvider> CreatePlatformProvider();

#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
  void InitializeMachineLevelUserCloudPolicies(
      PrefService* local_state,
      scoped_refptr<net::URLRequestContextGetter> request_context);
  bool GetEnrollmentTokenAndClientId(std::string* enrollment_token,
                                     std::string* client_id);
  void RegisterForPolicyWithEnrollmentTokenCallback(
      const std::string& dm_token,
      const std::string& client_id);

  void NotifyMachineLevelUserCloudPolicyRegisterFinished(bool succeeded);

  // Owned by base class.
  MachineLevelUserCloudPolicyManager* machine_level_user_cloud_policy_manager_ =
      nullptr;

  std::unique_ptr<MachineLevelUserCloudPolicyRegistrar>
      machine_level_user_cloud_policy_registrar_;
  std::unique_ptr<MachineLevelUserCloudPolicyFetcher>
      machine_level_user_cloud_policy_fetcher_;

#endif
  base::ObserverList<Observer, true> observers_;

  // Owned by base class.
  ConfigurationPolicyProvider* platform_provider_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(ChromeBrowserPolicyConnector);
};

}  // namespace policy

#endif  // CHROME_BROWSER_POLICY_CHROME_BROWSER_POLICY_CONNECTOR_H_
