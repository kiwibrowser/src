// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_ENTERPRISE_REPORTING_PRIVATE_ENTERPRISE_REPORTING_PRIVATE_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_ENTERPRISE_REPORTING_PRIVATE_ENTERPRISE_REPORTING_PRIVATE_API_H_

#include "extensions/browser/extension_function.h"

namespace policy {
class CloudPolicyClient;
}

namespace extensions {
namespace enterprise_reporting {

extern const char kInvalidInputErrorMessage[];
extern const char kUploadFailed[];
extern const char kDeviceNotEnrolled[];

}  // namespace enterprise_reporting

class EnterpriseReportingPrivateUploadChromeDesktopReportFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION(
      "enterprise.reportingPrivate.uploadChromeDesktopReport",
      ENTERPRISEREPORTINGPRIVATE_UPLOADCHROMEDESKTOPREPORT);
  EnterpriseReportingPrivateUploadChromeDesktopReportFunction();

  // ExtensionFunction
  ExtensionFunction::ResponseAction Run() override;

  void SetCloudPolicyClientForTesting(
      std::unique_ptr<policy::CloudPolicyClient> client);
  void SetRegistrationInfoForTesting(const std::string& dm_token,
                                     const std::string& client_id);

 private:
  ~EnterpriseReportingPrivateUploadChromeDesktopReportFunction() override;

  // ExtensionFunction
  void OnResponded() override;

  // Callback once Chrome get the response from the DM Server.
  void OnReportUploaded(bool status);

  std::unique_ptr<policy::CloudPolicyClient> cloud_policy_client_;
  std::string dm_token_;
  std::string client_id_;

  DISALLOW_COPY_AND_ASSIGN(
      EnterpriseReportingPrivateUploadChromeDesktopReportFunction);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_ENTERPRISE_REPORTING_PRIVATE_ENTERPRISE_REPORTING_PRIVATE_API_H_
