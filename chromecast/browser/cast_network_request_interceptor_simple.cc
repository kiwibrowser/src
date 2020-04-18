// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/cast_network_request_interceptor.h"

namespace chromecast {

std::unique_ptr<CastNetworkRequestInterceptor>
CastNetworkRequestInterceptor::Create() {
  return std::make_unique<CastNetworkRequestInterceptor>();
}

}  // namespace chromecast
