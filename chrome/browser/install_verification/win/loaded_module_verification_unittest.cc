// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/install_verification/win/loaded_module_verification.h"

#include "chrome/browser/install_verification/win/module_verification_test.h"
#include "testing/gtest/include/gtest/gtest.h"

class LoadedModuleVerificationTest : public ModuleVerificationTest { };

TEST_F(LoadedModuleVerificationTest, TestCase) {
  std::set<ModuleInfo> loaded_modules;
  ModuleIDs empty_modules_of_interest;
  ModuleIDs non_matching_modules_of_interest;
  ModuleIDs matching_modules_of_interest;

  matching_modules_of_interest.insert(
      std::make_pair(CalculateModuleNameDigest(L"fancy_pants.dll"), 999u));
  matching_modules_of_interest.insert(
      std::make_pair(CalculateModuleNameDigest(L"advapi32.dll"), 666u));
  matching_modules_of_interest.insert(
      std::make_pair(CalculateModuleNameDigest(L"unit_tests.exe"), 777u));
  matching_modules_of_interest.insert(
      std::make_pair(CalculateModuleNameDigest(L"user32.dll"), 888u));

  non_matching_modules_of_interest.insert(
      std::make_pair(CalculateModuleNameDigest(L"fancy_pants.dll"), 999u));

  // With empty loaded_modules, nothing matches.
  VerifyLoadedModules(loaded_modules,
                      empty_modules_of_interest,
                      &ModuleVerificationTest::ReportModule);
  ASSERT_TRUE(reported_module_ids_.empty());
  VerifyLoadedModules(loaded_modules,
                      non_matching_modules_of_interest,
                      &ModuleVerificationTest::ReportModule);
  ASSERT_TRUE(reported_module_ids_.empty());
  VerifyLoadedModules(loaded_modules,
                      matching_modules_of_interest,
                      &ModuleVerificationTest::ReportModule);
  ASSERT_TRUE(reported_module_ids_.empty());

  // With populated loaded_modules, only the 'matching' module data gives a
  // match.
  ASSERT_TRUE(GetLoadedModuleInfoSet(&loaded_modules));
  VerifyLoadedModules(loaded_modules,
                      empty_modules_of_interest,
                      &ModuleVerificationTest::ReportModule);
  ASSERT_TRUE(reported_module_ids_.empty());
  VerifyLoadedModules(loaded_modules,
                      non_matching_modules_of_interest,
                      &ModuleVerificationTest::ReportModule);
  ASSERT_TRUE(reported_module_ids_.empty());
  VerifyLoadedModules(loaded_modules,
                      matching_modules_of_interest,
                      &ModuleVerificationTest::ReportModule);
  ASSERT_EQ(3u, reported_module_ids_.size());
  ASSERT_NE(reported_module_ids_.end(), reported_module_ids_.find(666u));
  ASSERT_NE(reported_module_ids_.end(), reported_module_ids_.find(777u));
  ASSERT_NE(reported_module_ids_.end(), reported_module_ids_.find(888u));
}
