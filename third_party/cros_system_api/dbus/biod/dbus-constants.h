// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_BIOD_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_BIOD_DBUS_CONSTANTS_H_

namespace biod {
const char kBiodServicePath[] = "/org/chromium/BiometricsDaemon";
const char kBiodServiceName[] = "org.chromium.BiometricsDaemon";

// Interfaces for objects exported by biod
const char kBiometricsManagerInterface[] =
    "org.chromium.BiometricsDaemon.BiometricsManager";
const char kAuthSessionInterface[] =
    "org.chromium.BiometricsDaemon.AuthSession";
const char kEnrollSessionInterface[] =
    "org.chromium.BiometricsDaemon.EnrollSession";
const char kRecordInterface[] = "org.chromium.BiometricsDaemon.Record";

// List of all BiometricsManagers
const char kCrosFpBiometricsManagerName[] = "CrosFpBiometricsManager";
const char kFakeBiometricsManagerName[] = "FakeBiometricsManager";
const char kFpcBiometricsManagerName[] = "FpcBiometricsManager";

// Methods
const char kBiometricsManagerStartEnrollSessionMethod[] = "StartEnrollSession";
const char kBiometricsManagerGetRecordsForUserMethod[] = "GetRecordsForUser";
const char kBiometricsManagerDestroyAllRecordsMethod[] = "DestroyAllRecords";
const char kBiometricsManagerStartAuthSessionMethod[] = "StartAuthSession";
const char kAuthSessionEndMethod[] = "End";
const char kEnrollSessionCancelMethod[] = "Cancel";
const char kRecordRemoveMethod[] = "Remove";
const char kRecordSetLabelMethod[] = "SetLabel";

// Signals
const char kBiometricsManagerEnrollScanDoneSignal[] = "EnrollScanDone";
const char kBiometricsManagerAuthScanDoneSignal[] = "AuthScanDone";
const char kBiometricsManagerSessionFailedSignal[] = "SessionFailed";

// Properties
const char kBiometricsManagerBiometricTypeProperty[] = "Type";
const char kRecordLabelProperty[] = "Label";

// Values
enum BiometricType {
  BIOMETRIC_TYPE_UNKNOWN = 0,
  BIOMETRIC_TYPE_FINGERPRINT = 1,
  BIOMETRIC_TYPE_MAX,
};
}  // namespace biod

#endif  // SYSTEM_API_DBUS_BIOD_DBUS_CONSTANTS_H_
