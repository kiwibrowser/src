// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PAPPI_TESTS_TEST_VIDEO_SOURCE_H_
#define PAPPI_TESTS_TEST_VIDEO_SOURCE_H_

#include <string>

#include "ppapi/c/ppb_core.h"
#include "ppapi/c/private/ppb_video_source_private.h"
#include "ppapi/tests/test_case.h"

class TestVideoSource : public TestCase {
 public:
  explicit TestVideoSource(TestingInstance* instance);
  virtual ~TestVideoSource();

 private:
  // TestCase implementation.
  virtual bool Init();
  virtual void RunTests(const std::string& filter);

  // Overrides.
  virtual void HandleMessage(const pp::Var& message_data);

  std::string TestCreate();
  std::string TestGetFrame();

  const PPB_VideoSource_Private* ppb_video_source_private_interface_;
  const PPB_Core* ppb_core_interface_;
  NestedEvent event_;
  std::string stream_url_;
};

#endif  // PAPPI_TESTS_TEST_VIDEO_SOURCE_H_
