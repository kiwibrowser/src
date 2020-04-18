// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/identity/public/cpp/account_state_mojom_traits.h"

namespace mojo {

// static
bool StructTraits<
    identity::mojom::AccountState::DataView,
    identity::AccountState>::Read(identity::mojom::AccountState::DataView data,
                                  identity::AccountState* out) {
  out->has_refresh_token = data.has_refresh_token();
  out->is_primary_account = data.is_primary_account();

  return true;
}

}  // namespace mojo
