// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/base/data_type_histogram.h"

#include "base/metrics/histogram_functions.h"

const char kModelTypeMemoryHistogramPrefix[] = "Sync.ModelTypeMemoryKB.";

void SyncRecordMemoryKbHistogram(const std::string& histogram_name_prefix,
                                 syncer::ModelType model_type,
                                 size_t value) {
  std::string type_string;
  if (RealModelTypeToNotificationType(model_type, &type_string)) {
    std::string full_histogram_name = histogram_name_prefix + type_string;
    base::UmaHistogramCounts1M(full_histogram_name, value / 1024);
  }
}
