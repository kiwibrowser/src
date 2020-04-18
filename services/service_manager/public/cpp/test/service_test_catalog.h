// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_PUBLIC_CPP_TEST_SERVICE_TEST_CATALOG_H_
#define SERVICES_SERVICE_MANAGER_PUBLIC_CPP_TEST_SERVICE_TEST_CATALOG_H_

#include <memory>

namespace base {
class Value;
}

namespace service_manager {
namespace test {

// This function must be defined by any target linking against the
// ":run_all_service_tests" target in this directory. Use the service_test
// GN template defined in
// src/services/service_manager/public/tools/test/service_test.gni to
// autogenerate and link against a definition of the function generated from the
// contents of a catalog manifest. See the service_test.gni documentation for
// more details.
std::unique_ptr<base::Value> CreateTestCatalog();

}  // namespace test
}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_PUBLIC_CPP_TEST_SERVICE_TEST_CATALOG_H_
