// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_PUBLIC_CPP_TEST_COMMON_INITIALIZATION_H_
#define SERVICES_SERVICE_MANAGER_PUBLIC_CPP_TEST_COMMON_INITIALIZATION_H_

#include "base/test/launcher/unit_test_launcher.h"

namespace service_manager {

// Does common mojo edk/catalog initialization that needs to happen in all
// service tests. This method exists so that different test runners can use
// different base::TestSuite instances, but still use the common mojo
// initialization.
int InitializeAndLaunchUnitTests(int argc,
                                 char** argv,
                                 base::RunTestSuiteCallback run_test_suite);

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_PUBLIC_CPP_TEST_COMMON_INITIALIZATION_H_
