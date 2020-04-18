// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/renderer/fixed_received_data.h"

namespace content {

FixedReceivedData::FixedReceivedData(const char* data, size_t length)
    : data_(data, data + length) {}

FixedReceivedData::FixedReceivedData(ReceivedData* data)
    : FixedReceivedData(data->payload(), data->length()) {}

FixedReceivedData::FixedReceivedData(const std::vector<char>& data)
    : data_(data) {}

FixedReceivedData::~FixedReceivedData() {
}

const char* FixedReceivedData::payload() const {
  return data_.empty() ? nullptr : &data_[0];
}

int FixedReceivedData::length() const {
  return static_cast<int>(data_.size());
}

}  // namespace content
