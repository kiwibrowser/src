// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_DOWNLOADS_RESOURCE_THROTTLE_H_
#define CHROME_BROWSER_OFFLINE_PAGES_DOWNLOADS_RESOURCE_THROTTLE_H_

#include "content/public/browser/resource_throttle.h"
#include "net/url_request/url_request.h"

namespace offline_pages {
namespace downloads {

// This ResourceThrottle is used to hook up into the loading process at the
// moment when headers are available (WillProcessResponse) to determine if the
// download has text/html mime type and should be canceled as a download and
// re-scheduled as a OfflinePage download request. This will be handled by
// Offline Page backend as a full page download rather than a single .html
// file download.
class ResourceThrottle : public content::ResourceThrottle {
 public:
  explicit ResourceThrottle(const net::URLRequest* request);
  ~ResourceThrottle() override;

  // content::ResourceThrottle implementation:
  void WillProcessResponse(bool* defer) override;
  const char* GetNameForLogging() const override;

 private:
  const net::URLRequest* request_;

  DISALLOW_COPY_AND_ASSIGN(ResourceThrottle);
};

}  // namespace downloads
}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_DOWNLOADS_RESOURCE_THROTTLE_H_
