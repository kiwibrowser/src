// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/common/download_job_impl.h"

namespace download {

DownloadJobImpl::DownloadJobImpl(
    DownloadItem* download_item,
    std::unique_ptr<DownloadRequestHandleInterface> request_handle,
    bool is_parallizable)
    : DownloadJob(download_item, std::move(request_handle)),
      is_parallizable_(is_parallizable) {}

DownloadJobImpl::~DownloadJobImpl() = default;

bool DownloadJobImpl::IsParallelizable() const {
  return is_parallizable_;
}

}  // namespace download
