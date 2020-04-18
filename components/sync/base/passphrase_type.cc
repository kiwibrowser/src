// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/base/passphrase_type.h"

namespace syncer {

bool IsExplicitPassphrase(PassphraseType type) {
  return type == PassphraseType::CUSTOM_PASSPHRASE ||
         type == PassphraseType::FROZEN_IMPLICIT_PASSPHRASE;
}

}  // namespace syncer
