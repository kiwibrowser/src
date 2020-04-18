// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/offline_pages_ukm_reporter_stub.h"

namespace offline_pages {

OfflinePagesUkmReporterStub::OfflinePagesUkmReporterStub()
    : foreground_(false) {}

void OfflinePagesUkmReporterStub::ReportUrlOfflineRequest(const GURL& gurl,
                                                          bool foreground) {
  gurl_ = gurl;
  foreground_ = foreground;
}

}  // namespace offline_pages
