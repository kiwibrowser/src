// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/features/feature_session_type.h"

#include "base/logging.h"

namespace extensions {

namespace {

const FeatureSessionType kDefaultSessionType = FeatureSessionType::INITIAL;
FeatureSessionType g_current_session_type = kDefaultSessionType;

}  // namespace

FeatureSessionType GetCurrentFeatureSessionType() {
  return g_current_session_type;
}

void SetCurrentFeatureSessionType(FeatureSessionType session_type) {
  // Make sure that session type stays constant after it's been initialized.
  // Note that this requirement can be bypassed in tests by using
  // |ScopedCurrentFeatureSessionType|.
  CHECK(g_current_session_type == kDefaultSessionType ||
        session_type == g_current_session_type);
  // In certain unit tests, SetCurrentFeatureSessionType() can be called
  // within the same process (where e.g. utility processes run as a separate
  // thread). Don't write if the value is the same to avoid TSAN failures.
  if (session_type != g_current_session_type)
    g_current_session_type = session_type;
}

std::unique_ptr<base::AutoReset<FeatureSessionType>>
ScopedCurrentFeatureSessionType(FeatureSessionType type) {
  CHECK_EQ(g_current_session_type, kDefaultSessionType);
  return std::make_unique<base::AutoReset<FeatureSessionType>>(
      &g_current_session_type, type);
}

}  // namespace extensions
