// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/security_state/content/ssl_status_input_event_data.h"

#include <memory>
#include <utility>

namespace security_state {

SSLStatusInputEventData::SSLStatusInputEventData() {}
SSLStatusInputEventData::SSLStatusInputEventData(
    const InsecureInputEventData& initial_data)
    : data_(initial_data) {}

SSLStatusInputEventData::~SSLStatusInputEventData() {}

InsecureInputEventData* SSLStatusInputEventData::input_events() {
  return &data_;
}

std::unique_ptr<SSLStatus::UserData> SSLStatusInputEventData::Clone() {
  return std::make_unique<SSLStatusInputEventData>(data_);
}

}  // namespace security_state
