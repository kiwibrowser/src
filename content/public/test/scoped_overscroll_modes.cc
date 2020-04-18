// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/scoped_overscroll_modes.h"

namespace content {

ScopedHistoryNavigationMode::ScopedHistoryNavigationMode(
    OverscrollConfig::HistoryNavigationMode mode) {
  OverscrollConfig::SetHistoryNavigationMode(mode);
}

ScopedHistoryNavigationMode::~ScopedHistoryNavigationMode() {
  OverscrollConfig::ResetHistoryNavigationMode();
}

ScopedPullToRefreshMode::ScopedPullToRefreshMode(
    OverscrollConfig::PullToRefreshMode mode) {
  OverscrollConfig::SetPullToRefreshMode(mode);
}

ScopedPullToRefreshMode::~ScopedPullToRefreshMode() {
  OverscrollConfig::ResetPullToRefreshMode();
}

}  // namespace content
