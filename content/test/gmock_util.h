// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gmock/include/gmock/gmock.h"

#ifndef CONTENT_TEST_GMOCK_UTIL_H_
#define CONTENT_TEST_GMOCK_UTIL_H_

// This file contains gmock actions and matchers that integrate with concepts in
// /base.

namespace base {
namespace test {

// Defines a gmock action that runs a given closure.
ACTION_P(RunClosure, closure) {
  closure.Run();
}

}  // namespace test
}  // namespace base

#endif  // CONTENT_TEST_GMOCK_UTIL_H_
