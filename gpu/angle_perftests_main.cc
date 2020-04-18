// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/test/launcher/unit_test_launcher.h"
#include "base/test/test_suite.h"

namespace {

int RunHelper(base::TestSuite* test_suite) {
  base::MessageLoopForIO message_loop;
  return test_suite->Run();
}

}  // namespace

extern bool g_OnlyOneRunFrame;

int main(int argc, char** argv) {
  for (int i = 0; i < argc; ++i) {
    if (strcmp("--one-frame-only", argv[i]) == 0) {
      g_OnlyOneRunFrame = true;
    }
  }

  base::CommandLine::Init(argc, argv);
  base::TestSuite test_suite(argc, argv);
  int rt = base::LaunchUnitTestsSerially(
      argc,
      argv,
      base::Bind(&RunHelper, base::Unretained(&test_suite)));
  return rt;
}
