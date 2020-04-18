// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_IDENTITY_PUBLIC_CPP_ACCOUNT_STATE_MOJOM_TRAITS_H_
#define SERVICES_IDENTITY_PUBLIC_CPP_ACCOUNT_STATE_MOJOM_TRAITS_H_

#include "services/identity/public/cpp/account_state.h"
#include "services/identity/public/mojom/account_state.mojom.h"

namespace mojo {

template <>
struct StructTraits<identity::mojom::AccountState::DataView,
                    identity::AccountState> {
  static bool has_refresh_token(const identity::AccountState& r) {
    return r.has_refresh_token;
  }

  static bool is_primary_account(const identity::AccountState& r) {
    return r.is_primary_account;
  }

  static bool Read(identity::mojom::AccountState::DataView data,
                   identity::AccountState* out);
};

}  // namespace mojo

#endif  // SERVICES_IDENTITY_PUBLIC_CPP_ACCOUNT_STATE_MOJOM_TRAITS_H_
