// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/common/storage_histograms.h"

#include "base/metrics/histogram_functions.h"

namespace storage {

void RecordBytesWritten(const char* label, int amount) {
  const std::string name = "Storage.BytesWritten.";
  base::UmaHistogramCounts10M(name + label, amount);
}

void RecordBytesRead(const char* label, int amount) {
  const std::string name = "Storage.BytesRead.";
  base::UmaHistogramCounts10M(name + label, amount);
}

}  // namespace storage
