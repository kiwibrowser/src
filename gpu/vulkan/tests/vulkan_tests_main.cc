// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/test/launcher/unit_test_launcher.h"
#include "base/test/test_suite.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace {

int RunHelper(base::TestSuite* testSuite) {
  return testSuite->Run();
}

}  // namespace

int main(int argc, char** argv) {
  base::TestSuite test_suite(argc, argv);
  base::CommandLine::Init(argc, argv);
  testing::InitGoogleMock(&argc, argv);
  return base::LaunchUnitTestsSerially(
      argc, argv, base::Bind(&RunHelper, base::Unretained(&test_suite)));
}
