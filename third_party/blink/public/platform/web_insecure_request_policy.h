// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_INSECURE_REQUEST_POLICY_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_INSECURE_REQUEST_POLICY_H_

#include <cstdint>

namespace blink {

// TODO(mkwst): In an ideal world, the combined state would be the same as
// "Upgrade". Once we're consistently upgrading all requests, we can replace
// this bitfield-style representation with an enum. Until then, we need to
// ensure that all relevant flags are set. https://crbug.com/617584
using WebInsecureRequestPolicy = uint8_t;
const WebInsecureRequestPolicy kLeaveInsecureRequestsAlone = 0;
const WebInsecureRequestPolicy kUpgradeInsecureRequests = 1 << 0;
const WebInsecureRequestPolicy kBlockAllMixedContent = 1 << 1;

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_INSECURE_REQUEST_POLICY_H_
