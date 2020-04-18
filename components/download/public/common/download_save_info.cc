// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/public/common/download_save_info.h"

namespace download {

// static
const int64_t DownloadSaveInfo::kLengthFullContent = 0;

DownloadSaveInfo::DownloadSaveInfo()
    : offset(0), length(kLengthFullContent), prompt_for_save_location(false) {}

DownloadSaveInfo::~DownloadSaveInfo() {}

DownloadSaveInfo::DownloadSaveInfo(DownloadSaveInfo&& that)
    : file_path(std::move(that.file_path)),
      suggested_name(std::move(that.suggested_name)),
      file(std::move(that.file)),
      offset(that.offset),
      length(that.length),
      hash_state(std::move(that.hash_state)),
      hash_of_partial_file(std::move(that.hash_of_partial_file)),
      prompt_for_save_location(that.prompt_for_save_location) {}

}  // namespace download
