// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/core/common/locale_util.h"

#include "base/command_line.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

typedef testing::Test LocaleUtilTest;

TEST_F(LocaleUtilTest, SplitIntoMainAndTail) {
  std::string locale;
  std::string main;
  std::string tail;

  locale = "";
  language::SplitIntoMainAndTail(locale, &main, &tail);
  EXPECT_TRUE(main.empty());
  EXPECT_TRUE(tail.empty());

  locale = "en";
  main.clear();
  tail.clear();
  language::SplitIntoMainAndTail(locale, &main, &tail);
  EXPECT_EQ("en", main);
  EXPECT_TRUE(tail.empty());

  locale = "ogard543i";
  main.clear();
  tail.clear();
  language::SplitIntoMainAndTail(locale, &main, &tail);
  EXPECT_EQ("ogard543i", main);
  EXPECT_TRUE(tail.empty());

  locale = "en-AU";
  main.clear();
  tail.clear();
  language::SplitIntoMainAndTail(locale, &main, &tail);
  EXPECT_EQ("en", main);
  EXPECT_EQ("-AU", tail);

  locale = "es-419";
  main.clear();
  tail.clear();
  language::SplitIntoMainAndTail(locale, &main, &tail);
  EXPECT_EQ("es", main);
  EXPECT_EQ("-419", tail);

  locale = "en-AU-2009";
  main.clear();
  tail.clear();
  language::SplitIntoMainAndTail(locale, &main, &tail);
  EXPECT_EQ("en", main);
  EXPECT_EQ("-AU-2009", tail);
}

TEST_F(LocaleUtilTest, ContainsSameBaseLanguage) {
  std::vector<std::string> list;

  // Empty input.
  EXPECT_EQ(false, language::ContainsSameBaseLanguage(list, ""));

  // Empty list.
  EXPECT_EQ(false, language::ContainsSameBaseLanguage(list, "fr-FR"));

  // Empty language.
  list = {"en-US"};
  EXPECT_EQ(false, language::ContainsSameBaseLanguage(list, ""));

  // One element, no match.
  list = {"en-US"};
  EXPECT_EQ(false, language::ContainsSameBaseLanguage(list, "fr-FR"));

  // One element, with match.
  list = {"fr-CA"};
  EXPECT_EQ(true, language::ContainsSameBaseLanguage(list, "fr-FR"));

  // Multiple elements, no match.
  list = {"en-US", "es-AR", "en-UK"};
  EXPECT_EQ(false, language::ContainsSameBaseLanguage(list, "fr-FR"));

  // Multiple elements, with match.
  list = {"en-US", "fr-CA", "es-AR"};
  EXPECT_EQ(true, language::ContainsSameBaseLanguage(list, "fr-FR"));

  // Multiple elements matching.
  list = {"en-US", "fr-CA", "es-AR", "fr-FR"};
  EXPECT_EQ(true, language::ContainsSameBaseLanguage(list, "fr-FR"));

  // List includes base language.
  list = {"en-US", "fr", "es-AR", "fr-FR"};
  EXPECT_EQ(true, language::ContainsSameBaseLanguage(list, "fr-FR"));
}

TEST_F(LocaleUtilTest, ConvertToActualUILocale) {
  std::string locale;

  //---------------------------------------------------------------------------
  // Languages that are enabled as display UI.
  //---------------------------------------------------------------------------
  locale = "en-US";
  bool is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("en-US", locale);

  locale = "it";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("it", locale);

  locale = "fr-FR";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("fr-FR", locale);

  //---------------------------------------------------------------------------
  // Languages that are converted to their fallback version.
  //---------------------------------------------------------------------------

  // All Latin American Spanish languages fall back to "es-419".
  locale = "es-AR";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("es-419", locale);

  locale = "es-CL";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("es-419", locale);

  locale = "es-CO";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("es-419", locale);

  locale = "es-CR";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("es-419", locale);

  locale = "es-HN";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("es-419", locale);

  locale = "es-MX";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("es-419", locale);

  locale = "es-PE";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("es-419", locale);

  locale = "es-US";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("es-419", locale);

  locale = "es-UY";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("es-419", locale);

  locale = "es-VE";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("es-419", locale);

  // English falls back to US.
  locale = "en";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("en-US", locale);

  // All other regional English languages fall back to UK.
  locale = "en-AU";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("en-GB", locale);

  locale = "en-CA";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("en-GB", locale);

  locale = "en-IN";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("en-GB", locale);

  locale = "en-NZ";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("en-GB", locale);

  locale = "en-ZA";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("en-GB", locale);

  locale = "pt";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("pt-PT", locale);

  locale = "it-CH";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("it", locale);

  locale = "nn";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("nb", locale);

  locale = "no";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("nb", locale);

  //---------------------------------------------------------------------------
  // Languages that have their base language is a UI language.
  //---------------------------------------------------------------------------
  locale = "it-IT";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("it", locale);

  locale = "de-DE";
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_TRUE(is_ui);
  EXPECT_EQ("de", locale);

//---------------------------------------------------------------------------
// Languages that cannot be used as display UI.
//---------------------------------------------------------------------------
// This only matters for ChromeOS and Windows, as they are the only systems
// where users can set the display UI.
#if defined(OS_CHROMEOS) || defined(OS_WIN)
  locale = "sd";  // Sindhi
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_FALSE(is_ui);

  locale = "af";  // Afrikaans
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_FALSE(is_ui);

  locale = "ga";  // Irish
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_FALSE(is_ui);

  locale = "ky";  // Kyrgyz
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_FALSE(is_ui);

  locale = "zu";  // Zulu
  is_ui = language::ConvertToActualUILocale(&locale);
  EXPECT_FALSE(is_ui);
#endif
}
