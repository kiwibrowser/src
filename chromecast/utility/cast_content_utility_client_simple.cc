// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/utility/cast_content_utility_client.h"

namespace chromecast {
namespace shell {

// static
std::unique_ptr<CastContentUtilityClient> CastContentUtilityClient::Create() {
  return std::unique_ptr<CastContentUtilityClient>();
}

}  // namespace shell
}  // namespace chromecast
