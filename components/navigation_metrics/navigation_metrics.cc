// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/navigation_metrics/navigation_metrics.h"

#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "components/dom_distiller/core/url_constants.h"
#include "url/gurl.h"

namespace navigation_metrics {

namespace {

const char* const kSchemeNames[] = {
    "unknown",
    url::kHttpScheme,
    url::kHttpsScheme,
    url::kFileScheme,
    url::kFtpScheme,
    url::kDataScheme,
    url::kJavaScriptScheme,
    url::kAboutScheme,
    "chrome",
    url::kBlobScheme,
    url::kFileSystemScheme,
    "chrome-native",
    "chrome-search",
    dom_distiller::kDomDistillerScheme,
    "chrome-devtools",
    "chrome-extension",
    "view-source",
    "externalfile",
};

static_assert(arraysize(kSchemeNames) == static_cast<int>(Scheme::COUNT),
              "kSchemeNames should have Scheme::COUNT elements");

}  // namespace

Scheme GetScheme(const GURL& url) {
  for (int i = static_cast<int>(Scheme::HTTP);
       i < static_cast<int>(Scheme::COUNT); ++i) {
    if (url.SchemeIs(kSchemeNames[i]))
      return static_cast<Scheme>(i);
  }
  return Scheme::UNKNOWN;
}

void RecordMainFrameNavigation(const GURL& url,
                               bool is_same_document,
                               bool is_off_the_record) {
  Scheme scheme = GetScheme(url);
  UMA_HISTOGRAM_ENUMERATION("Navigation.MainFrameScheme", scheme,
                            Scheme::COUNT);
  if (!is_same_document) {
    UMA_HISTOGRAM_ENUMERATION("Navigation.MainFrameSchemeDifferentPage", scheme,
                              Scheme::COUNT);
  }

  if (is_off_the_record) {
    UMA_HISTOGRAM_ENUMERATION("Navigation.MainFrameSchemeOTR", scheme,
                              Scheme::COUNT);
    if (!is_same_document) {
      UMA_HISTOGRAM_ENUMERATION("Navigation.MainFrameSchemeDifferentPageOTR",
                                scheme, Scheme::COUNT);
    }
  }
}

void RecordOmniboxURLNavigation(const GURL& url) {
  UMA_HISTOGRAM_ENUMERATION("Omnibox.URLNavigationScheme", GetScheme(url),
                            Scheme::COUNT);
}

}  // namespace navigation_metrics
