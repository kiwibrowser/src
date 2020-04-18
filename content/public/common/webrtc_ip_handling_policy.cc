// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/webrtc_ip_handling_policy.h"

namespace content {

// The set of strings here need to match what's specified in privacy.json.
const char kWebRTCIPHandlingDefault[] = "default";
const char kWebRTCIPHandlingDefaultPublicAndPrivateInterfaces[] =
    "default_public_and_private_interfaces";
const char kWebRTCIPHandlingDefaultPublicInterfaceOnly[] =
    "default_public_interface_only";
const char kWebRTCIPHandlingDisableNonProxiedUdp[] = "disable_non_proxied_udp";

}  // namespace content
