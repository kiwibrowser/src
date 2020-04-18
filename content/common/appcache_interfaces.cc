// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/appcache_interfaces.h"

#include <set>

#include "base/strings/pattern.h"
#include "base/strings/string_util.h"
#include "content/public/common/url_constants.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace content {

const char kHttpGETMethod[] = "GET";
const char kHttpHEADMethod[] = "HEAD";

const base::FilePath::CharType kAppCacheDatabaseName[] =
    FILE_PATH_LITERAL("Index");

AppCacheNamespace::AppCacheNamespace()
    : type(APPCACHE_FALLBACK_NAMESPACE), is_pattern(false) {}

AppCacheNamespace::AppCacheNamespace(AppCacheNamespaceType type,
                                     const GURL& url,
                                     const GURL& target,
                                     bool is_pattern)
    : type(type),
      namespace_url(url),
      target_url(target),
      is_pattern(is_pattern) {}

AppCacheNamespace::~AppCacheNamespace() {
}

bool AppCacheNamespace::IsMatch(const GURL& url) const {
  if (is_pattern) {
    // We have to escape '?' characters since MatchPattern also treats those
    // as wildcards which we don't want here, we only do '*'s.
    std::string pattern = namespace_url.spec();
    if (namespace_url.has_query())
      base::ReplaceSubstringsAfterOffset(&pattern, 0, "?", "\\?");
    return base::MatchPattern(url.spec(), pattern);
  }
  return base::StartsWith(url.spec(), namespace_url.spec(),
                          base::CompareCase::SENSITIVE);
}

bool IsSchemeSupportedForAppCache(const GURL& url) {
  bool supported = url.SchemeIs(url::kHttpScheme) ||
                   url.SchemeIs(url::kHttpsScheme) ||
                   url.SchemeIs(kChromeDevToolsScheme);

#ifndef NDEBUG
  // TODO(michaeln): It would be really nice if this could optionally work for
  // file and filesystem urls too to help web developers experiment and test
  // their apps, perhaps enabled via a cmd line flag or some other developer
  // tool setting.  Unfortunately file scheme net::URLRequests don't produce the
  // same signalling (200 response codes, headers) as http URLRequests, so this
  // doesn't work just yet.
  // supported |= url.SchemeIsFile();
#endif
  return supported;
}

bool IsMethodSupportedForAppCache(const std::string& method) {
  return (method == kHttpGETMethod) || (method == kHttpHEADMethod);
}

}  // namespace content
