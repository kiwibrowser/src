// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/test/test_browser_ui.h"

#include "base/command_line.h"
#include "base/test/gtest_util.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_switches.h"
#include "build/build_config.h"
#include "chrome/common/chrome_features.h"
#include "ui/base/ui_base_features.h"

namespace {

// Extracts the |name| argument for ShowUi() from the current test case name.
// E.g. for InvokeUi_name (or DISABLED_InvokeUi_name) returns "name".
std::string NameFromTestCase() {
  const std::string name = base::TestNameWithoutDisabledPrefix(
      testing::UnitTest::GetInstance()->current_test_info()->name());
  size_t underscore = name.find('_');
  return underscore == std::string::npos ? std::string()
                                         : name.substr(underscore + 1);
}

}  // namespace

TestBrowserUi::TestBrowserUi() = default;
TestBrowserUi::~TestBrowserUi() = default;

void TestBrowserUi::ShowAndVerifyUi() {
  PreShow();
  ShowUi(NameFromTestCase());
  ASSERT_TRUE(VerifyUi());
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kTestLauncherInteractive))
    WaitForUserDismissal();
  else
    DismissUi();
}

void TestBrowserUi::UseMdOnly() {
  if (enable_md_)
    return;

  enable_md_ = std::make_unique<base::test::ScopedFeatureList>();
  enable_md_->InitWithFeatures(
#if defined(OS_MACOSX)
      {features::kSecondaryUiMd, features::kShowAllDialogsWithViewsToolkit},
#else
      {features::kSecondaryUiMd},
#endif
      {});
}
