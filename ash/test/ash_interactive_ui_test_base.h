// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_TEST_ASH_INTERACTIVE_UI_TEST_BASE_H_
#define ASH_TEST_ASH_INTERACTIVE_UI_TEST_BASE_H_

#include <memory>
#include <string>

#include "ash/test/ash_test_base.h"
#include "base/macros.h"

namespace aura {
class Env;
}

namespace ash {

class AshInteractiveUITestBase : public AshTestBase {
 public:
  AshInteractiveUITestBase();
  ~AshInteractiveUITestBase() override;

 protected:
  // testing::Test:
  void SetUp() override;
  void TearDown() override;

 private:
  std::unique_ptr<aura::Env> env_;

  DISALLOW_COPY_AND_ASSIGN(AshInteractiveUITestBase);
};

}  // namespace ash

#endif  // ASH_TEST_ASH_INTERACTIVE_UI_TEST_BASE_H_
