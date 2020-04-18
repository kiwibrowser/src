// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_TEST_AURA_MUS_TEST_BASE_H_
#define UI_AURA_TEST_AURA_MUS_TEST_BASE_H_

#include "base/macros.h"
#include "ui/aura/test/aura_test_base.h"

namespace aura {
namespace test {

// A base class for aura unit tests that use mus. You can also use AuraTestBase
// directly and call EnableMusWithTestWindowTree() before SetUp(). Prefer this
// if you don't need to subclass and want to target mus.
//
// This test class sets up the connection to mus as the window manager.
class AuraMusWmTestBase : public AuraTestBase {
 public:
  AuraMusWmTestBase();
  ~AuraMusWmTestBase() override;

  // AuraTestBase:
  void SetUp() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AuraMusWmTestBase);
};

// A base class for aura unit tests that use mus. You can also use AuraTestBase
// directly and call EnableMusWithTestWindowTree() before SetUp(). Prefer this
// if you don't need to subclass and want to target mus.
//
// This test class sets up the connection to mus as a normal client (not the
// window manager).
class AuraMusClientTestBase : public AuraTestBase {
 public:
  AuraMusClientTestBase();
  ~AuraMusClientTestBase() override;

  // AuraTestBase:
  void SetUp() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AuraMusClientTestBase);
};

}  // namespace test
}  // namespace aura

#endif  // UI_AURA_TEST_AURA_MUS_TEST_BASE_H_
