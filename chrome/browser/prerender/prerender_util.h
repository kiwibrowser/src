// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRERENDER_PRERENDER_UTIL_H_
#define CHROME_BROWSER_PRERENDER_PRERENDER_UTIL_H_

class GURL;

namespace prerender {

// Indicates whether the URL provided is a GWS origin.
bool IsGoogleOriginURL(const GURL& origin_url);

// Report a URL was canceled due to trying to handle an external URL.
void ReportPrerenderExternalURL();

// Report a URL was canceled due to unsupported prerender scheme.
void ReportUnsupportedPrerenderScheme(const GURL& url);

}  // namespace prerender

#endif  // CHROME_BROWSER_PRERENDER_PRERENDER_UTIL_H_
