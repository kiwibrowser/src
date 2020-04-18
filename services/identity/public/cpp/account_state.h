// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_IDENTITY_PUBLIC_CPP_ACCOUNT_STATE_H_
#define SERVICES_IDENTITY_PUBLIC_CPP_ACCOUNT_STATE_H_

namespace identity {

// C++ typemap of the Mojo struct giving information about the state of a
// specific account. See account_state.mojom for documentation.
struct AccountState {
  AccountState();
  AccountState(const AccountState& other);
  ~AccountState();

  bool has_refresh_token;
  bool is_primary_account;
};

}  // namespace identity

#endif  // SERVICES_IDENTITY_PUBLIC_CPP_ACCOUNT_STATE_H_
