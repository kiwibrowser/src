// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/data_with_timestamp.h"

#include <iomanip>

#include "base/logging.h"

namespace cryptauth {

DataWithTimestamp::DataWithTimestamp(const std::string& data,
                                     const int64_t start_timestamp_ms,
                                     const int64_t end_timestamp_ms)
    : data(data),
      start_timestamp_ms(start_timestamp_ms),
      end_timestamp_ms(end_timestamp_ms) {
  DCHECK(start_timestamp_ms < end_timestamp_ms);
  DCHECK(data.size());
}

DataWithTimestamp::DataWithTimestamp(const DataWithTimestamp& other)
    : data(other.data),
      start_timestamp_ms(other.start_timestamp_ms),
      end_timestamp_ms(other.end_timestamp_ms) {
  DCHECK(start_timestamp_ms < end_timestamp_ms);
  DCHECK(data.size());
}

// static.
std::string DataWithTimestamp::ToDebugString(
    const std::vector<DataWithTimestamp>& data_with_timestamps) {
  std::stringstream ss;
  ss << "[";
  for (const DataWithTimestamp& data : data_with_timestamps) {
    ss << "\n  (" << data.start_timestamp_ms << ": " << data.DataInHex()
       << "),";
  }
  ss << "\n]";
  return ss.str();
}

bool DataWithTimestamp::ContainsTime(const int64_t timestamp_ms) const {
  return start_timestamp_ms <= timestamp_ms && timestamp_ms < end_timestamp_ms;
}

std::string DataWithTimestamp::DataInHex() const {
  std::stringstream ss;
  ss << "0x";
  for (uint8_t byte : data) {
    ss << std::hex << std::setfill('0') << std::setw(2)
       << static_cast<uint64_t>(byte);
  }
  return ss.str();
}

bool DataWithTimestamp::operator==(const DataWithTimestamp& other) const {
  return data == other.data && start_timestamp_ms == other.start_timestamp_ms &&
         end_timestamp_ms == other.end_timestamp_ms;
}

}  // namespace cryptauth
