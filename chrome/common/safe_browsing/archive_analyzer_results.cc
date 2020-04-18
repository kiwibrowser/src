// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file contains the archive file analysis implementation for download
// protection, which runs in a sandboxed utility process.

#include "chrome/common/safe_browsing/archive_analyzer_results.h"

namespace safe_browsing {

ArchiveAnalyzerResults::ArchiveAnalyzerResults()
    : success(false), has_executable(false), has_archive(false) {}

ArchiveAnalyzerResults::ArchiveAnalyzerResults(
    const ArchiveAnalyzerResults& other) = default;

ArchiveAnalyzerResults::~ArchiveAnalyzerResults() {}

}  // namespace safe_browsing