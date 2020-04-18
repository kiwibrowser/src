// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/enterprise_reporting_private/enterprise_reporting_private_api.h"

#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/extensions/extension_api_unittest.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "components/policy/core/common/cloud/mock_cloud_policy_client.h"
#include "testing/gmock/include/gmock/gmock.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::WithArgs;

namespace extensions {
namespace {

const char kFakeDMToken[] = "fake-dm-token";
const char kFakeClientId[] = "fake-client-id";
const char kFakeMachineNameReport[] = "{\"computername\":\"name\"}";

class MockCloudPolicyClient : public policy::MockCloudPolicyClient {
 public:
  MockCloudPolicyClient() = default;

  void UploadChromeDesktopReport(
      std::unique_ptr<enterprise_management::ChromeDesktopReportRequest>
          request,
      const StatusCallback& callback) override {
    UploadChromeDesktopReportProxy(request.get(), callback);
  }
  MOCK_METHOD2(UploadChromeDesktopReportProxy,
               void(enterprise_management::ChromeDesktopReportRequest*,
                    const StatusCallback&));

  void OnReportUploadedFailed(const StatusCallback& callback) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(callback, false));
  }

  void OnReportUploadedSucceeded(const StatusCallback& callback) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(callback, true));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MockCloudPolicyClient);
};

}  // namespace

class EnterpriseReportingPrivateTest : public ExtensionApiUnittest {
 public:
  EnterpriseReportingPrivateTest() = default;

  UIThreadExtensionFunction* CreateChromeDesktopReportingFunction(
      const std::string& dm_token) {
    EnterpriseReportingPrivateUploadChromeDesktopReportFunction* function =
        new EnterpriseReportingPrivateUploadChromeDesktopReportFunction();
    std::unique_ptr<MockCloudPolicyClient> client =
        std::make_unique<MockCloudPolicyClient>();
    client_ = client.get();
    function->SetCloudPolicyClientForTesting(std::move(client));
    function->SetRegistrationInfoForTesting(dm_token, kFakeClientId);
    return function;
  }

  std::string GenerateArgs(const char* name) {
    return base::StringPrintf("[{\"machineName\":%s}]", name);
  }

  std::string GenerateInvalidReport() {
    // This report is invalid as the chromeSignInUser dictionary should not be
    // wrapped in a list.
    return std::string(
        "[{\"browserReport\": "
        "{\"chromeUserProfileReport\":[{\"chromeSignInUser\":\"Name\"}]}}]");
  }

  MockCloudPolicyClient* client_;

 private:
  DISALLOW_COPY_AND_ASSIGN(EnterpriseReportingPrivateTest);
};

TEST_F(EnterpriseReportingPrivateTest, DeviceIsNotEnrolled) {
  ASSERT_EQ(enterprise_reporting::kDeviceNotEnrolled,
            RunFunctionAndReturnError(
                CreateChromeDesktopReportingFunction(std::string()),
                GenerateArgs(kFakeMachineNameReport)));
}

TEST_F(EnterpriseReportingPrivateTest, ReportIsNotValid) {
  ASSERT_EQ(enterprise_reporting::kInvalidInputErrorMessage,
            RunFunctionAndReturnError(
                CreateChromeDesktopReportingFunction(kFakeDMToken),
                GenerateInvalidReport()));
}

TEST_F(EnterpriseReportingPrivateTest, UploadFailed) {
  UIThreadExtensionFunction* function =
      CreateChromeDesktopReportingFunction(kFakeDMToken);
  EXPECT_CALL(*client_, SetupRegistration(kFakeDMToken, kFakeClientId, _))
      .Times(1);
  EXPECT_CALL(*client_, UploadChromeDesktopReportProxy(_, _))
      .WillOnce(WithArgs<1>(
          Invoke(client_, &MockCloudPolicyClient::OnReportUploadedFailed)));
  ASSERT_EQ(enterprise_reporting::kUploadFailed,
            RunFunctionAndReturnError(function,
                                      GenerateArgs(kFakeMachineNameReport)));
  ::testing::Mock::VerifyAndClearExpectations(client_);
}

TEST_F(EnterpriseReportingPrivateTest, UploadSucceeded) {
  UIThreadExtensionFunction* function =
      CreateChromeDesktopReportingFunction(kFakeDMToken);
  EXPECT_CALL(*client_, SetupRegistration(kFakeDMToken, kFakeClientId, _))
      .Times(1);
  EXPECT_CALL(*client_, UploadChromeDesktopReportProxy(_, _))
      .WillOnce(WithArgs<1>(
          Invoke(client_, &MockCloudPolicyClient::OnReportUploadedSucceeded)));
  ASSERT_EQ(nullptr, RunFunctionAndReturnValue(
                         function, GenerateArgs(kFakeMachineNameReport)));
  ::testing::Mock::VerifyAndClearExpectations(client_);
}

}  // namespace extensions
