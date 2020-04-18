// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/install_verification/win/module_ids.h"

#include "base/strings/string_piece.h"
#include "chrome/browser/install_verification/win/module_verification_common.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(ModuleIDsTest, LoadModuleIDs) {
  ModuleIDs module_ids;
  LoadModuleIDs(&module_ids);
  ASSERT_EQ(1u, module_ids[CalculateModuleNameDigest(L"chrome.dll")]);
}

TEST(ModuleIDsTest, ParseAdditionalModuleIDs) {
  ModuleIDs module_ids;

  ParseAdditionalModuleIDs(base::StringPiece(), &module_ids);
  ASSERT_EQ(0u, module_ids.size());

  // Invalid input.
  ParseAdditionalModuleIDs("hello", &module_ids);
  ASSERT_EQ(0u, module_ids.size());
  ParseAdditionalModuleIDs("hello world", &module_ids);
  ASSERT_EQ(0u, module_ids.size());
  ParseAdditionalModuleIDs("hello world\nfoo bar", &module_ids);
  ASSERT_EQ(0u, module_ids.size());
  ParseAdditionalModuleIDs("123 world\nfoo bar", &module_ids);
  ASSERT_EQ(0u, module_ids.size());
  ParseAdditionalModuleIDs("hello 0123456789abcdef0123456789abcdef\nfoo bar",
                           &module_ids);
  ASSERT_EQ(0u, module_ids.size());

  // A single valid line followed by an invalid line.
  ParseAdditionalModuleIDs("123 0123456789abcdef0123456789abcdef\nfoo bar",
                         &module_ids);
  ASSERT_EQ(1u, module_ids.size());
  ASSERT_EQ(123u, module_ids.begin()->second);
  ASSERT_EQ("0123456789abcdef0123456789abcdef", module_ids.begin()->first);
  module_ids.clear();

  // The same, but with \r\n.
  ParseAdditionalModuleIDs("123 0123456789abcdef0123456789abcdef\r\nfoo bar",
                           &module_ids);
  ASSERT_EQ(1u, module_ids.size());
  ASSERT_EQ(123u, module_ids.begin()->second);
  ASSERT_EQ("0123456789abcdef0123456789abcdef", module_ids.begin()->first);
  module_ids.clear();

  // Several valid and invalid lines, with varying line terminations etc.
  ParseAdditionalModuleIDs("123 0123456789abcdef0123456789abcdef\r\n"
                           "456 DEADBEEFDEADBEEF\r"
                           "789 DEADBEEFDEADBEEFDEADBEEFDEADBEEF\n"
                           "\n"
                           "\n"
                           "321 BAADCAFEBAADCAFEBAADCAFEBAADCAFE\n",
                           &module_ids);
  ASSERT_EQ(3u, module_ids.size());
  ASSERT_EQ(123u, module_ids["0123456789abcdef0123456789abcdef"]);
  ASSERT_EQ(789u, module_ids["DEADBEEFDEADBEEFDEADBEEFDEADBEEF"]);
  ASSERT_EQ(321u, module_ids["BAADCAFEBAADCAFEBAADCAFEBAADCAFE"]);
  module_ids.clear();

  // Leading empty lines, no termination on final line.
  ParseAdditionalModuleIDs("\n"
                           "123 0123456789abcdef0123456789abcdef\r\n"
                           "321 BAADCAFEBAADCAFEBAADCAFEBAADCAFE",
                           &module_ids);
  ASSERT_EQ(2u, module_ids.size());
  ASSERT_EQ(123u, module_ids["0123456789abcdef0123456789abcdef"]);
  ASSERT_EQ(321u, module_ids["BAADCAFEBAADCAFEBAADCAFEBAADCAFE"]);
  module_ids.clear();
}
