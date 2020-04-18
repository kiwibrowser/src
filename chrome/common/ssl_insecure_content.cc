// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/ssl_insecure_content.h"

#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "url/gurl.h"

namespace {

// Constants for UMA statistic collection.
static const char kDotJS[] = ".js";
static const char kDotCSS[] = ".css";
static const char kDotSWF[] = ".swf";
static const char kDotHTML[] = ".html";

}  // namespace

void ReportInsecureContent(SslInsecureContentType signal) {
  UMA_HISTOGRAM_ENUMERATION(
      "SSL.InsecureContent", static_cast<int>(signal),
      static_cast<int>(SslInsecureContentType::NUM_EVENTS));
}

void FilteredReportInsecureContentDisplayed(const GURL& resource_gurl) {
  if (base::EndsWith(resource_gurl.path(), kDotHTML,
                     base::CompareCase::INSENSITIVE_ASCII)) {
    ReportInsecureContent(SslInsecureContentType::DISPLAY_HTML);
  }
}

void FilteredReportInsecureContentRan(const GURL& resource_gurl) {
  if (base::EndsWith(resource_gurl.path(), kDotJS,
                     base::CompareCase::INSENSITIVE_ASCII)) {
    ReportInsecureContent(SslInsecureContentType::RUN_JS);
  } else if (base::EndsWith(resource_gurl.path(), kDotCSS,
                            base::CompareCase::INSENSITIVE_ASCII)) {
    ReportInsecureContent(SslInsecureContentType::RUN_CSS);
  } else if (base::EndsWith(resource_gurl.path(), kDotSWF,
                            base::CompareCase::INSENSITIVE_ASCII)) {
    ReportInsecureContent(SslInsecureContentType::RUN_SWF);
  }
}
