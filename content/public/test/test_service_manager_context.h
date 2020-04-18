// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_TEST_SERVICE_MANAGER_CONTEXT_H_
#define CONTENT_PUBLIC_TEST_TEST_SERVICE_MANAGER_CONTEXT_H_

#include <memory>

#include "base/macros.h"

namespace content {

class ServiceManagerContext;

// Helper class to expose the internal content::ServiceManagerContext type to
// non-browser unit tests which need to construct one.
class TestServiceManagerContext {
 public:
  TestServiceManagerContext();
  ~TestServiceManagerContext();

 private:
  std::unique_ptr<ServiceManagerContext> context_;

  DISALLOW_COPY_AND_ASSIGN(TestServiceManagerContext);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_TEST_SERVICE_MANAGER_CONTEXT_H_
