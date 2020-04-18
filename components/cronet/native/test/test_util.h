// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_NATIVE_TEST_TEST_UTIL_H_
#define COMPONENTS_CRONET_NATIVE_TEST_TEST_UTIL_H_

#include "base/macros.h"
#include "cronet_c.h"

namespace cronet {
// Various test utility functions for testing Cronet.
namespace test {

// Create an engine that is configured to support local test servers.
Cronet_EnginePtr CreateTestEngine(int quic_server_port);

// Create an executor that runs tasks on different background thread.
Cronet_ExecutorPtr CreateTestExecutor();

}  // namespace test
}  // namespace cronet

#endif  // COMPONENTS_CRONET_NATIVE_TEST_TEST_UTIL_H_
