// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/test_suite.h"
#include "services/service_manager/public/cpp/test/common_initialization.h"

int main(int argc, char** argv) {
  base::TestSuite test_suite(argc, argv);

  return service_manager::InitializeAndLaunchUnitTests(
      argc, argv,
      base::Bind(&base::TestSuite::Run, base::Unretained(&test_suite)));
}
