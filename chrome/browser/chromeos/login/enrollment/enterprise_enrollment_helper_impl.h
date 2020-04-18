// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_ENROLLMENT_ENTERPRISE_ENROLLMENT_HELPER_IMPL_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_ENROLLMENT_ENTERPRISE_ENROLLMENT_HELPER_IMPL_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/login/enrollment/enterprise_enrollment_helper.h"
#include "chrome/browser/chromeos/policy/enrollment_config.h"
#include "components/policy/core/common/cloud/enterprise_metrics.h"
#include "google_apis/gaia/google_service_auth_error.h"

namespace policy {
class PolicyOAuth2TokenFetcher;
}

namespace chromeos {

class EnterpriseEnrollmentHelperImpl : public EnterpriseEnrollmentHelper {
 public:
  EnterpriseEnrollmentHelperImpl(
      EnrollmentStatusConsumer* status_consumer,
      ActiveDirectoryJoinDelegate* ad_join_delegate,
      const policy::EnrollmentConfig& enrollment_config,
      const std::string& enrolling_user_domain);
  ~EnterpriseEnrollmentHelperImpl() override;

  // Overridden from EnterpriseEnrollmentHelper:
  void EnrollUsingAuthCode(const std::string& auth_code,
                           bool fetch_additional_token) override;
  void EnrollUsingToken(const std::string& token) override;
  void EnrollUsingAttestation() override;
  void EnrollForOfflineDemo() override;
  void ClearAuth(const base::Closure& callback) override;
  void UseLicenseType(policy::LicenseType type) override;
  void GetDeviceAttributeUpdatePermission() override;
  void UpdateDeviceAttributes(const std::string& asset_id,
                              const std::string& location) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(EnterpriseEnrollmentTest,
                           TestProperPageGetsLoadedOnEnrollmentSuccess);
  FRIEND_TEST_ALL_PREFIXES(EnterpriseEnrollmentTest,
                           TestAttributePromptPageGetsLoaded);

  // Checks if license type selection should be performed during enrollment.
  bool ShouldCheckLicenseType() const;

  void DoEnroll(const std::string& token);

  // Handles completion of the OAuth2 token fetch attempt.
  void OnTokenFetched(bool is_additional_token,
                      const std::string& token,
                      const GoogleServiceAuthError& error);

  // Handles multiple license types case.
  void OnLicenseMapObtained(const EnrollmentLicenseMap& licenses);

  // Handles completion of the enrollment attempt.
  void OnEnrollmentFinished(policy::EnrollmentStatus status);

  // Handles completion of the device attribute update permission request.
  void OnDeviceAttributeUpdatePermission(bool granted);

  // Handles completion of the device attribute update attempt.
  void OnDeviceAttributeUploadCompleted(bool success);

  void ReportAuthStatus(const GoogleServiceAuthError& error);
  void ReportEnrollmentStatus(policy::EnrollmentStatus status);

  // Logs an UMA event in the kMetricEnrollment or the kMetricEnrollmentRecovery
  // histogram, depending on |enrollment_mode_|.
  void UMA(policy::MetricEnrollment sample);

  // Called by ProfileHelper when a signin profile clearance has finished.
  // |callback| is a callback, that was passed to ClearAuth() before.
  void OnSigninProfileCleared(const base::Closure& callback);

  const policy::EnrollmentConfig enrollment_config_;
  const std::string enrolling_user_domain_;
  bool fetch_additional_token_;

  std::string additional_token_;
  enum {
    OAUTH_NOT_STARTED,
    OAUTH_STARTED_WITH_AUTH_CODE,
    OAUTH_STARTED_WITH_TOKEN,
    OAUTH_FINISHED
  } oauth_status_ = OAUTH_NOT_STARTED;
  bool oauth_data_cleared_ = false;
  std::string oauth_token_;
  bool success_ = false;
  ActiveDirectoryJoinDelegate* ad_join_delegate_ = nullptr;

  std::unique_ptr<policy::PolicyOAuth2TokenFetcher> oauth_fetcher_;

  base::WeakPtrFactory<EnterpriseEnrollmentHelperImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(EnterpriseEnrollmentHelperImpl);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_ENROLLMENT_ENTERPRISE_ENROLLMENT_HELPER_IMPL_H_
