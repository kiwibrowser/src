// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/enabled_state_provider.h"

namespace metrics {

bool EnabledStateProvider::IsReportingEnabled() const {
  return IsConsentGiven();
}

}  // namespace metrics
