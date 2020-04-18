// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/macros.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/err.h"
#include "tools/gn/label.h"
#include "tools/gn/value.h"

namespace {

struct ParseDepStringCase {
  const char* cur_dir;
  const char* str;
  bool success;
  const char* expected_dir;
  const char* expected_name;
  const char* expected_toolchain_dir;
  const char* expected_toolchain_name;
};

}  // namespace

TEST(Label, Resolve) {
  ParseDepStringCase cases[] = {
    {"//chrome/", "", false, "", "", "", ""},
    {"//chrome/", "/", false, "", "", "", ""},
    {"//chrome/", ":", false, "", "", "", ""},
    {"//chrome/", "/:", false, "", "", "", ""},
    {"//chrome/", "blah", true, "//chrome/blah/", "blah", "//t/", "d"},
    {"//chrome/", "blah:bar", true, "//chrome/blah/", "bar", "//t/", "d"},
    // Absolute paths.
    {"//chrome/", "/chrome:bar", true, "/chrome/", "bar", "//t/", "d"},
    {"//chrome/", "/chrome/:bar", true, "/chrome/", "bar", "//t/", "d"},
#if defined(OS_WIN)
    {"//chrome/", "/C:/chrome:bar", true, "/C:/chrome/", "bar", "//t/", "d"},
    {"//chrome/", "/C:/chrome/:bar", true, "/C:/chrome/", "bar", "//t/", "d"},
    {"//chrome/", "C:/chrome:bar", true, "/C:/chrome/", "bar", "//t/", "d"},
#endif
    // Refers to root dir.
    {"//chrome/", "//:bar", true, "//", "bar", "//t/", "d"},
    // Implicit directory
    {"//chrome/", ":bar", true, "//chrome/", "bar", "//t/", "d"},
    {"//chrome/renderer/", ":bar", true, "//chrome/renderer/", "bar", "//t/",
     "d"},
    // Implicit names.
    {"//chrome/", "//base", true, "//base/", "base", "//t/", "d"},
    {"//chrome/", "//base/i18n", true, "//base/i18n/", "i18n", "//t/", "d"},
    {"//chrome/", "//base/i18n:foo", true, "//base/i18n/", "foo", "//t/", "d"},
    {"//chrome/", "//", false, "", "", "", ""},
    // Toolchain parsing.
    {"//chrome/", "//chrome:bar(//t:n)", true, "//chrome/", "bar", "//t/", "n"},
    {"//chrome/", "//chrome:bar(//t)", true, "//chrome/", "bar", "//t/", "t"},
    {"//chrome/", "//chrome:bar(//t:)", true, "//chrome/", "bar", "//t/", "t"},
    {"//chrome/", "//chrome:bar()", true, "//chrome/", "bar", "//t/", "d"},
    {"//chrome/", "//chrome:bar(foo)", true, "//chrome/", "bar",
     "//chrome/foo/", "foo"},
    {"//chrome/", "//chrome:bar(:foo)", true, "//chrome/", "bar", "//chrome/",
     "foo"},
    // TODO(brettw) it might be nice to make this an error:
    //{"//chrome/", "//chrome:bar())", false, "", "", "", "" },
    {"//chrome/", "//chrome:bar(//t:bar(tc))", false, "", "", "", ""},
    {"//chrome/", "//chrome:bar(()", false, "", "", "", ""},
    {"//chrome/", "(t:b)", false, "", "", "", ""},
    {"//chrome/", ":bar(//t/b)", true, "//chrome/", "bar", "//t/b/", "b"},
    {"//chrome/", ":bar(/t/b)", true, "//chrome/", "bar", "/t/b/", "b"},
    {"//chrome/", ":bar(t/b)", true, "//chrome/", "bar", "//chrome/t/b/", "b"},
  };

  Label default_toolchain(SourceDir("//t/"), "d");

  for (size_t i = 0; i < arraysize(cases); i++) {
    const ParseDepStringCase& cur = cases[i];

    std::string location, name;
    Err err;
    Value v(nullptr, Value::STRING);
    v.string_value() = cur.str;
    Label result =
        Label::Resolve(SourceDir(cur.cur_dir), default_toolchain, v, &err);
    EXPECT_EQ(cur.success, !err.has_error()) << i << " " << cur.str;
    if (!err.has_error() && cur.success) {
      EXPECT_EQ(cur.expected_dir, result.dir().value()) << i << " " << cur.str;
      EXPECT_EQ(cur.expected_name, result.name()) << i << " " << cur.str;
      EXPECT_EQ(cur.expected_toolchain_dir, result.toolchain_dir().value())
          << i << " " << cur.str;
      EXPECT_EQ(cur.expected_toolchain_name, result.toolchain_name())
          << i << " " << cur.str;
    }
  }
}
