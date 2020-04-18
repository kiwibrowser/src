// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/fileapi/recent_source.h"

#include <utility>

#include "base/logging.h"

namespace chromeos {

RecentSource::Params::Params(storage::FileSystemContext* file_system_context,
                             const GURL& origin,
                             size_t max_files,
                             const base::Time& cutoff_time,
                             GetRecentFilesCallback callback)
    : file_system_context_(file_system_context),
      origin_(origin),
      max_files_(max_files),
      cutoff_time_(cutoff_time),
      callback_(std::move(callback)) {
  DCHECK(!callback_.is_null());
}

RecentSource::Params::Params(Params&& other) = default;

RecentSource::Params::~Params() = default;

RecentSource::RecentSource() = default;

RecentSource::~RecentSource() = default;

}  // namespace chromeos
