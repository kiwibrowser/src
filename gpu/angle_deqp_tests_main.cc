// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/test/launcher/unit_test_launcher.h"
#include "base/test/test_suite.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

int RunHelper(base::TestSuite* test_suite) {
  base::MessageLoop message_loop;
  return test_suite->Run();
}

}  // namespace

// Defined in angle_deqp_gtest.cpp. Declared here so we don't need to make a
// header that we import in Chromium.
namespace angle {
void InitTestHarness(int* argc, char** argv);
}

int main(int argc, char** argv) {
  angle::InitTestHarness(&argc, argv);
  base::CommandLine::Init(argc, argv);
  base::TestSuite test_suite(argc, argv);
  int rt = base::LaunchUnitTestsSerially(
      argc,
      argv,
      base::Bind(&RunHelper, base::Unretained(&test_suite)));
  return rt;
}
