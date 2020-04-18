// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/pref_names.h"

namespace cryptauth {
namespace prefs {

// Whether the system is scheduling device_syncs more aggressively to recover
// from the previous device_sync failure.
const char kCryptAuthDeviceSyncIsRecoveringFromFailure[] =
    "cryptauth.device_sync.is_recovering_from_failure";

// The timestamp of the last successful CryptAuth device_sync in seconds.
const char kCryptAuthDeviceSyncLastSyncTimeSeconds[] =
    "cryptauth.device_sync.last_device_sync_time_seconds";

// The reason that the next device_sync is performed. This should be one of the
// enum values of InvocationReason in
// components/cryptauth/proto/cryptauth_api.proto.
const char kCryptAuthDeviceSyncReason[] = "cryptauth.device_sync.reason";

// A list of unlock keys (stored as dictionaries) synced from CryptAuth. Unlock
// Keys are phones belonging to the user that can unlock other devices, such as
// desktop PCs.
const char kCryptAuthDeviceSyncUnlockKeys[] =
    "cryptauth.device_sync.unlock_keys";

// Whether the system is scheduling enrollments more aggressively to recover
// from the previous enrollment failure.
const char kCryptAuthEnrollmentIsRecoveringFromFailure[] =
    "cryptauth.enrollment.is_recovering_from_failure";

// The timestamp of the last successful CryptAuth enrollment in seconds.
const char kCryptAuthEnrollmentLastEnrollmentTimeSeconds[] =
    "cryptauth.enrollment.last_enrollment_time_seconds";

// The reason that the next enrollment is performed. This should be one of the
// enum values of InvocationReason in
// components/cryptauth/proto/cryptauth_api.proto.
const char kCryptAuthEnrollmentReason[] = "cryptauth.enrollment.reason";

// The public key of the user and device enrolled with CryptAuth.
const char kCryptAuthEnrollmentUserPublicKey[] =
    "cryptauth.enrollment.user_public_key";

// The private key of the user and device enrolled with CryptAuth.
const char kCryptAuthEnrollmentUserPrivateKey[] =
    "cryptauth.enrollment.user_private_key";

// The GCM registration id used for receiving push messages from CryptAuth.
const char kCryptAuthGCMRegistrationId[] = "cryptauth.gcm_registration_id";

}  // namespace prefs
}  // namespace cryptauth
