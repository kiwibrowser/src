// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/mixin_based_browser_test.h"

#include <utility>

#include "base/containers/adapters.h"

namespace chromeos {

MixinBasedBrowserTest::MixinBasedBrowserTest() : setup_was_launched_(false) {}

MixinBasedBrowserTest::~MixinBasedBrowserTest() {}

void MixinBasedBrowserTest::SetUpCommandLine(base::CommandLine* command_line) {
  setup_was_launched_ = true;
  for (const auto& mixin : mixins_)
    mixin->SetUpCommandLine(command_line);
}

void MixinBasedBrowserTest::SetUpInProcessBrowserTestFixture() {
  setup_was_launched_ = true;
  for (const auto& mixin : mixins_)
    mixin->SetUpInProcessBrowserTestFixture();
}

void MixinBasedBrowserTest::SetUpOnMainThread() {
  setup_was_launched_ = true;
  for (const auto& mixin : mixins_)
    mixin->SetUpOnMainThread();
}

void MixinBasedBrowserTest::TearDownOnMainThread() {
  for (const auto& mixin : base::Reversed(mixins_))
    mixin->TearDownInProcessBrowserTestFixture();
}

void MixinBasedBrowserTest::TearDownInProcessBrowserTestFixture() {
  for (const auto& mixin : base::Reversed(mixins_))
    mixin->TearDownInProcessBrowserTestFixture();
}

void MixinBasedBrowserTest::AddMixin(std::unique_ptr<Mixin> mixin) {
  CHECK(!setup_was_launched_)
      << "You are trying to add a mixin after setting up has already started.";
  mixins_.push_back(std::move(mixin));
}

}  // namespace chromeos
