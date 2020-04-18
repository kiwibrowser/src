// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_SAFE_SEARCH_UTIL_H_
#define CHROME_BROWSER_NET_SAFE_SEARCH_UTIL_H_

class GURL;

namespace net {
class HttpRequestHeaders;
class URLRequest;
}

namespace safe_search_util {

// Values for YouTube Restricted Mode.
// VALUES MUST COINCIDE WITH ForceYouTubeRestrict POLICY.
enum YouTubeRestrictMode {
  YOUTUBE_RESTRICT_OFF = 0,      // Do not restrict YouTube content. YouTube
                                 // might still restrict content based on its
                                 // user settings.
  YOUTUBE_RESTRICT_MODERATE = 1, // Enforce at least a moderately strict
                                 // content filter for YouTube.
  YOUTUBE_RESTRICT_STRICT = 2,   // Enforce a strict content filter for YouTube.
  YOUTUBE_RESTRICT_COUNT = 3     // Enum counter
};


// If |request| is a request to Google Web Search, enforces that the SafeSearch
// query parameters are set to active. Sets |new_url| to a copy of the request
// url in which the query part contains the new values of the parameters.
void ForceGoogleSafeSearch(const net::URLRequest* request, GURL* new_url);

// Does nothing if |request| is not a request to YouTube. Otherwise, if |mode|
// is not |YOUTUBE_RESTRICT_OFF|, enforces a minimum YouTube Restrict mode
// by setting YouTube Restrict header. Setting |YOUTUBE_RESTRICT_OFF| is not
// supported and will do nothing in production.
void ForceYouTubeRestrict(const net::URLRequest* request,
                          net::HttpRequestHeaders* headers,
                          YouTubeRestrictMode mode);

int GetForceGoogleSafeSearchCountForTesting();
int GetForceYouTubeRestrictCountForTesting();
void ClearForceGoogleSafeSearchCountForTesting();
void ClearForceYouTubeRestrictCountForTesting();

}  // namespace safe_search_util

#endif  // CHROME_BROWSER_NET_SAFE_SEARCH_UTIL_H_
