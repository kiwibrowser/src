// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/network_hints/common/network_hints_common.h"

namespace network_hints {

const size_t kMaxDnsHostnamesPerRequest = 30;
const size_t kMaxDnsHostnameLength = 255;

LookupRequest::LookupRequest() {
}

LookupRequest::~LookupRequest() {
}

}  // namespace network_hints
