
// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/history_report/usage_report_util.h"

#include <iomanip>
#include <sstream>

#include "chrome/browser/android/proto/delta_file.pb.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

namespace history_report {
namespace usage_report_util {

// Returns a levelDb key for a report. It's a concatenation of timestamp and id
// fields of a report.
std::string ReportToKey(const history_report::UsageReport& report) {
  std::stringstream key;
  key << std::setfill('0') << std::setw(15) << report.timestamp_ms()
      << report.id();
  return key.str();
}

bool IsTypedVisit(ui::PageTransition visit_transition) {
  return ui::PageTransitionCoreTypeIs(visit_transition,
                                      ui::PAGE_TRANSITION_TYPED) &&
         !ui::PageTransitionIsRedirect(visit_transition);
}

bool ShouldIgnoreUrl(const GURL& url) {
  if (url.spec().empty())
    return true;

  // Ignore local file URLs.
  // TODO(nileshagrawal): Maybe we should ignore content:// urls too.
  if (url.SchemeIsFile())
    return true;

  // Ignore localhost URLs.
  if (net::IsLocalhost(url))
    return true;

  return false;
}

}  // namespace usage_report_util
}  // namespace history_report
