// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/visual_studio_utils.h"

#include "base/location.h"
#include "base/strings/string_util.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(VisualStudioUtils, MakeGuid) {
  std::string pattern = "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}";
  std::string guid = MakeGuid(__FILE__, "foo");
  ASSERT_EQ(pattern.size(), guid.size());
  for (size_t i = 0; i < pattern.size(); ++i) {
    if (pattern[i] == 'x')
      ASSERT_TRUE(base::IsAsciiAlpha(guid[i]) || base::IsAsciiDigit(guid[i]));
    else
      ASSERT_EQ(pattern[i], guid[i]);
  }

  // Calling function again should produce the same GUID.
  ASSERT_EQ(guid, MakeGuid(__FILE__, "foo"));

  // GUIDs should be different if path or seed is different.
  ASSERT_NE(guid, MakeGuid(std::string(__FILE__) + ".txt", "foo"));
  ASSERT_NE(guid, MakeGuid(__FILE__, "bar"));
}

TEST(VisualStudioUtils, ParseCompilerOption) {
  CompilerOptions options;
  ParseCompilerOption("/FIinclude.h", &options);
  ParseCompilerOption("/FIC:/path/file.h", &options);
  ASSERT_EQ("include.h;C:/path/file.h;", options.forced_include_files);

  CHECK(options.buffer_security_check.empty());
  ParseCompilerOption("/GS", &options);
  ASSERT_EQ("true", options.buffer_security_check);
  ParseCompilerOption("/GS-", &options);
  ASSERT_EQ("false", options.buffer_security_check);

  CHECK(options.runtime_library.empty());
  ParseCompilerOption("/MD", &options);
  ASSERT_EQ("MultiThreadedDLL", options.runtime_library);
  ParseCompilerOption("/MDd", &options);
  ASSERT_EQ("MultiThreadedDebugDLL", options.runtime_library);
  ParseCompilerOption("/MT", &options);
  ASSERT_EQ("MultiThreaded", options.runtime_library);
  ParseCompilerOption("/MTd", &options);
  ASSERT_EQ("MultiThreadedDebug", options.runtime_library);

  CHECK(options.optimization.empty());
  ParseCompilerOption("/O1", &options);
  ASSERT_EQ("MinSpace", options.optimization);
  ParseCompilerOption("/O2", &options);
  ASSERT_EQ("MaxSpeed", options.optimization);
  ParseCompilerOption("/Od", &options);
  ASSERT_EQ("Disabled", options.optimization);
  ParseCompilerOption("/Ox", &options);
  ASSERT_EQ("Full", options.optimization);

  CHECK(options.additional_options.empty());
  ParseCompilerOption("/TC", &options);
  ASSERT_TRUE(options.additional_options.empty());
  ParseCompilerOption("/TP", &options);
  ASSERT_TRUE(options.additional_options.empty());

  CHECK(options.warning_level.empty());
  ParseCompilerOption("/W0", &options);
  ASSERT_EQ("Level0", options.warning_level);
  ParseCompilerOption("/W1", &options);
  ASSERT_EQ("Level1", options.warning_level);
  ParseCompilerOption("/W2", &options);
  ASSERT_EQ("Level2", options.warning_level);
  ParseCompilerOption("/W3", &options);
  ASSERT_EQ("Level3", options.warning_level);
  ParseCompilerOption("/W4", &options);
  ASSERT_EQ("Level4", options.warning_level);

  CHECK(options.treat_warning_as_error.empty());
  ParseCompilerOption("/WX", &options);
  ASSERT_EQ("true", options.treat_warning_as_error);

  CHECK(options.disable_specific_warnings.empty());
  ParseCompilerOption("/wd1234", &options);
  ParseCompilerOption("/wd56", &options);
  ASSERT_EQ("1234;56;", options.disable_specific_warnings);

  CHECK(options.additional_options.empty());
  ParseCompilerOption("/MP", &options);
  ParseCompilerOption("/bigobj", &options);
  ParseCompilerOption("/Zc:sizedDealloc", &options);
  ASSERT_EQ("/MP /bigobj /Zc:sizedDealloc ", options.additional_options);
}

TEST(VisualStudioUtils, ParseLinkerOption) {
  LinkerOptions options;
  ParseLinkerOption("/SUBSYSTEM:CONSOLE,5.02h", &options);
  ASSERT_EQ("CONSOLE", options.subsystem);

  ParseLinkerOption("/SUBSYSTEM:WINDOWS", &options);
  ASSERT_EQ("WINDOWS", options.subsystem);
}
