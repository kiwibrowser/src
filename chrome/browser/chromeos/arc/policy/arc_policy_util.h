// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_POLICY_ARC_POLICY_UTIL_H_
#define CHROME_BROWSER_CHROMEOS_ARC_POLICY_ARC_POLICY_UTIL_H_

#include <stddef.h>
#include <stdint.h>

class Profile;

namespace arc {
namespace policy_util {

// The action that should be taken when an ecryptfs user home which needs
// migration is detected. This must match the order/values of the
// EcryptfsMigrationStrategy policy.
enum class EcryptfsMigrationAction : int32_t {
  // Don't migrate.
  kDisallowMigration = 0,
  // Migrate without asking the user.
  kMigrate = 1,
  // Wipe the user home and start again.
  kWipe = 2,
  // Ask the user if migration should be performed.
  kAskUser = 3,
  // Minimal migration - similar to kWipe, but runs migration code with a small
  // whitelist of files to preserve authentication data.
  kMinimalMigrate = 4,
  // Special case for EDU default: Behaves like kAskUser if the device model
  // supported ARC on ecryptfs and ARC is enabled. Otherwise, behaves like
  // kDisallowMigration.
  kAskForEcryptfsArcUsers = 5,
};
constexpr size_t kEcryptfsMigrationActionMaxValue =
    static_cast<size_t>(EcryptfsMigrationAction::kAskForEcryptfsArcUsers);

// Returns true if the account is managed. Otherwise false.
bool IsAccountManaged(const Profile* profile);

// Returns true if ARC is disabled by --enterprise-diable-arc flag.
bool IsArcDisabledForEnterprise();

// Returns the default ecryptfs migration action for a managed user.
// |active_directory_user| specifies if the user authenticates with active
// directory. We have a separate default for active directory users, as these
// are assumed to be enterprise users.
EcryptfsMigrationAction GetDefaultEcryptfsMigrationActionForManagedUser(
    bool active_directory_user);

}  // namespace policy_util
}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_POLICY_ARC_POLICY_UTIL_H_
