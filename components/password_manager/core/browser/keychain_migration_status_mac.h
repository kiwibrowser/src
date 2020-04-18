// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_KEYCHAIN_MIGRATION_STATUS_MAC_H
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_KEYCHAIN_MIGRATION_STATUS_MAC_H

namespace password_manager {

// Status of password migration from the Keychain.
// The enum is used to back a histogram, and should therefore be treated as
// append-only.
enum class MigrationStatus {
  // Migration wasn't tried yet.
  NOT_STARTED = 0,

  // Migration finished successfully.
  MIGRATED,

  // Migration failed once. It should be tried again.
  FAILED_ONCE,

  // Migration failed twice. It should not be tried again.
  FAILED_TWICE,

  // Migration finished successfully. The Keychain was cleaned up.
  MIGRATED_DELETED,

  // Best effort migration happened. Some passwords were inaccessible.
  MIGRATED_PARTIALLY,

  // Gave up on migration as the passwords aren't accessible anyway after Chrome
  // changes the certificate.
  MIGRATION_STOPPED,

  MIGRATION_STATUS_COUNT,
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_KEYCHAIN_MIGRATION_STATUS_MAC_H
