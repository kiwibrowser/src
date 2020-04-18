// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_TEST_EXO_TEST_BASE_H_
#define COMPONENTS_EXO_TEST_EXO_TEST_BASE_H_

#include <memory>

#include "ash/test/ash_test_base.h"
#include "base/macros.h"

namespace exo {
class WMHelper;

namespace test {
class ExoTestHelper;

class ExoTestBase : public ash::AshTestBase {
 public:
  ExoTestBase();
  ~ExoTestBase() override;

  // Overridden from testing::Test:
  void SetUp() override;
  void TearDown() override;

  ExoTestHelper* exo_test_helper() { return exo_test_helper_.get(); }

 private:
  std::unique_ptr<ExoTestHelper> exo_test_helper_;
  std::unique_ptr<WMHelper> wm_helper_;

  DISALLOW_COPY_AND_ASSIGN(ExoTestBase);
};

}  // namespace test
}  // namespace exo

#endif  // COMPONENTS_EXO_TEST_EXO_TEST_BASE_H_
