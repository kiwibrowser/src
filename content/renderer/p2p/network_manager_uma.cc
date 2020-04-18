// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/p2p/network_manager_uma.h"

#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"

namespace content {

void ReportTimeToUpdateNetworkList(const base::TimeDelta& ticks) {
  UMA_HISTOGRAM_TIMES("WebRTC.PeerConnection.TimeToNetworkUpdated", ticks);
}

void ReportIPPermissionStatus(IPPermissionStatus status) {
  UMA_HISTOGRAM_ENUMERATION("WebRTC.PeerConnection.IPPermissionStatus", status,
                            PERMISSION_MAX);
}

}  // namespace content
