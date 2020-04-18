// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sessions/content/content_record_password_state.h"

#include "content/public/browser/navigation_entry.h"

namespace sessions {

namespace {
// The key used to store PasswordState in the NavigationEntry;
// We stash an enum value in the first character of the string16 that is
// associated with this key.
const char kPasswordStateKey[] = "sessions_password_state";
}

SerializedNavigationEntry::PasswordState GetPasswordStateFromNavigation(
    const content::NavigationEntry& entry) {
  base::string16 password_state_str;
  if (!entry.GetExtraData(kPasswordStateKey, &password_state_str) ||
      password_state_str.size() != 1) {
    return SerializedNavigationEntry::PASSWORD_STATE_UNKNOWN;
  }

  SerializedNavigationEntry::PasswordState state =
      static_cast<SerializedNavigationEntry::PasswordState>(
          password_state_str[0]);

  DCHECK_GE(state, SerializedNavigationEntry::PASSWORD_STATE_UNKNOWN);
  DCHECK_LE(state, SerializedNavigationEntry::HAS_PASSWORD_FIELD);
  return state;
}

void SetPasswordStateInNavigation(
    SerializedNavigationEntry::PasswordState state,
    content::NavigationEntry* entry) {
  base::string16 password_state_str(1, static_cast<uint16_t>(state));
  entry->SetExtraData(kPasswordStateKey, password_state_str);
}

}  // namespace sessions
