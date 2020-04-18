// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/fake_auth_policy_client.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_cryptohome_client.h"
#include "chromeos/dbus/fake_session_manager_client.h"
#include "chromeos/login/auth/authpolicy_login_helper.h"
#include "components/account_id/account_id.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace em = enterprise_management;

namespace chromeos {
namespace {

constexpr char kCorrectMachineName[] = "machine_name";
constexpr char kCorrectUserName[] = "user@domain.com";
constexpr char kCorrectUserDomain[] = "domain.com";
constexpr char kAccountId[] = "user-account-id";
constexpr char kMachineDomain[] = "machine.domain";

}  // namespace

class FakeAuthPolicyClientTest : public ::testing::Test {
 public:
  FakeAuthPolicyClientTest() = default;

 protected:
  FakeAuthPolicyClient* authpolicy_client() { return auth_policy_client_ptr_; }
  FakeSessionManagerClient* session_manager_client() {
    return session_manager_client_ptr_;
  }

  void SetUp() override {
    ::testing::Test::SetUp();
    auto session_manager_client = std::make_unique<FakeSessionManagerClient>();
    session_manager_client_ptr_ = session_manager_client.get();
    DBusThreadManager::GetSetterForTesting()->SetSessionManagerClient(
        std::move(session_manager_client));
    DBusThreadManager::GetSetterForTesting()->SetCryptohomeClient(
        std::make_unique<FakeCryptohomeClient>());
    auto auth_policy_client = std::make_unique<FakeAuthPolicyClient>();
    auth_policy_client_ptr_ = auth_policy_client.get();
    DBusThreadManager::GetSetterForTesting()->SetAuthPolicyClient(
        std::move(auth_policy_client));
    authpolicy_client()->DisableOperationDelayForTesting();
  }

  void JoinAdDomain(const std::string& machine_name,
                    const std::string& username,
                    AuthPolicyClient::JoinCallback callback) {
    authpolicy::JoinDomainRequest request;
    request.set_machine_name(machine_name);
    request.set_user_principal_name(username);
    authpolicy_client()->JoinAdDomain(request, /* password_fd */ -1,
                                      std::move(callback));
  }

  void JoinAdDomainWithMachineDomain(const std::string& machine_name,
                                     const std::string& machine_domain,
                                     const std::string& username,
                                     AuthPolicyClient::JoinCallback callback) {
    authpolicy::JoinDomainRequest request;
    request.set_machine_name(machine_name);
    request.set_user_principal_name(username);
    request.set_machine_domain(machine_domain);
    authpolicy_client()->JoinAdDomain(request, /* password_fd */ -1,
                                      std::move(callback));
  }

  void AuthenticateUser(const std::string& username,
                        const std::string& account_id,
                        AuthPolicyClient::AuthCallback callback) {
    authpolicy::AuthenticateUserRequest request;
    request.set_user_principal_name(username);
    request.set_account_id(account_id);
    authpolicy_client()->AuthenticateUser(request, /* password_fd */ -1,
                                          std::move(callback));
  }

  void LockDeviceActiveDirectory() {
    EXPECT_TRUE(AuthPolicyLoginHelper::LockDeviceActiveDirectoryForTesting(
        std::string()));
  }

 private:
  FakeAuthPolicyClient* auth_policy_client_ptr_;  // not owned.
  FakeSessionManagerClient* session_manager_client_ptr_;  // not owned.
  base::MessageLoop loop_;

  DISALLOW_COPY_AND_ASSIGN(FakeAuthPolicyClientTest);
};

// Tests parsing machine name.
TEST_F(FakeAuthPolicyClientTest, JoinAdDomain_ParseMachineName) {
  authpolicy_client()->set_started(true);
  JoinAdDomain("correct_length1", kCorrectUserName,
               base::BindOnce(
                   [](authpolicy::ErrorType error, const std::string& domain) {
                     EXPECT_EQ(authpolicy::ERROR_NONE, error);
                     EXPECT_EQ(kCorrectUserDomain, domain);
                   }));
  JoinAdDomain("", kCorrectUserName,
               base::BindOnce(
                   [](authpolicy::ErrorType error, const std::string& domain) {
                     EXPECT_EQ(authpolicy::ERROR_INVALID_MACHINE_NAME, error);
                     EXPECT_TRUE(domain.empty());
                   }));
  JoinAdDomain("too_long_machine_name", kCorrectUserName,
               base::BindOnce(
                   [](authpolicy::ErrorType error, const std::string& domain) {
                     EXPECT_EQ(authpolicy::ERROR_MACHINE_NAME_TOO_LONG, error);
                     EXPECT_TRUE(domain.empty());
                   }));
  JoinAdDomain("invalid:name", kCorrectUserName,
               base::BindOnce(
                   [](authpolicy::ErrorType error, const std::string& domain) {
                     EXPECT_EQ(authpolicy::ERROR_INVALID_MACHINE_NAME, error);
                     EXPECT_TRUE(domain.empty());
                   }));
  base::RunLoop loop;
  JoinAdDomain(">nvalidname", kCorrectUserName,
               base::BindOnce(
                   [](base::OnceClosure closure, authpolicy::ErrorType error,
                      const std::string& domain) {
                     EXPECT_EQ(authpolicy::ERROR_INVALID_MACHINE_NAME, error);
                     EXPECT_TRUE(domain.empty());
                     std::move(closure).Run();
                   },
                   loop.QuitClosure()));
  loop.Run();
}

// Tests join to a different machine domain.
TEST_F(FakeAuthPolicyClientTest, JoinAdDomain_MachineDomain) {
  authpolicy_client()->set_started(true);
  JoinAdDomainWithMachineDomain(kCorrectMachineName, kMachineDomain,
                                kCorrectUserName,
                                base::BindOnce([](authpolicy::ErrorType error,
                                                  const std::string& domain) {
                                  EXPECT_EQ(authpolicy::ERROR_NONE, error);
                                  EXPECT_EQ(kMachineDomain, domain);
                                }));
  base::RunLoop loop;
  JoinAdDomainWithMachineDomain(
      kCorrectMachineName, "", kCorrectUserName,
      base::BindOnce(
          [](base::OnceClosure closure, authpolicy::ErrorType error,
             const std::string& domain) {
            EXPECT_EQ(authpolicy::ERROR_NONE, error);
            EXPECT_EQ(kCorrectUserDomain, domain);
            std::move(closure).Run();
          },
          loop.QuitClosure()));
  loop.Run();
}

// Tests parsing user name.
TEST_F(FakeAuthPolicyClientTest, JoinAdDomain_ParseUPN) {
  authpolicy_client()->set_started(true);
  JoinAdDomain(kCorrectMachineName, kCorrectUserName,
               base::BindOnce(
                   [](authpolicy::ErrorType error, const std::string& domain) {
                     EXPECT_EQ(authpolicy::ERROR_NONE, error);
                     EXPECT_EQ(kCorrectUserDomain, domain);
                   }));
  JoinAdDomain(kCorrectMachineName, "user",
               base::BindOnce(
                   [](authpolicy::ErrorType error, const std::string& domain) {
                     EXPECT_EQ(authpolicy::ERROR_PARSE_UPN_FAILED, error);
                     EXPECT_TRUE(domain.empty());
                   }));
  JoinAdDomain(kCorrectMachineName, "",
               base::BindOnce(
                   [](authpolicy::ErrorType error, const std::string& domain) {
                     EXPECT_EQ(authpolicy::ERROR_PARSE_UPN_FAILED, error);
                     EXPECT_TRUE(domain.empty());
                   }));
  JoinAdDomain(kCorrectMachineName, "user@",
               base::BindOnce(
                   [](authpolicy::ErrorType error, const std::string& domain) {
                     EXPECT_EQ(authpolicy::ERROR_PARSE_UPN_FAILED, error);
                     EXPECT_TRUE(domain.empty());
                   }));
  JoinAdDomain(kCorrectMachineName, "@realm",
               base::BindOnce(
                   [](authpolicy::ErrorType error, const std::string& domain) {
                     EXPECT_EQ(authpolicy::ERROR_PARSE_UPN_FAILED, error);
                     EXPECT_TRUE(domain.empty());
                   }));
  base::RunLoop loop;
  JoinAdDomain(kCorrectMachineName, "user@realm@com",
               base::BindOnce(
                   [](base::OnceClosure closure, authpolicy::ErrorType error,
                      const std::string& domain) {
                     EXPECT_EQ(authpolicy::ERROR_PARSE_UPN_FAILED, error);
                     EXPECT_TRUE(domain.empty());
                     std::move(closure).Run();
                   },
                   loop.QuitClosure()));
  loop.Run();
}

// Tests that fake server does not support legacy encryption types.
TEST_F(FakeAuthPolicyClientTest, JoinAdDomain_NotSupportedEncType) {
  authpolicy_client()->set_started(true);
  base::RunLoop loop;
  authpolicy::JoinDomainRequest request;
  request.set_machine_name(kCorrectMachineName);
  request.set_user_principal_name(kCorrectUserName);
  request.set_kerberos_encryption_types(
      authpolicy::KerberosEncryptionTypes::ENC_TYPES_LEGACY);
  authpolicy_client()->JoinAdDomain(
      request, /* password_fd */ -1,
      base::BindOnce(
          [](base::OnceClosure closure, authpolicy::ErrorType error,
             const std::string& domain) {
            EXPECT_EQ(authpolicy::ERROR_KDC_DOES_NOT_SUPPORT_ENCRYPTION_TYPE,
                      error);
            EXPECT_TRUE(domain.empty());
            std::move(closure).Run();
          },
          loop.QuitClosure()));
  loop.Run();
}

// Test AuthenticateUser.
TEST_F(FakeAuthPolicyClientTest, AuthenticateUser_ByAccountId) {
  authpolicy_client()->set_started(true);
  LockDeviceActiveDirectory();
  // Check that account_id do not change.
  AuthenticateUser(
      kCorrectUserName, kAccountId,
      base::BindOnce(
          [](authpolicy::ErrorType error,
             const authpolicy::ActiveDirectoryAccountInfo& account_info) {
            EXPECT_EQ(authpolicy::ERROR_NONE, error);
            EXPECT_EQ(kAccountId, account_info.account_id());
          }));
}

// Tests calls to not started authpolicyd fails.
TEST_F(FakeAuthPolicyClientTest, NotStartedAuthPolicyService) {
  JoinAdDomain(kCorrectMachineName, kCorrectUserName,
               base::BindOnce(
                   [](authpolicy::ErrorType error, const std::string& domain) {
                     EXPECT_EQ(authpolicy::ERROR_DBUS_FAILURE, error);
                     EXPECT_TRUE(domain.empty());
                   }));
  LockDeviceActiveDirectory();
  AuthenticateUser(
      kCorrectUserName, std::string() /* account_id */,
      base::BindOnce([](authpolicy::ErrorType error,
                        const authpolicy::ActiveDirectoryAccountInfo&) {
        EXPECT_EQ(authpolicy::ERROR_DBUS_FAILURE, error);
      }));
  authpolicy_client()->RefreshDevicePolicy(
      base::BindOnce([](authpolicy::ErrorType error) {
        EXPECT_EQ(authpolicy::ERROR_DBUS_FAILURE, error);
      }));
  base::RunLoop loop;
  authpolicy_client()->RefreshUserPolicy(
      AccountId::FromUserEmail(kCorrectUserName),
      base::BindOnce(
          [](base::OnceClosure closure, authpolicy::ErrorType error) {
            EXPECT_EQ(authpolicy::ERROR_DBUS_FAILURE, error);
            std::move(closure).Run();
          },
          loop.QuitClosure()));
  loop.Run();
}

// Tests RefreshDevicePolicy. On a not locked device it should cache policy. On
// a locked device it should send policy to session_manager.
TEST_F(FakeAuthPolicyClientTest, NotLockedDeviceCachesPolicy) {
  authpolicy_client()->set_started(true);
  authpolicy_client()->RefreshDevicePolicy(
      base::BindOnce([](authpolicy::ErrorType error) {
        EXPECT_EQ(authpolicy::ERROR_DEVICE_POLICY_CACHED_BUT_NOT_SENT, error);
      }));
  LockDeviceActiveDirectory();
  base::RunLoop loop;
  authpolicy_client()->RefreshDevicePolicy(base::BindOnce(
      [](base::OnceClosure closure, authpolicy::ErrorType error) {
        EXPECT_EQ(authpolicy::ERROR_NONE, error);
        std::move(closure).Run();
      },
      loop.QuitClosure()));
  loop.Run();
}

// Tests that RefreshDevicePolicy stores device policy in the session manager.
TEST_F(FakeAuthPolicyClientTest, RefreshDevicePolicyStoresPolicy) {
  authpolicy_client()->set_started(true);
  LockDeviceActiveDirectory();

  {
    // Call RefreshDevicePolicy.
    base::RunLoop loop;
    em::ChromeDeviceSettingsProto policy;
    policy.mutable_allow_new_users()->set_allow_new_users(true);
    authpolicy_client()->set_device_policy(policy);
    authpolicy_client()->RefreshDevicePolicy(base::BindOnce(
        [](base::OnceClosure closure, authpolicy::ErrorType error) {
          EXPECT_EQ(authpolicy::ERROR_NONE, error);
          std::move(closure).Run();
        },
        loop.QuitClosure()));
    loop.Run();
  }

  {
    // Retrieve device policy from the session manager.
    std::string response_blob;
    EXPECT_EQ(
        SessionManagerClient::RetrievePolicyResponseType::SUCCESS,
        session_manager_client()->BlockingRetrieveDevicePolicy(&response_blob));
    em::PolicyFetchResponse response;
    EXPECT_TRUE(response.ParseFromString(response_blob));
    EXPECT_TRUE(response.has_policy_data());

    em::PolicyData policy_data;
    EXPECT_TRUE(policy_data.ParseFromString(response.policy_data()));

    em::ChromeDeviceSettingsProto policy;
    EXPECT_TRUE(policy.ParseFromString(policy_data.policy_value()));
    EXPECT_TRUE(policy.has_allow_new_users());
    EXPECT_TRUE(policy.allow_new_users().allow_new_users());
  }
}

}  // namespace chromeos
