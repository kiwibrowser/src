// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/controller_pairing_screen_view.h"

namespace chromeos {

namespace controller_pairing {

// Keep these constants synced with corresponding constants defined in
// oobe_screen_controller_pairing.js.
const char kContextKeyPage[] = "page";
const char kContextKeyControlsDisabled[] = "controlsDisabled";
const char kContextKeyDevices[] = "devices";
const char kContextKeyConfirmationCode[] = "code";
const char kContextKeySelectedDevice[] = "selectedDevice";
const char kContextKeyAccountId[] = "accountId";
const char kContextKeyEnrollmentDomain[] = "enrollmentDomain";

const char kPageDevicesDiscovery[] = "devices-discovery";
const char kPageDeviceSelect[] = "device-select";
const char kPageDeviceNotFound[] = "device-not-found";
const char kPageEstablishingConnection[] = "establishing-connection";
const char kPageEstablishingConnectionError[] = "establishing-connection-error";
const char kPageCodeConfirmation[] = "code-confirmation";
const char kPageHostNetworkError[] = "host-network-error";
const char kPageHostUpdate[] = "host-update";
const char kPageHostConnectionLost[] = "host-connection-lost";
const char kPageEnrollmentIntroduction[] = "enrollment-introduction";
const char kPageAuthentication[] = "authentication";
const char kPageHostEnrollment[] = "host-enrollment";
const char kPageHostEnrollmentError[] = "host-enrollment-error";
const char kPagePairingDone[] = "pairing-done";

const char kActionChooseDevice[] = "chooseDevice";
const char kActionRepeatDiscovery[] = "repeatDiscovery";
const char kActionAcceptCode[] = "acceptCode";
const char kActionRejectCode[] = "rejectCode";
const char kActionProceedToAuthentication[] = "proceedToAuthentication";
const char kActionEnroll[] = "enroll";
const char kActionStartSession[] = "startSession";

}  // namespace controller_pairing

ControllerPairingScreenView::ControllerPairingScreenView() {}

ControllerPairingScreenView::~ControllerPairingScreenView() {}

}  // namespace chromeos
