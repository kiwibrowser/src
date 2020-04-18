// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine/sync_string_conversions.h"
#include "components/sync/base/passphrase_type.h"

#define ENUM_CASE(x) \
  case x:            \
    return #x

namespace syncer {

const char* ConnectionStatusToString(ConnectionStatus status) {
  switch (status) {
    ENUM_CASE(CONNECTION_OK);
    ENUM_CASE(CONNECTION_AUTH_ERROR);
    ENUM_CASE(CONNECTION_SERVER_ERROR);
    default:
      NOTREACHED();
      return "INVALID_CONNECTION_STATUS";
  }
}

// Helper function that converts a PassphraseRequiredReason value to a string.
const char* PassphraseRequiredReasonToString(PassphraseRequiredReason reason) {
  switch (reason) {
    ENUM_CASE(REASON_PASSPHRASE_NOT_REQUIRED);
    ENUM_CASE(REASON_ENCRYPTION);
    ENUM_CASE(REASON_DECRYPTION);
    default:
      NOTREACHED();
      return "INVALID_REASON";
  }
}

const char* PassphraseTypeToString(PassphraseType type) {
  switch (type) {
    ENUM_CASE(PassphraseType::IMPLICIT_PASSPHRASE);
    ENUM_CASE(PassphraseType::KEYSTORE_PASSPHRASE);
    ENUM_CASE(PassphraseType::FROZEN_IMPLICIT_PASSPHRASE);
    ENUM_CASE(PassphraseType::CUSTOM_PASSPHRASE);
    default:
      NOTREACHED();
      return "INVALID_PASSPHRASE_TYPE";
  }
}

const char* BootstrapTokenTypeToString(BootstrapTokenType type) {
  switch (type) {
    ENUM_CASE(PASSPHRASE_BOOTSTRAP_TOKEN);
    ENUM_CASE(KEYSTORE_BOOTSTRAP_TOKEN);
    default:
      NOTREACHED();
      return "INVALID_BOOTSTRAP_TOKEN_TYPE";
  }
}

#undef ENUM_CASE

}  // namespace syncer
