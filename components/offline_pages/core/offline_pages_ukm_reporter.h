// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGES_UKM_REPORTER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGES_UKM_REPORTER_H_

class GURL;

namespace offline_pages {

// Interface for reporting that a URL has been offlined.  The implementation
// needs to be in BROWSER code, but the interface is used from COMPONENTS code.
class OfflinePagesUkmReporter {
 public:
  virtual ~OfflinePagesUkmReporter() = default;

  // Report that an offline copy has been made of this URL.
  virtual void ReportUrlOfflineRequest(const GURL& gurl, bool foreground);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGES_UKM_REPORTER_H_
