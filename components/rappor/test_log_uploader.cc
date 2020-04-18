// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/rappor/test_log_uploader.h"

namespace rappor {

TestLogUploader::TestLogUploader() : is_running_(false) {}

TestLogUploader::~TestLogUploader() {}

void TestLogUploader::Start() {
  is_running_ = true;
}

void TestLogUploader::Stop() {
  is_running_ = false;
}

void TestLogUploader::QueueLog(const std::string& string) {
}

}  // namespace rappor
