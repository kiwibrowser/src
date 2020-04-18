// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SECURITY_STATE_SECURITY_STATE_UI_H_
#define COMPONENTS_SECURITY_STATE_SECURITY_STATE_UI_H_

#include "components/security_state/core/security_state.h"

// Provides helper methods that encapsulate platform-independent security UI
// logic.
namespace security_state {

// On some (mobile) form factors, the security indicator icon is hidden to save
// UI space. This returns whether the icon should always be shown for the given
// |security_level|, i.e. whether to override the hiding behaviour.
bool ShouldAlwaysShowIcon(SecurityLevel security_level);

}  // namespace security_state

#endif  // COMPONENTS_SECURITY_STATE_SECURITY_STATE_UI_H_
