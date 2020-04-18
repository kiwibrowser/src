// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_USER_POLICY_MANAGER_FACTORY_CHROMEOS_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_USER_POLICY_MANAGER_FACTORY_CHROMEOS_H_

#include <map>
#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_base_factory.h"

class Profile;

namespace base {
class SequencedTaskRunner;
}

namespace content {
class BrowserContext;
}

namespace policy {

class ConfigurationPolicyProvider;
class ActiveDirectoryPolicyManager;
class UserCloudPolicyManagerChromeOS;

// Shared BrowserContextKeyedBaseFactory implementation for
// UserCloudPolicyManagerChromeOS and ActiveDirectoryPolicyManager.
//
// UserCloudPolicyManagerChromeOS/ActiveDirectoryPolicyManager is handled
// differently than other KeyedServices because it is a dependency of
// PrefService.  Therefore, lifetime of instances is managed by Profile, Profile
// startup code invokes CreateForProfile() explicitly, takes ownership, and the
// instance is only deleted after PrefService destruction.
//
// TODO(mnissler): Remove the special lifetime management in favor of
// PrefService directly depending on UserCloudPolicyManagerChromeOS once the
// former has been converted to a KeyedService.
// See also http://crbug.com/131843 and http://crbug.com/131844.
class UserPolicyManagerFactoryChromeOS : public BrowserContextKeyedBaseFactory {
 public:
  // Returns an instance of the UserPolicyManagerFactoryChromeOS singleton.
  static UserPolicyManagerFactoryChromeOS* GetInstance();

  // Get the ConfigurationPolicyProvider instance associated with |profile|.
  // Depending on the type of management, either a
  // UserCloudPolicyManagerChromeOS or an ActiveDirectoryPolicyManager or
  // nullptr is returned.
  static ConfigurationPolicyProvider* GetForProfile(Profile* profile);

  // Returns the UserCloudPolicyManagerChromeOS instance associated with
  // |profile| or nullptr in case there is none.
  static UserCloudPolicyManagerChromeOS* GetCloudPolicyManagerForProfile(
      Profile* profile);

  // Returns the ActiveDirectoryPolicyManager instance associated with |profile|
  // or nullptr in case there is none.
  static ActiveDirectoryPolicyManager*
  GetActiveDirectoryPolicyManagerForProfile(Profile* profile);

  // Creates an instance for |profile|. Note that the caller is responsible for
  // managing the lifetime of the instance. Subsequent calls to GetForProfile()
  // will return the created instance as long as it lives.
  //
  // If |force_immediate_load| is true, policy is loaded synchronously from
  // UserCloudPolicyStore at startup.
  static std::unique_ptr<ConfigurationPolicyProvider> CreateForProfile(
      Profile* profile,
      bool force_immediate_load,
      scoped_refptr<base::SequencedTaskRunner> background_task_runner);

 private:
  friend struct base::DefaultSingletonTraits<UserPolicyManagerFactoryChromeOS>;

  UserPolicyManagerFactoryChromeOS();
  ~UserPolicyManagerFactoryChromeOS() override;

  // See comments for the static versions above.
  UserCloudPolicyManagerChromeOS* GetCloudPolicyManager(Profile* profile);
  ActiveDirectoryPolicyManager* GetActiveDirectoryPolicyManager(
      Profile* profile);
  std::unique_ptr<ConfigurationPolicyProvider> CreateManagerForProfile(
      Profile* profile,
      bool force_immediate_load,
      scoped_refptr<base::SequencedTaskRunner> background_task_runner);

  // BrowserContextKeyedBaseFactory:
  void BrowserContextShutdown(content::BrowserContext* context) override;
  void BrowserContextDestroyed(content::BrowserContext* context) override;
  void SetEmptyTestingFactory(content::BrowserContext* context) override;
  bool HasTestingFactory(content::BrowserContext* context) override;
  void CreateServiceNow(content::BrowserContext* context) override;

  std::map<Profile*, UserCloudPolicyManagerChromeOS*> cloud_managers_;
  std::map<Profile*, ActiveDirectoryPolicyManager*> active_directory_managers_;

  DISALLOW_COPY_AND_ASSIGN(UserPolicyManagerFactoryChromeOS);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_USER_POLICY_MANAGER_FACTORY_CHROMEOS_H_
