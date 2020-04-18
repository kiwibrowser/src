// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/identity/public/cpp/account_state.h"

namespace identity {

AccountState::AccountState()
    : has_refresh_token(false), is_primary_account(false) {}

AccountState::AccountState(const AccountState& other) = default;
AccountState::~AccountState() {}

}  // namespace identity
