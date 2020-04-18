// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_METRICS_PUBLIC_CPP_UKM_SOURCE_ID_H_
#define SERVICES_METRICS_PUBLIC_CPP_UKM_SOURCE_ID_H_

#include <stdint.h>

#include "services/metrics/public/cpp/metrics_export.h"

namespace ukm {

typedef int64_t SourceId;

enum class SourceIdType : int64_t {
  UKM = 0,
  NAVIGATION_ID = 1,
};

const SourceId kInvalidSourceId = 0;

// Get a new source ID, which is unique for the duration of a browser session.
METRICS_EXPORT SourceId AssignNewSourceId();

// Utility for converting other unique ids to source ids.
METRICS_EXPORT SourceId ConvertToSourceId(int64_t other_id,
                                          SourceIdType id_type);

// Get the SourceIdType of the SourceId.
METRICS_EXPORT SourceIdType GetSourceIdType(SourceId source_id);

}  // namespace ukm

#endif  // SERVICES_METRICS_PUBLIC_CPP_UKM_SOURCE_ID_H_
