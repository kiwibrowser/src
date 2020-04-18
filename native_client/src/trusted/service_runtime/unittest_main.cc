/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


// Testing NativeClient's service_runtime functionality


#include "native_client/src/include/portability.h"

#include "gtest/gtest.h"

// TODO(gregoryd) - more tests

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
