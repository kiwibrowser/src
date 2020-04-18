// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FAVICON_CONTENT_FAVICON_URL_UTIL_H_
#define COMPONENTS_FAVICON_CONTENT_FAVICON_URL_UTIL_H_

#include <vector>

namespace content {
struct FaviconURL;
}

namespace favicon {

struct FaviconURL;

// Creates a favicon::FaviconURL from a content::FaviconURL.
FaviconURL FaviconURLFromContentFaviconURL(
    const content::FaviconURL& favicon_url);

// Creates favicon::FaviconURLs from content::FaviconURLs.
std::vector<FaviconURL> FaviconURLsFromContentFaviconURLs(
    const std::vector<content::FaviconURL>& favicon_urls);

}  // namespace favicon

#endif  // COMPONENTS_FAVICON_CONTENT_FAVICON_URL_UTIL_H_
