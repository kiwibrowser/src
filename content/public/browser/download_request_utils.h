// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_DOWNLOAD_REQUEST_UTILS_H_
#define CONTENT_PUBLIC_BROWSER_DOWNLOAD_REQUEST_UTILS_H_

#include <string>

#include "components/download/public/common/download_url_parameters.h"
#include "content/common/content_export.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

class GURL;

namespace net {
class URLRequest;
}  // namespace net

namespace content {

class WebContents;

// Utility methods for download requests.
class CONTENT_EXPORT DownloadRequestUtils {
 public:
  // Returns the identifier for origin of the download.
  static std::string GetRequestOriginFromRequest(
      const net::URLRequest* request);

  // Construct download::DownloadUrlParameters for downloading the resource at
  // |url| and associating the download with the main frame of the given
  // WebContents.
  static std::unique_ptr<download::DownloadUrlParameters>
  CreateDownloadForWebContentsMainFrame(
      WebContents* web_contents,
      const GURL& url,
      const net::NetworkTrafficAnnotationTag& traffic_annotation);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_DOWNLOAD_REQUEST_UTILS_H_
