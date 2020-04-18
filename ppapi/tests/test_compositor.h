// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PAPPI_TESTS_TEST_COMPOSITOR_H_
#define PAPPI_TESTS_TEST_COMPOSITOR_H_

#include <stdint.h>

#include <set>
#include <string>

#include "ppapi/cpp/compositor.h"
#include "ppapi/cpp/compositor_layer.h"
#include "ppapi/cpp/graphics_3d.h"
#include "ppapi/lib/gl/include/GLES2/gl2.h"
#include "ppapi/tests/test_case.h"

class TestCompositor : public TestCase {
 public:
  TestCompositor(TestingInstance* instance) : TestCase(instance) {}

  // TestCase implementation.
  virtual bool Init();
  virtual void RunTests(const std::string& filter);

 private:
  // Various tests.
  std::string TestRelease();
  std::string TestReleaseWithoutCommit();
  std::string TestCommitTwoTimesWithoutChange();
  std::string TestGeneral();

  std::string TestReleaseUnbound();
  std::string TestReleaseWithoutCommitUnbound();
  std::string TestCommitTwoTimesWithoutChangeUnbound();
  std::string TestGeneralUnbound();

  std::string TestBindUnbind();

  std::string TestReleaseInternal(bool bind);
  std::string TestReleaseWithoutCommitInternal(bool bind);
  std::string TestCommitTwoTimesWithoutChangeInternal(bool bind);
  std::string TestGeneralInternal(bool bind);

  // Helper functions
  std::string CreateTexture(uint32_t* texture);
  std::string ReleaseTexture(uint32_t texture);
  std::string CreateImage(pp::ImageData* image);
  std::string SetColorLayer(pp::CompositorLayer layer, int32_t result);

};

#endif  // PAPPI_TESTS_TEST_COMPOSItor_H_
