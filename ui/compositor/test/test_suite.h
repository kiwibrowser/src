// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_COMPOSITOR_TEST_TEST_SUITE_H_
#define UI_COMPOSITOR_TEST_TEST_SUITE_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/test/test_suite.h"

namespace base {
namespace test {
class ScopedTaskEnvironment;
}
}

namespace ui {
namespace test {

class CompositorTestSuite : public base::TestSuite {
 public:
  CompositorTestSuite(int argc, char** argv);
  ~CompositorTestSuite() override;

 protected:
  // Overridden from base::TestSuite:
  void Initialize() override;
  void Shutdown() override;

 private:
  std::unique_ptr<base::test::ScopedTaskEnvironment> scoped_task_environment_;

  DISALLOW_COPY_AND_ASSIGN(CompositorTestSuite);
};

}  // namespace test
}  // namespace ui

#endif  // UI_COMPOSITOR_TEST_TEST_SUITE_H_
