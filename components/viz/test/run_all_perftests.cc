// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/test/launcher/unit_test_launcher.h"
#include "components/viz/test/viz_test_suite.h"

int main(int argc, char** argv) {
  viz::VizTestSuite test_suite(argc, argv);

  // Always run the perf tests serially, to avoid distorting
  // perf measurements with randomness resulting from running
  // in parallel.
  return base::LaunchUnitTestsSerially(
      argc, argv,
      base::Bind(&viz::VizTestSuite::Run, base::Unretained(&test_suite)));
}
