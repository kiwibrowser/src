// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_HOST_PAIRING_SCREEN_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_HOST_PAIRING_SCREEN_VIEW_H_

#include "base/macros.h"
#include "chrome/browser/chromeos/login/enrollment/enterprise_enrollment_helper.h"
#include "chrome/browser/chromeos/login/oobe_screen.h"

namespace base {
class DictionaryValue;
}

class GoogleServiceAuthError;

namespace policy {
class EnrollmentStatus;
}

namespace chromeos {

namespace host_pairing {

// Keep these constants synced with corresponding constants defined in
// oobe_screen_host_pairing.js.
// Conxtext keys.
extern const char kContextKeyPage[];
extern const char kContextKeyDeviceName[];
extern const char kContextKeyConfirmationCode[];
extern const char kContextKeyEnrollmentDomain[];
extern const char kContextKeyUpdateProgress[];
extern const char kContextKeyEnrollmentError[];

// Pages names.
extern const char kPageWelcome[];
extern const char kPageIntializationError[];
extern const char kPageCodeConfirmation[];
extern const char kPageConnectionError[];
extern const char kPageSetupBasicConfiguration[];
extern const char kPageSetupNetworkError[];
extern const char kPageUpdate[];
extern const char kPageEnrollmentIntroduction[];
extern const char kPageEnrollment[];
extern const char kPageEnrollmentError[];
extern const char kPagePairingDone[];

}  // namespace host_pairing

class HostPairingScreenView {
 public:
  class Delegate {
   public:
    virtual ~Delegate() {}
    virtual void OnViewDestroyed(HostPairingScreenView* view) = 0;
  };

  constexpr static OobeScreen kScreenId = OobeScreen::SCREEN_OOBE_HOST_PAIRING;

  HostPairingScreenView();
  virtual ~HostPairingScreenView();

  virtual void Show() = 0;
  virtual void Hide() = 0;
  virtual void SetDelegate(Delegate* delegate) = 0;
  virtual void OnContextChanged(const base::DictionaryValue& diff) = 0;

  virtual std::string GetErrorStringFromAuthError(
      const GoogleServiceAuthError& error) = 0;
  virtual std::string GetErrorStringFromEnrollmentError(
      policy::EnrollmentStatus status) = 0;
  virtual std::string GetErrorStringFromOtherError(
      EnterpriseEnrollmentHelper::OtherError error) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(HostPairingScreenView);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_HOST_PAIRING_SCREEN_VIEW_H_
