// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGES_UKM_REPORTER_STUB_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGES_UKM_REPORTER_STUB_H_

#include "components/offline_pages/core/offline_pages_ukm_reporter.h"
#include "url/gurl.h"

namespace offline_pages {

class OfflinePagesUkmReporterStub : public OfflinePagesUkmReporter {
 public:
  OfflinePagesUkmReporterStub();
  ~OfflinePagesUkmReporterStub() override = default;
  void ReportUrlOfflineRequest(const GURL& gurl, bool foreground) override;

  const GURL& GetLastOfflinedUrl() const { return gurl_; }
  bool GetForeground() const { return foreground_; }

 private:
  GURL gurl_;
  bool foreground_;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGES_UKM_REPORTER_STUB_H_
