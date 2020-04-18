// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/policy/arc_policy_util.h"

#include "base/command_line.h"
#include "chrome/browser/policy/profile_policy_connector.h"
#include "chrome/browser/policy/profile_policy_connector_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chromeos/chromeos_switches.h"

namespace arc {
namespace policy_util {

bool IsAccountManaged(const Profile* profile) {
  return policy::ProfilePolicyConnectorFactory::IsProfileManaged(profile);
}

bool IsArcDisabledForEnterprise() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      chromeos::switches::kEnterpriseDisableArc);
}

EcryptfsMigrationAction GetDefaultEcryptfsMigrationActionForManagedUser(
    bool active_directory_user) {
  // Active directory users are assumed to be enterprise users, so mimic the
  // server-side default logic for enterprise users, letting them choose if
  // they want to migrate by default.
  return active_directory_user ? EcryptfsMigrationAction::kAskUser
                               : EcryptfsMigrationAction::kDisallowMigration;
}

}  // namespace policy_util
}  // namespace arc
