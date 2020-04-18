// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/translate/core/common/translate_util.h"

#include "base/command_line.h"
#include "components/translate/core/common/translate_switches.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

typedef testing::Test TranslateUtilTest;

// Tests that synonym language code is converted to one used in supporting list.
TEST_F(TranslateUtilTest, ToTranslateLanguageSynonym) {
  std::string language;

  language = std::string("nb");
  translate::ToTranslateLanguageSynonym(&language);
  EXPECT_EQ("no", language);

  // Test all known Chinese cases.
  language = std::string("zh-HK");
  translate::ToTranslateLanguageSynonym(&language);
  EXPECT_EQ("zh-TW", language);
  language = std::string("zh-MO");
  translate::ToTranslateLanguageSynonym(&language);
  EXPECT_EQ("zh-TW", language);
  language = std::string("zh-SG");
  translate::ToTranslateLanguageSynonym(&language);
  EXPECT_EQ("zh-CN", language);
  language = std::string("zh");
  translate::ToTranslateLanguageSynonym(&language);
  EXPECT_EQ("zh", language);

  // A sub code is not preserved (except for Chinese).
  language = std::string("he-IL");
  translate::ToTranslateLanguageSynonym(&language);
  EXPECT_EQ("iw", language);

  language = std::string("zh-JP");
  translate::ToTranslateLanguageSynonym(&language);
  EXPECT_EQ("zh-JP", language);

  // Preserve the argument if it doesn't have its synonym.
  language = std::string("en");
  translate::ToTranslateLanguageSynonym(&language);
  EXPECT_EQ("en", language);
}

// Tests that synonym language code is converted to one used in Chrome internal.
TEST_F(TranslateUtilTest, ToChromeLanguageSynonym) {
  std::string language;

  language = std::string("no");
  translate::ToChromeLanguageSynonym(&language);
  EXPECT_EQ("nb", language);

  // Preserve a sub code
  language = std::string("iw-IL");
  translate::ToChromeLanguageSynonym(&language);
  EXPECT_EQ("he-IL", language);

  // Preserve the argument if it doesn't have its synonym.
  language = std::string("en");
  translate::ToChromeLanguageSynonym(&language);
  EXPECT_EQ("en", language);
}

TEST_F(TranslateUtilTest, SecurityOrigin) {
  GURL origin = translate::GetTranslateSecurityOrigin();
  EXPECT_EQ(std::string(translate::kSecurityOrigin), origin.spec());

  const std::string running_origin("http://www.tamurayukari.com/");
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitchASCII(translate::switches::kTranslateSecurityOrigin,
                                  running_origin);
  GURL modified_origin = translate::GetTranslateSecurityOrigin();
  EXPECT_EQ(running_origin, modified_origin.spec());
}

