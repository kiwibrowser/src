// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/search_util.h"

#include "base/metrics/histogram_macros.h"

namespace {
const char kAppListSearchResultOpenTypeHistogram[] =
    "Apps.AppListSearchResultOpenType";
}

namespace app_list {

void RecordHistogram(SearchResultType type) {
  UMA_HISTOGRAM_ENUMERATION(
      kAppListSearchResultOpenTypeHistogram, type, SEARCH_RESULT_TYPE_BOUNDARY);
}

}  // namespace app_list
