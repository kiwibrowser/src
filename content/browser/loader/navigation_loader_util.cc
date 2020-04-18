// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/navigation_loader_util.h"

#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"
#include "net/http/http_content_disposition.h"
#include "net/http/http_response_headers.h"
#include "third_party/blink/public/common/mime_util/mime_util.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {
namespace navigation_loader_util {

bool MustDownload(const GURL& url,
                  net::HttpResponseHeaders* headers,
                  const std::string& mime_type) {
  if (headers) {
    std::string disposition;
    if (headers->GetNormalizedHeader("content-disposition", &disposition) &&
        !disposition.empty() &&
        net::HttpContentDisposition(disposition, std::string())
            .is_attachment()) {
      return true;
    }
    if (GetContentClient()->browser()->ShouldForceDownloadResource(url,
                                                                   mime_type))
      return true;
    if (mime_type == "multipart/related" || mime_type == "message/rfc822") {
      // TODO(https://crbug.com/790734): retrieve the new NavigationUIData from
      // the request and and pass it to AllowRenderingMhtmlOverHttp().
      return !GetContentClient()->browser()->AllowRenderingMhtmlOverHttp(
          nullptr);
    }
    // TODO(qinmin): Check whether this is special-case user script that needs
    // to be downloaded.
  }

  return false;
}

bool IsDownload(const GURL& url,
                net::HttpResponseHeaders* headers,
                const std::string& mime_type) {
  if (MustDownload(url, headers, mime_type))
    return true;

  if (blink::IsSupportedMimeType(mime_type))
    return false;

  return !headers || headers->response_code() / 100 == 2;
}

}  // namespace navigation_loader_util
}  // namespace content
