// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/chrome_browser_policy_connector.h"

#include <memory>
#include <string>
#include <utility>

#include "base/callback.h"
#include "base/command_line.h"
#include "base/metrics/histogram_macros.h"
#include "base/path_service.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/policy/browser_dm_token_storage.h"
#include "chrome/browser/policy/cloud/machine_level_user_cloud_policy_helper.h"
#include "chrome/browser/policy/configuration_policy_handler_list_factory.h"
#include "chrome/browser/policy/device_management_service_configuration.h"
#include "chrome/common/chrome_paths.h"
#include "components/policy/core/common/async_policy_provider.h"
#include "components/policy/core/common/cloud/cloud_external_data_manager.h"
#include "components/policy/core/common/cloud/cloud_policy_client_registration_helper.h"
#include "components/policy/core/common/cloud/device_management_service.h"
#include "components/policy/core/common/cloud/machine_level_user_cloud_policy_manager.h"
#include "components/policy/core/common/cloud/machine_level_user_cloud_policy_metrics.h"
#include "components/policy/core/common/cloud/machine_level_user_cloud_policy_store.h"
#include "components/policy/core/common/cloud/user_cloud_policy_manager.h"
#include "components/policy/core/common/configuration_policy_provider.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_service.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_switches.h"
#include "net/url_request/url_request_context_getter.h"

#if defined(OS_WIN)
#include "base/win/registry.h"
#include "chrome/install_static/install_util.h"
#include "components/policy/core/common/policy_loader_win.h"
#elif defined(OS_MACOSX)
#include <CoreFoundation/CoreFoundation.h>
#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "components/policy/core/common/policy_loader_mac.h"
#include "components/policy/core/common/preferences_mac.h"
#elif defined(OS_POSIX) && !defined(OS_ANDROID)
#include "components/policy/core/common/config_dir_policy_loader.h"
#elif defined(OS_ANDROID)
#include "components/policy/core/browser/android/android_combined_policy_provider.h"
#endif

namespace policy {

namespace {

#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)

std::unique_ptr<MachineLevelUserCloudPolicyManager>
CreateMachineLevelUserCloudPolicyManager() {
  base::FilePath user_data_dir;
  if (!base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir))
    return nullptr;

  DVLOG(1) << "Creating machine level cloud policy manager";

  base::FilePath policy_dir =
      user_data_dir.Append(ChromeBrowserPolicyConnector::kPolicyDir);
  std::string dm_token = BrowserDMTokenStorage::Get()->RetrieveDMToken();
  std::string client_id = BrowserDMTokenStorage::Get()->RetrieveClientId();
  std::unique_ptr<MachineLevelUserCloudPolicyStore> policy_store =
      MachineLevelUserCloudPolicyStore::Create(
          dm_token, client_id, policy_dir,
          base::CreateSequencedTaskRunnerWithTraits(
              {base::MayBlock(), base::TaskPriority::BACKGROUND}));
  return std::make_unique<MachineLevelUserCloudPolicyManager>(
      std::move(policy_store), nullptr, policy_dir,
      base::ThreadTaskRunnerHandle::Get(),
      content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::IO));
}

void RecordEnrollmentResult(
    MachineLevelUserCloudPolicyEnrollmentResult result) {
  UMA_HISTOGRAM_ENUMERATION(
      "Enterprise.MachineLevelUserCloudPolicyEnrollment.Result", result);
}
#endif

}  // namespace

const base::FilePath::CharType ChromeBrowserPolicyConnector::kPolicyDir[] =
    FILE_PATH_LITERAL("Policy");

ChromeBrowserPolicyConnector::ChromeBrowserPolicyConnector()
    : BrowserPolicyConnector(base::Bind(&BuildHandlerList)) {
}

ChromeBrowserPolicyConnector::~ChromeBrowserPolicyConnector() {}

void ChromeBrowserPolicyConnector::OnResourceBundleCreated() {
  BrowserPolicyConnectorBase::OnResourceBundleCreated();
}

void ChromeBrowserPolicyConnector::Init(
    PrefService* local_state,
    scoped_refptr<net::URLRequestContextGetter> request_context) {
  std::unique_ptr<DeviceManagementService::Configuration> configuration(
      new DeviceManagementServiceConfiguration(
          BrowserPolicyConnector::GetDeviceManagementUrl()));
  std::unique_ptr<DeviceManagementService> device_management_service(
      new DeviceManagementService(std::move(configuration)));
  device_management_service->ScheduleInitialization(
      kServiceInitializationStartupDelay);

  InitInternal(local_state, std::move(device_management_service));

#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
  if (machine_level_user_cloud_policy_manager_)
    InitializeMachineLevelUserCloudPolicies(local_state, request_context);
#endif
}

bool ChromeBrowserPolicyConnector::IsEnterpriseManaged() const {
  NOTREACHED() << "This method is only defined for Chrome OS";
  return false;
}

void ChromeBrowserPolicyConnector::Shutdown() {
#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
  // Reset the registrar and fetcher before calling base class so that
  // shutdown occurs in correct sequence.
  machine_level_user_cloud_policy_registrar_.reset();
  machine_level_user_cloud_policy_fetcher_.reset();
#endif

  BrowserPolicyConnector::Shutdown();
}

ConfigurationPolicyProvider*
ChromeBrowserPolicyConnector::GetPlatformProvider() {
  ConfigurationPolicyProvider* provider =
      BrowserPolicyConnectorBase::GetPolicyProviderForTesting();
  return provider ? provider : platform_provider_;
}

void ChromeBrowserPolicyConnector::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void ChromeBrowserPolicyConnector::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
MachineLevelUserCloudPolicyManager*
ChromeBrowserPolicyConnector::GetMachineLevelUserCloudPolicyManager() {
  return machine_level_user_cloud_policy_manager_;
}
#endif

std::vector<std::unique_ptr<policy::ConfigurationPolicyProvider>>
ChromeBrowserPolicyConnector::CreatePolicyProviders() {
  auto providers = BrowserPolicyConnector::CreatePolicyProviders();
  std::unique_ptr<ConfigurationPolicyProvider> platform_provider =
      CreatePlatformProvider();
  if (platform_provider) {
    platform_provider_ = platform_provider.get();
    // PlatformProvider should be before all other providers (highest priority).
    providers.insert(providers.begin(), std::move(platform_provider));
  }

#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
  std::string enrollment_token =
      BrowserDMTokenStorage::Get()->RetrieveEnrollmentToken();
  std::string dm_token = BrowserDMTokenStorage::Get()->RetrieveDMToken();
  if (!enrollment_token.empty() || !dm_token.empty()) {
    std::unique_ptr<MachineLevelUserCloudPolicyManager> cloud_policy_manager =
        CreateMachineLevelUserCloudPolicyManager();
    if (cloud_policy_manager) {
      machine_level_user_cloud_policy_manager_ = cloud_policy_manager.get();
      providers.push_back(std::move(cloud_policy_manager));
    }
  }
#endif

  return providers;
}

std::unique_ptr<ConfigurationPolicyProvider>
ChromeBrowserPolicyConnector::CreatePlatformProvider() {
#if defined(OS_WIN)
  std::unique_ptr<AsyncPolicyLoader> loader(PolicyLoaderWin::Create(
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND}),
      kRegistryChromePolicyKey));
  return std::make_unique<AsyncPolicyProvider>(GetSchemaRegistry(),
                                               std::move(loader));
#elif defined(OS_MACOSX)
#if defined(GOOGLE_CHROME_BUILD)
  // Explicitly watch the "com.google.Chrome" bundle ID, no matter what this
  // app's bundle ID actually is. All channels of Chrome should obey the same
  // policies.
  CFStringRef bundle_id = CFSTR("com.google.Chrome");
#else
  base::ScopedCFTypeRef<CFStringRef> bundle_id(
      base::SysUTF8ToCFStringRef(base::mac::BaseBundleID()));
#endif
  auto loader = std::make_unique<PolicyLoaderMac>(
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND}),
      policy::PolicyLoaderMac::GetManagedPolicyPath(bundle_id),
      new MacPreferences(), bundle_id);
  return std::make_unique<AsyncPolicyProvider>(GetSchemaRegistry(),
                                               std::move(loader));
#elif defined(OS_POSIX) && !defined(OS_ANDROID)
  base::FilePath config_dir_path;
  if (base::PathService::Get(chrome::DIR_POLICY_FILES, &config_dir_path)) {
    std::unique_ptr<AsyncPolicyLoader> loader(new ConfigDirPolicyLoader(
        base::CreateSequencedTaskRunnerWithTraits(
            {base::MayBlock(), base::TaskPriority::BACKGROUND}),
        config_dir_path, POLICY_SCOPE_MACHINE));
    return std::make_unique<AsyncPolicyProvider>(GetSchemaRegistry(),
                                                 std::move(loader));
  } else {
    return nullptr;
  }
#elif defined(OS_ANDROID)
  return std::make_unique<policy::android::AndroidCombinedPolicyProvider>(
      GetSchemaRegistry());
#else
  return nullptr;
#endif
}

#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)

void ChromeBrowserPolicyConnector::InitializeMachineLevelUserCloudPolicies(
    PrefService* local_state,
    scoped_refptr<net::URLRequestContextGetter> request_context) {
  // If there exists an enrollment token, then there are two states:
  //   1/ There also exists a DM token.  This machine is already registeted, so
  //      the next step is to fetch policies.
  //   2/ There is no DM token.  In this case the machine is not already
  //      registered and needs to request a DM token.
  std::string enrollment_token;
  std::string client_id;

  if (!GetEnrollmentTokenAndClientId(&enrollment_token, &client_id))
    return;

  DCHECK(!enrollment_token.empty());
  DCHECK(!client_id.empty());
  DVLOG(1) << "Enrollment token = " << enrollment_token;
  DVLOG(1) << "Client ID = " << client_id;

  machine_level_user_cloud_policy_registrar_ =
      std::make_unique<MachineLevelUserCloudPolicyRegistrar>(
          device_management_service(), request_context);
  machine_level_user_cloud_policy_fetcher_ =
      std::make_unique<MachineLevelUserCloudPolicyFetcher>(
          machine_level_user_cloud_policy_manager_, local_state,
          device_management_service(), request_context);

  std::string dm_token = BrowserDMTokenStorage::Get()->RetrieveDMToken();
  DVLOG(1) << "DM token = " << (dm_token.empty() ? "none" : "from persistence");

  if (dm_token.empty()) {
    // Not registered already, so do it now.
    machine_level_user_cloud_policy_registrar_
        ->RegisterForPolicyWithEnrollmentToken(
            enrollment_token, client_id,
            base::Bind(&ChromeBrowserPolicyConnector::
                           RegisterForPolicyWithEnrollmentTokenCallback,
                       base::Unretained(this)));
#if defined(OS_WIN)
    // This metric is only published on Windows to indicate how many user level
    // install Chrome try to enroll the policy which can't store the DM token
    // in the Registry in the end of enrollment. Mac and Linux does not need
    // this metric for now as they might use different token storage mechanism
    // in the future.
    UMA_HISTOGRAM_BOOLEAN(
        "Enterprise.MachineLevelUserCloudPolicyEnrollment.InstallLevel_Win",
        install_static::IsSystemInstall());
#endif
  }
}

bool ChromeBrowserPolicyConnector::GetEnrollmentTokenAndClientId(
    std::string* enrollment_token,
    std::string* client_id) {
  *client_id = BrowserDMTokenStorage::Get()->RetrieveClientId();
  if (client_id->empty())
    return false;

  *enrollment_token = BrowserDMTokenStorage::Get()->RetrieveEnrollmentToken();
  return !enrollment_token->empty();
}

void ChromeBrowserPolicyConnector::RegisterForPolicyWithEnrollmentTokenCallback(
    const std::string& dm_token,
    const std::string& client_id) {
  if (dm_token.empty()) {
    DVLOG(1) << "No DM token returned from browser registration";
    RecordEnrollmentResult(
        MachineLevelUserCloudPolicyEnrollmentResult::kFailedToFetch);
    NotifyMachineLevelUserCloudPolicyRegisterFinished(false);
    return;
  }

  DVLOG(1) << "DM token = retrieved from server";

  // TODO(alito): Log failures to store the DM token. Should we try again later?
  BrowserDMTokenStorage::Get()->StoreDMToken(
      dm_token, base::BindOnce([](bool success) {
        if (!success) {
          DVLOG(1) << "Failed to store the DM token";
          RecordEnrollmentResult(
              MachineLevelUserCloudPolicyEnrollmentResult::kFailedToStore);
        } else {
          DVLOG(1) << "Successfully stored the DM token";
          RecordEnrollmentResult(
              MachineLevelUserCloudPolicyEnrollmentResult::kSuccess);
        }
      }));

  // Start fetching policies.
  machine_level_user_cloud_policy_fetcher_->SetupRegistrationAndFetchPolicy(
      dm_token, client_id);
  NotifyMachineLevelUserCloudPolicyRegisterFinished(true);
}

void ChromeBrowserPolicyConnector::
    NotifyMachineLevelUserCloudPolicyRegisterFinished(bool succeeded) {
  for (auto& observer : observers_) {
    observer.OnMachineLevelUserCloudPolicyRegisterFinished(succeeded);
  }
}

#endif  // !defined(OS_ANDROID) && !defined(OS_CHROMEOS)

}  // namespace policy
