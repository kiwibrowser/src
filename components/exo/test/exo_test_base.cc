// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/test/exo_test_base.h"

#include "base/command_line.h"
#include "components/exo/test/exo_test_helper.h"
#include "components/exo/test/test_client_controlled_state_delegate.h"
#include "components/exo/wm_helper.h"
#include "ui/wm/core/wm_core_switches.h"

namespace exo {
namespace test {

////////////////////////////////////////////////////////////////////////////////
// ExoTestBase, public:

ExoTestBase::ExoTestBase() : exo_test_helper_(new ExoTestHelper) {}

ExoTestBase::~ExoTestBase() {}

void ExoTestBase::SetUp() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  // Disable window animation when running tests.
  command_line->AppendSwitch(wm::switches::kWindowAnimationsDisabled);
  AshTestBase::SetUp();
  wm_helper_ = std::make_unique<WMHelper>();
  WMHelper::SetInstance(wm_helper_.get());
  test::TestClientControlledStateDelegate::InstallFactory();
}

void ExoTestBase::TearDown() {
  test::TestClientControlledStateDelegate::UninstallFactory();
  WMHelper::SetInstance(nullptr);
  wm_helper_.reset();
  AshTestBase::TearDown();
}

}  // namespace test
}  // namespace exo
