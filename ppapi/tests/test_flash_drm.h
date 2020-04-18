// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PAPPI_TESTS_TEST_FLASH_DRM_H_
#define PAPPI_TESTS_TEST_FLASH_DRM_H_

#include <string>

#include "ppapi/tests/test_case.h"
#include "ppapi/utility/completion_callback_factory.h"

class TestFlashDRM : public TestCase {
 public:
  explicit TestFlashDRM(TestingInstance* instance);

  // TestCase implementation.
  virtual void RunTests(const std::string& filter);

 private:
  std::string TestGetDeviceID();
  std::string TestGetHmonitor();
  std::string TestGetVoucherFile();

  pp::CompletionCallbackFactory<TestFlashDRM> callback_factory_;
};

#endif  // PAPPI_TESTS_TEST_FLASH_DRM_H_
