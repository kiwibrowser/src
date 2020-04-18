// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/security_state/core/security_state_ui.h"

namespace security_state {

bool ShouldAlwaysShowIcon(SecurityLevel security_level) {
  // Enumerate all |SecurityLevel| values for compile-time enforcement that all
  // cases are explicitly handled.
  switch (security_level) {
    case NONE:
      return false;
    case HTTP_SHOW_WARNING:
    case EV_SECURE:
    case SECURE:
    case SECURE_WITH_POLICY_INSTALLED_CERT:
    case DANGEROUS:
      return true;
    case SECURITY_LEVEL_COUNT:
      NOTREACHED();
      return false;
  }
  NOTREACHED();
  return false;
}

}  // namespace security_state
