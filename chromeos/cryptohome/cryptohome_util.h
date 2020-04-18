// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_CRYPTOHOME_CRYPTOHOME_UTIL_H_
#define CHROMEOS_CRYPTOHOME_CRYPTOHOME_UTIL_H_

#include <string>

#include "base/optional.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/cryptohome/cryptohome_parameters.h"
#include "chromeos/dbus/cryptohome/key.pb.h"
#include "chromeos/dbus/cryptohome/rpc.pb.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace cryptohome {

// Returns a MountError code from the MountEx |reply| returning
// MOUNT_ERROR_NONE if the reply is well-formed and there is no error.
CHROMEOS_EXPORT MountError
MountExReplyToMountError(const base::Optional<BaseReply>& reply);

// Returns a MountError code from |reply|, returning MOUNT_ERROR_NONE
// if the reply is well-formed and there is no error.
CHROMEOS_EXPORT MountError
BaseReplyToMountError(const base::Optional<BaseReply>& reply);

// Returns a MountError code from the GetKeyDataEx |reply| returning
// MOUNT_ERROR_NONE if the reply is well-formed and there is no error.
CHROMEOS_EXPORT MountError
GetKeyDataReplyToMountError(const base::Optional<BaseReply>& reply);

CHROMEOS_EXPORT std::vector<KeyDefinition> GetKeyDataReplyToKeyDefinitions(
    const base::Optional<BaseReply>& reply);

// Extracts the account's disk usage size from |reply|.
// If |reply| is malformed, returns -1.
CHROMEOS_EXPORT
int64_t AccountDiskUsageReplyToUsageSize(
    const base::Optional<BaseReply>& reply);

// Extracts the mount hash from |reply|.
// This method assumes |reply| is well-formed. To check if a reply
// is well-formed, callers can check if BaseReplyToMountError returns
// MOUNT_ERROR_NONE.
CHROMEOS_EXPORT const std::string& MountExReplyToMountHash(
    const BaseReply& reply);

// Creates an AuthorizationRequest from the given secret and label.
CHROMEOS_EXPORT AuthorizationRequest
CreateAuthorizationRequest(const std::string& label, const std::string& secret);

// Converts the given KeyDefinition to a Key.
CHROMEOS_EXPORT void KeyDefinitionToKey(const KeyDefinition& key_def, Key* key);

// Converts CryptohomeErrorCode to MountError.
CHROMEOS_EXPORT MountError
CryptohomeErrorToMountError(CryptohomeErrorCode code);

// Converts the given KeyAuthorizationData to AuthorizationData pointed to by
// |authorization_data|.
CHROMEOS_EXPORT
void KeyAuthorizationDataToAuthorizationData(
    const KeyAuthorizationData& authorization_data_proto,
    KeyDefinition::AuthorizationData* authorization_data);

}  // namespace cryptohome

#endif  // CHROMEOS_CRYPTOHOME_CRYPTOHOME_UTIL_H_
