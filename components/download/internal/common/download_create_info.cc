// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/public/common/download_create_info.h"

#include <string>

#include "base/format_macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "net/http/http_response_headers.h"

namespace download {

DownloadCreateInfo::DownloadCreateInfo(
    const base::Time& start_time,
    std::unique_ptr<DownloadSaveInfo> save_info)
    : download_id(DownloadItem::kInvalidId),
      start_time(start_time),
      total_bytes(0),
      offset(0),
      has_user_gesture(false),
      transient(false),
      result(DOWNLOAD_INTERRUPT_REASON_NONE),
      save_info(std::move(save_info)),
      render_process_id(-1),
      render_frame_id(-1),
      accept_range(false),
      connection_info(net::HttpResponseInfo::CONNECTION_INFO_UNKNOWN),
      method("GET"),
      ukm_source_id(ukm::kInvalidSourceId) {}

DownloadCreateInfo::DownloadCreateInfo()
    : DownloadCreateInfo(base::Time(), base::WrapUnique(new DownloadSaveInfo)) {
}

DownloadCreateInfo::~DownloadCreateInfo() {}

const GURL& DownloadCreateInfo::url() const {
  return url_chain.empty() ? GURL::EmptyGURL() : url_chain.back();
}

}  // namespace download
