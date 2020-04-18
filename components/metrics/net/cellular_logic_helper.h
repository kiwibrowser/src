// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_NET_CELLULAR_LOGIC_HELPER_H_
#define COMPONENTS_METRICS_NET_CELLULAR_LOGIC_HELPER_H_

#include "base/time/time.h"

namespace metrics {

// Returns UMA log upload interval based on OS and ongoing cellular experiment.
base::TimeDelta GetUploadInterval();

// Returns true if current connection type is cellular and user is assigned to
// experimental group for enabled cellular uploads.
bool IsCellularLogicEnabled();

}  // namespace metrics

#endif  // COMPONENTS_METRICS_NET_CELLULAR_LOGIC_HELPER_H_
