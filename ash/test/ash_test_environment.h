// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_TEST_ASH_TEST_ENVIRONMENT_H_
#define ASH_TEST_ASH_TEST_ENVIRONMENT_H_

#include <memory>
#include <string>

namespace ash {

class AshTestViewsDelegate;

// AshTestEnvironment creates objects specific to an environment. Two
// environments are provided, one for content (AshTestEnvironmentContent)
// and one without content (AshTestEnvironmentDefault).
//
// AshTestBase creates an AshTestEnvironment by way of
// AshTestEnvironment::Create(). The implementation of Create() depends upon
// the ash target that was linked against: //ash:test_support_with_content
// includes AshTestEnvironmentContent and
// //ash:test_support_without_content includes AshTestEnvironmentDefault.
class AshTestEnvironment {
 public:
  virtual ~AshTestEnvironment() {}

  // Creates the object appropriate to the current environment.
  static std::unique_ptr<AshTestEnvironment> Create();

  // Returns the ASCII file name of where the 100% resources are stored.
  static std::string Get100PercentResourceFileName();

  // Called from AshTestHelper::SetUp()/TearDown().
  virtual void SetUp() {}
  virtual void TearDown() {}

  virtual std::unique_ptr<AshTestViewsDelegate> CreateViewsDelegate() = 0;

 protected:
  AshTestEnvironment() {}
};

}  // namespace ash

#endif  // ASH_TEST_ASH_TEST_ENVIRONMENT_H_
