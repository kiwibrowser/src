// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_NAVIGATION_LOADER_UTIL_H_
#define CONTENT_BROWSER_LOADER_NAVIGATION_LOADER_UTIL_H_

#include "base/optional.h"

#include <string>

class GURL;
namespace net {
class HttpResponseHeaders;
}

namespace content {
namespace navigation_loader_util {

// Returns true if the given response must be downloaded because of the headers.
bool MustDownload(const GURL& url,
                  net::HttpResponseHeaders* headers,
                  const std::string& mime_type);

// Determines whether given response would result in a download.
// Note this doesn't handle the case when a plugin exists for the |mime_type|.
bool IsDownload(const GURL& url,
                net::HttpResponseHeaders* headers,
                const std::string& mime_type);

}  // namespace navigation_loader_util
}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_NAVIGATION_LOADER_UTIL_H_
