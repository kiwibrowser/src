// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_RAPPOR_TEST_LOG_UPLOADER_H_
#define COMPONENTS_RAPPOR_TEST_LOG_UPLOADER_H_

#include <string>

#include "base/macros.h"
#include "components/rappor/log_uploader_interface.h"

namespace rappor {

class TestLogUploader : public LogUploaderInterface {
 public:
  TestLogUploader();
  ~TestLogUploader() override;

  // LogUploaderInterface:
  void Start() override;
  void Stop() override;
  void QueueLog(const std::string& log) override;

  // Get if the uploader is running.
  bool is_running() { return is_running_; }

 private:
  // True if Start was called since the last Stop.
  bool is_running_;

  DISALLOW_COPY_AND_ASSIGN(TestLogUploader);
};

}  // namespace rappor

#endif  // COMPONENTS_RAPPOR_LOG_UPLOADER_INTERFACE_H_
