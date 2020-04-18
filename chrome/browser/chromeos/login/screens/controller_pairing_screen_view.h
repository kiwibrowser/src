// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_CONTROLLER_PAIRING_SCREEN_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_CONTROLLER_PAIRING_SCREEN_VIEW_H_

#include <string>

#include "base/macros.h"
#include "chrome/browser/chromeos/login/oobe_screen.h"

namespace base {
class DictionaryValue;
}

namespace content {
class BrowserContext;
}

namespace chromeos {

namespace controller_pairing {

// Keep these constants synced with corresponding constants defined in
// oobe_screen_controller_pairing.js.
// Context keys.
extern const char kContextKeyPage[];
extern const char kContextKeyControlsDisabled[];
extern const char kContextKeyDevices[];
extern const char kContextKeyConfirmationCode[];
extern const char kContextKeySelectedDevice[];
extern const char kContextKeyAccountId[];
extern const char kContextKeyEnrollmentDomain[];

// Pages names.
extern const char kPageDevicesDiscovery[];
extern const char kPageDeviceSelect[];
extern const char kPageDeviceNotFound[];
extern const char kPageEstablishingConnection[];
extern const char kPageEstablishingConnectionError[];
extern const char kPageCodeConfirmation[];
extern const char kPageHostNetworkError[];
extern const char kPageHostUpdate[];
extern const char kPageHostConnectionLost[];
extern const char kPageEnrollmentIntroduction[];
extern const char kPageAuthentication[];
extern const char kPageHostEnrollment[];
extern const char kPageHostEnrollmentError[];
extern const char kPagePairingDone[];

// Actions names.
extern const char kActionChooseDevice[];
extern const char kActionRepeatDiscovery[];
extern const char kActionAcceptCode[];
extern const char kActionRejectCode[];
extern const char kActionProceedToAuthentication[];
extern const char kActionEnroll[];
extern const char kActionStartSession[];

}  // namespace controller_pairing

class ControllerPairingScreenView {
 public:
  class Delegate {
   public:
    virtual ~Delegate() {}
    virtual void OnViewDestroyed(ControllerPairingScreenView* view) = 0;
    virtual void OnScreenContextChanged(const base::DictionaryValue& diff) = 0;
    virtual void OnUserActed(const std::string& action) = 0;
  };

  constexpr static OobeScreen kScreenId =
      OobeScreen::SCREEN_OOBE_CONTROLLER_PAIRING;

  ControllerPairingScreenView();
  virtual ~ControllerPairingScreenView();

  virtual void Show() = 0;
  virtual void Hide() = 0;
  virtual void SetDelegate(Delegate* delegate) = 0;
  virtual void OnContextChanged(const base::DictionaryValue& diff) = 0;
  virtual content::BrowserContext* GetBrowserContext() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(ControllerPairingScreenView);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_CONTROLLER_PAIRING_SCREEN_VIEW_H_
