// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/net/wifi_access_point_info_provider.h"

namespace metrics {

WifiAccessPointInfoProvider::WifiAccessPointInfo::WifiAccessPointInfo() {
}

WifiAccessPointInfoProvider::WifiAccessPointInfo::~WifiAccessPointInfo() {
}

WifiAccessPointInfoProvider::WifiAccessPointInfoProvider() {
}

WifiAccessPointInfoProvider::~WifiAccessPointInfoProvider() {
}

bool WifiAccessPointInfoProvider::GetInfo(WifiAccessPointInfo *info) {
  return false;
}

}  // namespace metrics
