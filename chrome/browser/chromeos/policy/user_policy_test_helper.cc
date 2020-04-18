// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/user_policy_test_helper.h"

#include <utility>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_writer.h"
#include "base/run_loop.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/policy/user_cloud_policy_manager_chromeos.h"
#include "chrome/browser/chromeos/policy/user_policy_manager_factory_chromeos.h"
#include "chrome/browser/policy/chrome_browser_policy_connector.h"
#include "chrome/browser/policy/profile_policy_connector.h"
#include "chrome/browser/policy/profile_policy_connector_factory.h"
#include "chrome/browser/policy/test/local_policy_test_server.h"
#include "chrome/browser/profiles/profile.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/cloud/cloud_policy_client.h"
#include "components/policy/core/common/cloud/cloud_policy_constants.h"
#include "components/policy/core/common/cloud/cloud_policy_core.h"
#include "components/policy/core/common/policy_service.h"
#include "components/policy/core/common/policy_switches.h"
#include "components/policy/proto/device_management_backend.pb.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace policy {

namespace {

std::string BuildPolicy(const base::DictionaryValue& mandatory,
                        const base::DictionaryValue& recommended,
                        const std::string& policyType,
                        const std::string& account_id) {
  std::unique_ptr<base::DictionaryValue> policy_type_dict(
      new base::DictionaryValue);
  policy_type_dict->SetWithoutPathExpansion("mandatory",
                                            mandatory.CreateDeepCopy());
  policy_type_dict->SetWithoutPathExpansion("recommended",
                                            recommended.CreateDeepCopy());

  std::unique_ptr<base::ListValue> managed_users_list(new base::ListValue);
  managed_users_list->AppendString("*");

  base::DictionaryValue root_dict;
  root_dict.SetWithoutPathExpansion(policyType, std::move(policy_type_dict));
  root_dict.SetWithoutPathExpansion("managed_users",
                                    std::move(managed_users_list));
  root_dict.SetKey("policy_user", base::Value(account_id));
  root_dict.SetKey("current_key_index", base::Value(0));

  std::string json_policy;
  base::JSONWriter::WriteWithOptions(
      root_dict, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json_policy);
  return json_policy;
}

}  // namespace

UserPolicyTestHelper::UserPolicyTestHelper(const std::string& account_id)
    : account_id_(account_id) {
}

UserPolicyTestHelper::~UserPolicyTestHelper() {
}

void UserPolicyTestHelper::Init(
    const base::DictionaryValue& mandatory_policy,
    const base::DictionaryValue& recommended_policy) {
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  WritePolicyFile(mandatory_policy, recommended_policy);

  test_server_.reset(new LocalPolicyTestServer(PolicyFilePath()));
  ASSERT_TRUE(test_server_->Start());
}

void UserPolicyTestHelper::UpdateCommandLine(
    base::CommandLine* command_line) const {
  command_line->AppendSwitchASCII(policy::switches::kDeviceManagementUrl,
                                  test_server_->GetServiceURL().spec());
}

void UserPolicyTestHelper::WaitForInitialPolicy(Profile* profile) {
  BrowserPolicyConnector* const connector =
      g_browser_process->browser_policy_connector();
  connector->ScheduleServiceInitialization(0);

  UserCloudPolicyManagerChromeOS* const policy_manager =
      UserPolicyManagerFactoryChromeOS::GetCloudPolicyManagerForProfile(
          profile);
  DCHECK(!policy_manager->IsInitializationComplete(POLICY_DOMAIN_CHROME));

  // Give a bogus OAuth token to the |policy_manager|. This should make its
  // CloudPolicyClient fetch the DMToken.
  ASSERT_FALSE(policy_manager->core()->client()->is_registered());
  const enterprise_management::DeviceRegisterRequest::Type registration_type =
      enterprise_management::DeviceRegisterRequest::USER;
  policy_manager->core()->client()->Register(
      registration_type,
      enterprise_management::DeviceRegisterRequest::FLAVOR_USER_REGISTRATION,
      enterprise_management::DeviceRegisterRequest::LIFETIME_INDEFINITE,
      enterprise_management::LicenseType::UNDEFINED, "bogus", std::string(),
      std::string(), std::string());

  policy::ProfilePolicyConnector* const profile_connector =
      policy::ProfilePolicyConnectorFactory::GetForBrowserContext(profile);
  policy::PolicyService* const policy_service =
      profile_connector->policy_service();

  base::RunLoop run_loop;
  policy_service->RefreshPolicies(run_loop.QuitClosure());
  run_loop.Run();
}

void UserPolicyTestHelper::UpdatePolicy(
    const base::DictionaryValue& mandatory_policy,
    const base::DictionaryValue& recommended_policy,
    Profile* profile) {
  WritePolicyFile(mandatory_policy, recommended_policy);

  policy::ProfilePolicyConnector* const profile_connector =
      policy::ProfilePolicyConnectorFactory::GetForBrowserContext(profile);
  policy::PolicyService* const policy_service =
      profile_connector->policy_service();

  base::RunLoop run_loop;
  policy_service->RefreshPolicies(run_loop.QuitClosure());
  run_loop.Run();
}

void UserPolicyTestHelper::DeletePolicyFile() {
  base::DeleteFile(PolicyFilePath(), false);
}

void UserPolicyTestHelper::WritePolicyFile(
    const base::DictionaryValue& mandatory,
    const base::DictionaryValue& recommended) {
  const std::string policy = BuildPolicy(
      mandatory, recommended, dm_protocol::kChromeUserPolicyType, account_id_);
  const int bytes_written =
      base::WriteFile(PolicyFilePath(), policy.data(), policy.size());
  ASSERT_EQ(static_cast<int>(policy.size()), bytes_written);
}

base::FilePath UserPolicyTestHelper::PolicyFilePath() const {
  return temp_dir_.GetPath().AppendASCII("policy.json");
}

}  // namespace policy
