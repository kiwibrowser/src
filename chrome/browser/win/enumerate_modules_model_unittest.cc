// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/win/enumerate_modules_model.h"

#include <stddef.h>

#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

typedef testing::Test EnumerateModulesTest;

// Set up some constants to use as default when creating the structs.
static const ModuleEnumerator::ModuleType kType =
    ModuleEnumerator::LOADED_MODULE;

static const ModuleEnumerator::ModuleStatus kStatus =
    ModuleEnumerator::NOT_MATCHED;

static const ModuleEnumerator::RecommendedAction kAction =
    ModuleEnumerator::NONE;

// This is a list of test cases to normalize.
static const struct NormalizationEntryList {
  ModuleEnumerator::Module test_case;
  ModuleEnumerator::Module expected;
} kNormalizationTestCases[] = {
  {
    // Only path normalization needed.
    {kType, kStatus, L"c:\\foo\\bar.dll", L"",        L"Prod", L"Desc", L"1.0",
         kAction},
    {kType, kStatus, L"c:\\foo\\",        L"bar.dll", L"Prod", L"Desc", L"1.0",
         kAction},
  }, {
    // Lower case normalization.
    {kType, kStatus, L"C:\\Foo\\Bar.dll", L"",        L"", L"", L"1.0",
         kAction},
    {kType, kStatus, L"c:\\foo\\",        L"bar.dll", L"", L"", L"1.0",
         kAction},
  }, {
    // Version can include strings after the version number. Strip that away.
    {kType, kStatus, L"c:\\foo.dll", L"",        L"", L"", L"1.0 asdf",
         kAction},
    {kType, kStatus, L"c:\\",        L"foo.dll", L"", L"", L"1.0",
         kAction},
  }, {
    // Commas instead of periods in version string.
    {kType, kStatus, L"", L"", L"", L"", L"1, 0", kAction},
    {kType, kStatus, L"", L"", L"", L"", L"1.0", kAction},
  }, {
    // Corner case: No path (not sure this will ever happen).
    {kType, kStatus, L"bar.dll", L"",        L"", L"", L"", kAction},
    {kType, kStatus, L"",        L"bar.dll", L"", L"", L"", kAction},
  }, {
    // Error case: Missing filename (not sure this will ever happen).
    {kType, kStatus, L"", L"", L"", L"", L"1.0", kAction},
    {kType, kStatus, L"", L"", L"", L"", L"1.0", kAction},
  },
};

TEST_F(EnumerateModulesTest, NormalizeEntry) {
  for (size_t i = 0; i < arraysize(kNormalizationTestCases); ++i) {
    ModuleEnumerator::Module test = kNormalizationTestCases[i].test_case;
    ModuleEnumerator::NormalizeModule(&test);
    ModuleEnumerator::Module expected = kNormalizationTestCases[i].expected;

    SCOPED_TRACE("Test case no: " + base::IntToString(i));
    EXPECT_EQ(expected.type, test.type);
    EXPECT_EQ(expected.status, test.status);
    EXPECT_STREQ(expected.location.c_str(), test.location.c_str());
    EXPECT_STREQ(expected.name.c_str(), test.name.c_str());
    EXPECT_STREQ(expected.product_name.c_str(), test.product_name.c_str());
    EXPECT_STREQ(expected.description.c_str(), test.description.c_str());
    EXPECT_STREQ(expected.version.c_str(), test.version.c_str());
    EXPECT_EQ(expected.recommended_action, test.recommended_action);
  }
}
