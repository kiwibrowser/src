// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/dom_distiller/core/url_utils.h"

#include <string>

#include "base/guid.h"
#include "base/strings/string_number_conversions.h"
#include "components/dom_distiller/core/url_constants.h"
#include "components/grit/components_resources.h"
#include "net/base/url_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"

namespace dom_distiller {

namespace url_utils {

namespace {

const char kDummyInternalUrlPrefix[] = "chrome-distiller-internal://dummy/";

}  // namespace

const GURL GetDistillerViewUrlFromEntryId(const std::string& scheme,
                                          const std::string& entry_id) {
  GURL url(scheme + "://" + base::GenerateGUID());
  return net::AppendOrReplaceQueryParameter(url, kEntryIdKey, entry_id);
}

const GURL GetDistillerViewUrlFromUrl(const std::string& scheme,
                                      const GURL& view_url,
                                      int64_t start_time_ms) {
  GURL url(scheme + "://" + base::GenerateGUID());
  if (start_time_ms > 0) {
    url = net::AppendOrReplaceQueryParameter(url, kTimeKey,
        base::IntToString(start_time_ms));
  }
  return net::AppendOrReplaceQueryParameter(url, kUrlKey, view_url.spec());
}

const GURL GetOriginalUrlFromDistillerUrl(const GURL& url) {
  if (!dom_distiller::url_utils::IsDistilledPage(url))
    return url;

  std::string original_url_str;
  net::GetValueForKeyInQuery(url, kUrlKey, &original_url_str);

  return GURL(original_url_str);
}

int64_t GetTimeFromDistillerUrl(const GURL& url) {
  if (!dom_distiller::url_utils::IsDistilledPage(url))
    return 0;

  std::string time_str;
  if (!net::GetValueForKeyInQuery(url, kTimeKey, &time_str))
    return 0;

  int64_t time_int = 0;
  if (!base::StringToInt64(time_str, &time_int))
    return 0;

  return time_int;
}

std::string GetValueForKeyInUrl(const GURL& url, const std::string& key) {
  if (!url.is_valid())
    return "";
  std::string value;
  if (net::GetValueForKeyInQuery(url, key, &value)) {
    return value;
  }
  return "";
}

std::string GetValueForKeyInUrlPathQuery(const std::string& path,
                                         const std::string& key) {
  // Tools for retrieving a value in a query only works with full GURLs, so
  // using a dummy scheme and host to create a fake URL which can be parsed.
  GURL dummy_url(kDummyInternalUrlPrefix + path);
  return GetValueForKeyInUrl(dummy_url, key);
}

bool IsUrlDistillable(const GURL& url) {
  return url.is_valid() && url.SchemeIsHTTPOrHTTPS();
}

bool IsDistilledPage(const GURL& url) {
  return url.is_valid() && url.scheme() == kDomDistillerScheme;
}

}  // namespace url_utils

}  // namespace dom_distiller
