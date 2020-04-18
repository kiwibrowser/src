// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Entry point for tests. gtest is able to automatically run the tests in other
// *.cc files as long as they use the TEST macro.

#include "native_client_sdk/src/libraries/ppapi_simple/ps_main.h"
#include "testing/gtest/gtest.h"

int test_main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

// Register the function to call once the Instance Object is initialized.
// see: pappi_simple/ps_main.h
PPAPI_SIMPLE_REGISTER_MAIN(test_main);
