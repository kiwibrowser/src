// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_BASE_PASSPHRASE_TYPE_H_
#define COMPONENTS_SYNC_BASE_PASSPHRASE_TYPE_H_

namespace syncer {

// The different states for the encryption passphrase. These control if and how
// the user should be prompted for a decryption passphrase.
// Do not re-order or delete these entries; they are used in a UMA histogram.
// Please edit SyncPassphraseType in histograms.xml if a value is added.
enum class PassphraseType {
  IMPLICIT_PASSPHRASE = 0,         // GAIA-based passphrase (deprecated).
  KEYSTORE_PASSPHRASE = 1,         // Keystore passphrase.
  FROZEN_IMPLICIT_PASSPHRASE = 2,  // Frozen GAIA passphrase.
  CUSTOM_PASSPHRASE = 3,           // User-provided passphrase.
  PASSPHRASE_TYPE_SIZE,            // The size of this enum; keep last.
};

bool IsExplicitPassphrase(PassphraseType type);

}  // namespace syncer

#endif  // COMPONENTS_SYNC_BASE_PASSPHRASE_TYPE_H_
