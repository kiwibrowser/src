// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_FEATURES_FEATURE_SESSION_TYPE_H_
#define EXTENSIONS_COMMON_FEATURES_FEATURE_SESSION_TYPE_H_

#include <memory>

#include "base/auto_reset.h"

namespace extensions {

// Classes of session types to which features can be restricted in feature
// files. The session type describes session based on the type of user that is
// active in the current session.
enum class FeatureSessionType {
  // Initial session state - before a user logs in.
  INITIAL = 0,
  // Represents a session type that cannot be used with feature's session types
  // property.
  UNKNOWN = 1,
  // Regular user session.
  REGULAR = 2,
  // Kiosk app session.
  KIOSK = 3,
  // Kiosk app session that's been auto-launched from login screen (without
  // any user interaction).
  AUTOLAUNCHED_KIOSK = 4,
  // Helper for determining max enum value - not used as a real type.
  LAST = AUTOLAUNCHED_KIOSK
};

// Gets the current session type as seen by the Feature system.
FeatureSessionType GetCurrentFeatureSessionType();

// Sets the current session type as seen by the Feature system. In the browser
// process this should be extensions::util::GetCurrentSessionType(), and in
// the renderer this will need to come from an IPC.
void SetCurrentFeatureSessionType(FeatureSessionType session_type);

// Scoped session type setter. Use for tests.
std::unique_ptr<base::AutoReset<FeatureSessionType>>
ScopedCurrentFeatureSessionType(FeatureSessionType session_type);

}  // namespace extensions

#endif  // EXTENSIONS_COMMON_FEATURES_FEATURE_SESSION_TYPE_H_
