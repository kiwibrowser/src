// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/security_state/ios/ssl_status_input_event_data.h"

#include <memory>

namespace security_state {

SSLStatusInputEventData::SSLStatusInputEventData() {}

SSLStatusInputEventData::SSLStatusInputEventData(
    const security_state::InsecureInputEventData& initial_data)
    : data_(initial_data) {}

SSLStatusInputEventData::~SSLStatusInputEventData() {}

security_state::InsecureInputEventData*
SSLStatusInputEventData::input_events() {
  return &data_;
}

std::unique_ptr<web::SSLStatus::UserData> SSLStatusInputEventData::Clone() {
  return std::make_unique<SSLStatusInputEventData>(data_);
}

}  // namespace security_state
