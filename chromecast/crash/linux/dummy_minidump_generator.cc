// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/crash/linux/dummy_minidump_generator.h"

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/threading/thread_restrictions.h"

namespace chromecast {

DummyMinidumpGenerator::DummyMinidumpGenerator(
    const std::string& existing_minidump_path)
    : existing_minidump_path_(existing_minidump_path) {
}

bool DummyMinidumpGenerator::Generate(const std::string& minidump_path) {
  base::AssertBlockingAllowed();

  // Return false if the file does not exist.
  if (!base::PathExists(base::FilePath(existing_minidump_path_))) {
    LOG(ERROR) << existing_minidump_path_ << " does not exist.";
    return false;
  }

  LOG(INFO) << "Moving minidump from " << existing_minidump_path_ << " to "
            << minidump_path << " for further uploading.";
  return base::Move(base::FilePath(existing_minidump_path_),
                    base::FilePath(minidump_path));
}

}  // namespace chromecast
