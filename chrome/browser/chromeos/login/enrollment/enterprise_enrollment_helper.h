// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_ENROLLMENT_ENTERPRISE_ENROLLMENT_HELPER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_ENROLLMENT_ENTERPRISE_ENROLLMENT_HELPER_H_

#include <map>
#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/policy/device_cloud_policy_initializer.h"
#include "components/policy/core/common/cloud/cloud_policy_constants.h"

class GoogleServiceAuthError;

namespace policy {
struct EnrollmentConfig;
class EnrollmentStatus;
}  // namespace policy

namespace chromeos {

// Maps a license type to number of available licenses.
using EnrollmentLicenseMap = std::map<policy::LicenseType, int>;

class ActiveDirectoryJoinDelegate;

// This class is capable to enroll the device into enterprise domain, using
// either a profile containing authentication data or OAuth token.
// It can also clear an authentication data from the profile and revoke tokens
// that are not longer needed.
class EnterpriseEnrollmentHelper {
 public:
  using EnrollmentCallback =
      policy::DeviceCloudPolicyInitializer::EnrollmentCallback;

  // Enumeration of the possible errors that can occur during enrollment which
  // are not covered by GoogleServiceAuthError or EnrollmentStatus.
  enum OtherError {
    // Existing enrollment domain doesn't match authentication user.
    OTHER_ERROR_DOMAIN_MISMATCH,
    // Unexpected error condition, indicates a bug in the code.
    OTHER_ERROR_FATAL
  };

  class EnrollmentStatusConsumer {
   public:
    // Called when an error happens on attempt to receive authentication tokens.
    virtual void OnAuthError(const GoogleServiceAuthError& error) = 0;

    // Called when there are multiple license types available for enrollment,
    // and admin allowed user to choose license type to assign.
    // Enrollment is paused, and will resume once UseLicenseType() is called.
    virtual void OnMultipleLicensesAvailable(
        const EnrollmentLicenseMap& licenses) = 0;

    // Called when an error happens during enrollment.
    virtual void OnEnrollmentError(policy::EnrollmentStatus status) = 0;

    // Called when some other error happens.
    virtual void OnOtherError(OtherError error) = 0;

    // Called when enrollment finishes successfully. |additional_token| keeps
    // the additional access token, if it was requested by setting the
    // |fetch_additional_token| param of EnrollUsingProfile() to true.
    // Otherwise, |additional_token| is empty.
    virtual void OnDeviceEnrolled(const std::string& additional_token) = 0;

    // Called when device attribute update permission granted,
    // |granted| indicates whether permission granted or not.
    virtual void OnDeviceAttributeUpdatePermission(bool granted) = 0;

    // Called when device attribute upload finishes. |success| indicates
    // whether it is successful or not.
    virtual void OnDeviceAttributeUploadCompleted(bool success) = 0;
  };

  // Factory method. Caller takes ownership of the returned object.
  static std::unique_ptr<EnterpriseEnrollmentHelper> Create(
      EnrollmentStatusConsumer* status_consumer,
      ActiveDirectoryJoinDelegate* ad_join_delegate,
      const policy::EnrollmentConfig& enrollment_config,
      const std::string& enrolling_user_domain);

  using CreateMockEnrollmentHelper =
      EnterpriseEnrollmentHelper* (*)(EnrollmentStatusConsumer* status_consumer,
                                      const policy::EnrollmentConfig&
                                          enrollment_config,
                                      const std::string& enrolling_user_domain);

  // Use |creator| instead of the default enrollment helper allocator. This
  // allows tests to substitute in a mock enrollment helper. This function will
  // only be used once.
  static void SetupEnrollmentHelperMock(CreateMockEnrollmentHelper creator);

  virtual ~EnterpriseEnrollmentHelper();

  // Starts enterprise enrollment using |auth_code|. First tries to exchange the
  // auth code to authentication token, then tries to enroll the device with the
  // received token.
  // If |fetch_additional_token| is true, the helper fetches an additional token
  // and passes it to the |status_consumer| on successful enrollment.
  // EnrollUsingAuthCode can be called only once during this object's lifetime,
  // and only if none of the EnrollUsing* methods was called before.
  virtual void EnrollUsingAuthCode(const std::string& auth_code,
                                   bool fetch_additional_token) = 0;

  // Starts enterprise enrollment using |token|.
  // EnrollUsingToken can be called only once during this object's lifetime, and
  // only if none of the EnrollUsing* was called before.
  virtual void EnrollUsingToken(const std::string& token) = 0;

  // Starts enterprise enrollment using PCA attestation.
  // EnrollUsingAttestation can be called only once during the object's
  // lifetime, and only if none of the EnrollUsing* was called before.
  virtual void EnrollUsingAttestation() = 0;

  // Starts enterprise enrollment for offline demo-mode.
  // EnrollForOfflineDemo is used offline, no network connections. Thus it goes
  // into enrollment without authentication -- and applies policies which are
  // stored locally.
  virtual void EnrollForOfflineDemo() = 0;

  // Continue enrollment using license |type|.
  virtual void UseLicenseType(policy::LicenseType type) = 0;

  // Starts device attribute update process. First tries to get
  // permission to update device attributes for current user
  // using stored during enrollment oauth token.
  virtual void GetDeviceAttributeUpdatePermission() = 0;

  // Uploads device attributes on DM server. |asset_id| - Asset Identifier
  // and |location| - Assigned Location, these attributes were typed by
  // current user on the device attribute prompt screen after successful
  // enrollment.
  virtual void UpdateDeviceAttributes(const std::string& asset_id,
                                      const std::string& location) = 0;

  // Clears authentication data from the profile (if EnrollUsingProfile was
  // used) and revokes fetched tokens.
  // Does not revoke the additional token if enrollment finished successfully.
  // Calls |callback| on completion.
  virtual void ClearAuth(const base::Closure& callback) = 0;

 protected:
  // |status_consumer| must outlive |this|. Moreover, the user of this class
  // is responsible for clearing auth data in some cases (see comment for
  // EnrollUsingProfile()).
  explicit EnterpriseEnrollmentHelper(
      EnrollmentStatusConsumer* status_consumer);

  EnrollmentStatusConsumer* status_consumer() { return status_consumer_; }

 private:
  EnrollmentStatusConsumer* status_consumer_;

  // If this is not nullptr, then it will be used to create the enrollment
  // helper. |create_mock_enrollment_helper_| needs to outlive this class.
  static CreateMockEnrollmentHelper create_mock_enrollment_helper_;

  DISALLOW_COPY_AND_ASSIGN(EnterpriseEnrollmentHelper);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_ENROLLMENT_ENTERPRISE_ENROLLMENT_HELPER_H_
