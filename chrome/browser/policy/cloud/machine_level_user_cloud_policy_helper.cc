// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/cloud/machine_level_user_cloud_policy_helper.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/logging.h"
#include "chrome/browser/policy/browser_dm_token_storage.h"
#include "components/policy/core/common/cloud/cloud_policy_client_registration_helper.h"
#include "components/policy/core/common/cloud/cloud_policy_core.h"
#include "components/policy/core/common/cloud/cloud_policy_manager.h"
#include "components/policy/core/common/cloud/cloud_policy_service.h"
#include "components/policy/core/common/cloud/device_management_service.h"
#include "components/policy/core/common/cloud/machine_level_user_cloud_policy_manager.h"
#include "components/policy/core/common/cloud/machine_level_user_cloud_policy_store.h"
#include "components/prefs/pref_service.h"
#include "net/url_request/url_request_context_getter.h"

namespace policy {

/* MachineLevelUserCloudPolicyRegistrar */
MachineLevelUserCloudPolicyRegistrar::MachineLevelUserCloudPolicyRegistrar(
    DeviceManagementService* device_management_service,
    scoped_refptr<net::URLRequestContextGetter> system_request_context)
    : device_management_service_(device_management_service),
      system_request_context_(system_request_context) {}

MachineLevelUserCloudPolicyRegistrar::~MachineLevelUserCloudPolicyRegistrar() {}

void MachineLevelUserCloudPolicyRegistrar::RegisterForPolicyWithEnrollmentToken(
    const std::string& enrollment_token,
    const std::string& client_id,
    const PolicyRegistrationCallback& callback) {
  DCHECK(!enrollment_token.empty());
  DCHECK(!client_id.empty());

  // If the DeviceManagementService is not yet initialized, start it up now.
  device_management_service_->ScheduleInitialization(0);

  // Create a new CloudPolicyClient for fetching the DMToken.  This is
  // distinct from the CloudPolicyClient in MachineLevelUserCloudPolicyManager,
  // which is used for policy fetching.  The client created here should only be
  // used for enrollment and gets destroyed when |registration_helper_| is
  // reset.
  std::unique_ptr<CloudPolicyClient> policy_client =
      std::make_unique<CloudPolicyClient>(
          std::string() /* machine_id */, std::string() /* machine_model */,
          std::string() /* brand_code */, device_management_service_,
          system_request_context_, nullptr,
          CloudPolicyClient::DeviceDMTokenCallback());

  // Fire off the registration process. Callback keeps the CloudPolicyClient
  // alive for the length of the registration process. Use the system
  // request context because the user is not signed in to this profile.
  registration_helper_ = std::make_unique<CloudPolicyClientRegistrationHelper>(
      policy_client.get(),
      enterprise_management::DeviceRegisterRequest::BROWSER);
  registration_helper_->StartRegistrationWithEnrollmentToken(
      enrollment_token, client_id,
      base::BindRepeating(
          &MachineLevelUserCloudPolicyRegistrar::CallPolicyRegistrationCallback,
          base::Unretained(this), base::Passed(&policy_client), callback));
}

void MachineLevelUserCloudPolicyRegistrar::CallPolicyRegistrationCallback(
    std::unique_ptr<CloudPolicyClient> client,
    PolicyRegistrationCallback callback) {
  registration_helper_.reset();
  if (callback)
    callback.Run(client->dm_token(), client->client_id());
}

/* MachineLevelUserCloudPolicyFetcher */
MachineLevelUserCloudPolicyFetcher::MachineLevelUserCloudPolicyFetcher(
    MachineLevelUserCloudPolicyManager* policy_manager,
    PrefService* local_state,
    DeviceManagementService* device_management_service,
    scoped_refptr<net::URLRequestContextGetter> system_request_context)
    : policy_manager_(policy_manager),
      local_state_(local_state),
      device_management_service_(device_management_service),
      system_request_context_(system_request_context) {
  std::unique_ptr<CloudPolicyClient> client =
      std::make_unique<CloudPolicyClient>(
          std::string() /* machine_id */, std::string() /* machine_model */,
          std::string() /* brand_code */, device_management_service_,
          system_request_context_, nullptr,
          CloudPolicyClient::DeviceDMTokenCallback());
  InitializeManager(std::move(client));
}

MachineLevelUserCloudPolicyFetcher::~MachineLevelUserCloudPolicyFetcher() {
  policy_manager_->core()->service()->RemoveObserver(this);
}

void MachineLevelUserCloudPolicyFetcher::SetupRegistrationAndFetchPolicy(
    const std::string& dm_token,
    const std::string& client_id) {
  policy_manager_->core()->client()->SetupRegistration(
      dm_token, client_id, std::vector<std::string>());
  policy_manager_->store()->SetupRegistration(dm_token, client_id);
  DCHECK(policy_manager_->IsClientRegistered());

  policy_manager_->core()->service()->RefreshPolicy(base::DoNothing());
}

void MachineLevelUserCloudPolicyFetcher::OnInitializationCompleted(
    CloudPolicyService* service) {
  // Client will be registered before policy fetch. A non-registered client
  // means there is no validated policy cache on the disk while the device has
  // been enrolled already. Hence, we need to fetch the
  // policy based on the global dm_token.
  //
  // Note that Chrome will not fetch policy again immediately here if DM server
  // returns a policy that Chrome is not able to validate.
  if (!policy_manager_->IsClientRegistered()) {
    TryToFetchPolicy();
  }
}

void MachineLevelUserCloudPolicyFetcher::InitializeManager(
    std::unique_ptr<CloudPolicyClient> client) {
  policy_manager_->Connect(local_state_, system_request_context_,
                           std::move(client));
  policy_manager_->core()->service()->AddObserver(this);

  // If CloudPolicyStore is already initialized then |OnInitializationCompleted|
  // has already fired. Fetch policy if CloudPolicyClient hasn't been registered
  // which means there is no valid policy cache.
  if (policy_manager_->store()->is_initialized() &&
      !policy_manager_->IsClientRegistered()) {
    TryToFetchPolicy();
  }
}

void MachineLevelUserCloudPolicyFetcher::TryToFetchPolicy() {
  std::string dm_token = BrowserDMTokenStorage::Get()->RetrieveDMToken();
  std::string client_id = BrowserDMTokenStorage::Get()->RetrieveClientId();
  if (!dm_token.empty() && !client_id.empty())
    SetupRegistrationAndFetchPolicy(dm_token, client_id);
}

}  // namespace policy
