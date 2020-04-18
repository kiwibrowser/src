// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/feature_list.h"
#include "base/message_loop/message_loop.h"
#if defined(OS_MACOSX)
#include "base/mac/scoped_nsautorelease_pool.h"
#endif
#include "base/test/launcher/unit_test_launcher.h"
#include "base/test/test_suite.h"
#include "gpu/command_buffer/client/gles2_lib.h"
#include "gpu/command_buffer/tests/gl_test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace {

int RunHelper(base::TestSuite* testSuite) {
#if defined(USE_OZONE)
  base::MessageLoopForUI main_loop;
#else
  base::MessageLoopForIO message_loop;
#endif
  base::FeatureList::InitializeInstance(std::string(), std::string());
  gpu::GLTestHelper::InitializeGLDefault();
  ::gles2::Initialize();
  return testSuite->Run();
}

}  // namespace

int main(int argc, char** argv) {
  base::TestSuite test_suite(argc, argv);
  base::CommandLine::Init(argc, argv);
#if defined(OS_MACOSX)
  base::mac::ScopedNSAutoreleasePool pool;
#endif
  testing::InitGoogleMock(&argc, argv);
  return base::LaunchUnitTestsSerially(
      argc,
      argv,
      base::Bind(&RunHelper, base::Unretained(&test_suite)));
}
