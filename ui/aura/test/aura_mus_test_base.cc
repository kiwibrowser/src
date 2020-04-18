// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/test/aura_mus_test_base.h"

namespace aura {
namespace test {

AuraMusWmTestBase::AuraMusWmTestBase() {}

AuraMusWmTestBase::~AuraMusWmTestBase() {}

void AuraMusWmTestBase::SetUp() {
  // Run AuraTestBase::SetUp() first because it puts an InProcessContextFactory
  // in env.
  EnableMusWithTestWindowTree();
  AuraTestBase::SetUp();
}

AuraMusClientTestBase::AuraMusClientTestBase() {}

AuraMusClientTestBase::~AuraMusClientTestBase() {}

void AuraMusClientTestBase::SetUp() {
  // Run AuraTestBase::SetUp() first because it puts an InProcessContextFactory
  // in env.
  EnableMusWithTestWindowTree();
  set_window_manager_delegate(nullptr);
  AuraTestBase::SetUp();
}

}  // namespace test
}  // namespace aura
