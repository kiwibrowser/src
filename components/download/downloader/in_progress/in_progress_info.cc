// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/downloader/in_progress/in_progress_info.h"

namespace download {

InProgressInfo::InProgressInfo() = default;

InProgressInfo::InProgressInfo(const InProgressInfo& other) = default;

InProgressInfo::~InProgressInfo() = default;

bool InProgressInfo::operator==(const InProgressInfo& other) const {
  return url_chain == other.url_chain &&
         fetch_error_body == other.fetch_error_body &&
         request_headers == other.request_headers && etag == other.etag &&
         last_modified == other.last_modified &&
         total_bytes == other.total_bytes &&
         current_path == other.current_path &&
         target_path == other.target_path &&
         received_bytes == other.received_bytes && end_time == other.end_time &&
         received_slices == other.received_slices && hash == other.hash &&
         transient == other.transient && state == other.state &&
         danger_type == other.danger_type &&
         interrupt_reason == other.interrupt_reason && paused == other.paused &&
         metered == other.metered && request_origin == other.request_origin &&
         bytes_wasted == other.bytes_wasted;
}

}  // namespace download
