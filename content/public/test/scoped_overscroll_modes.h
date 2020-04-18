// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_SCOPED_OVERSCROLL_MODES_H_
#define CONTENT_PUBLIC_TEST_SCOPED_OVERSCROLL_MODES_H_

#include "base/macros.h"
#include "content/public/browser/overscroll_configuration.h"

namespace content {

// Helper class to set the overscroll history navigation mode temporarily in
// tests.
class ScopedHistoryNavigationMode {
 public:
  explicit ScopedHistoryNavigationMode(
      OverscrollConfig::HistoryNavigationMode mode);
  ~ScopedHistoryNavigationMode();

 private:
  DISALLOW_COPY_AND_ASSIGN(ScopedHistoryNavigationMode);
};

// Helper class to set the pull-to-refresh mode temporarily in tests.
class ScopedPullToRefreshMode {
 public:
  explicit ScopedPullToRefreshMode(OverscrollConfig::PullToRefreshMode mode);
  ~ScopedPullToRefreshMode();

 private:
  DISALLOW_COPY_AND_ASSIGN(ScopedPullToRefreshMode);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_SCOPED_OVERSCROLL_MODES_H_
