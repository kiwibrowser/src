// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_use_measurement/core/data_use.h"

namespace data_use_measurement {

DataUse::DataUse(TrafficType traffic_type)
    : traffic_type_(traffic_type),
      total_bytes_sent_(0),
      total_bytes_received_(0) {}

DataUse::~DataUse() {}

void DataUse::IncrementTotalBytes(int64_t bytes_received, int64_t bytes_sent) {
  total_bytes_received_ += bytes_received;
  total_bytes_sent_ += bytes_sent;
  DCHECK_LE(0, total_bytes_received_);
  DCHECK_LE(0, total_bytes_sent_);
}

}  // namespace data_use_measurement
