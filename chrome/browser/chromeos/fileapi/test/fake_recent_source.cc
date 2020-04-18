// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/fileapi/test/fake_recent_source.h"

#include <utility>

#include "chrome/browser/chromeos/fileapi/recent_file.h"

namespace chromeos {

FakeRecentSource::FakeRecentSource() = default;

FakeRecentSource::~FakeRecentSource() = default;

void FakeRecentSource::AddFile(const RecentFile& file) {
  canned_files_.emplace_back(file);
}

void FakeRecentSource::GetRecentFiles(Params params) {
  std::move(params.callback()).Run(canned_files_);
}

}  // namespace chromeos
