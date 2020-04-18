// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/net/view_blob_internals_job_factory.h"

#include <memory>

#include "base/strings/string_util.h"
#include "content/public/common/url_constants.h"
#include "storage/browser/blob/view_blob_internals_job.h"

namespace content {

// static.
bool ViewBlobInternalsJobFactory::IsSupportedURL(const GURL& url) {
  return url.SchemeIs(kChromeUIScheme) &&
         url.host_piece() == kChromeUIBlobInternalsHost;
}

// static.
net::URLRequestJob* ViewBlobInternalsJobFactory::CreateJobForRequest(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    storage::BlobStorageContext* blob_storage_context) {
  return new storage::ViewBlobInternalsJob(
      request, network_delegate, blob_storage_context);
}

}  // namespace content
