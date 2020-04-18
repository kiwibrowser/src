// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/background_fetch_response.h"

namespace content {

BackgroundFetchResponse::BackgroundFetchResponse(
    const std::vector<GURL>& url_chain,
    const scoped_refptr<const net::HttpResponseHeaders>& headers)
    : url_chain(url_chain), headers(headers) {}

BackgroundFetchResponse::~BackgroundFetchResponse() {}

BackgroundFetchResult::BackgroundFetchResult(base::Time response_time,
                                             FailureReason failure_reason)
    : response_time(response_time), failure_reason(failure_reason) {}

BackgroundFetchResult::BackgroundFetchResult(base::Time response_time,
                                             const base::FilePath& path,
                                             uint64_t file_size)
    : response_time(response_time),
      file_path(path),
      file_size(file_size),
      failure_reason(FailureReason::NONE) {}

BackgroundFetchResult::~BackgroundFetchResult() {}

}  // namespace content
