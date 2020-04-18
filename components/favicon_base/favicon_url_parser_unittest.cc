// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/favicon_base/favicon_url_parser.h"

#include <memory>

#include "base/macros.h"
#include "components/favicon_base/favicon_types.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/layout.h"

class FaviconUrlParserTest : public testing::Test {
 public:
  FaviconUrlParserTest() {
    // Set the supported scale factors because the supported scale factors
    // affect the result of ParsePathAndScale().
    std::vector<ui::ScaleFactor> supported_scale_factors;
    supported_scale_factors.push_back(ui::SCALE_FACTOR_100P);
    supported_scale_factors.push_back(ui::SCALE_FACTOR_140P);
    scoped_set_supported_scale_factors_.reset(
        new ui::test::ScopedSetSupportedScaleFactors(supported_scale_factors));
  }

  ~FaviconUrlParserTest() override {}

 private:
  typedef std::unique_ptr<ui::test::ScopedSetSupportedScaleFactors>
      ScopedSetSupportedScaleFactors;
  ScopedSetSupportedScaleFactors scoped_set_supported_scale_factors_;

  DISALLOW_COPY_AND_ASSIGN(FaviconUrlParserTest);
};

// Test parsing path with no extra parameters.
TEST_F(FaviconUrlParserTest, ParsingNoExtraParams) {
  const std::string url("https://www.google.ca/imghp?hl=en&tab=wi");
  chrome::ParsedFaviconPath parsed;

  const std::string path1 = url;
  EXPECT_TRUE(chrome::ParseFaviconPath(path1, &parsed));
  EXPECT_FALSE(parsed.is_icon_url);
  EXPECT_EQ(url, parsed.url);
  EXPECT_EQ(16, parsed.size_in_dip);
  EXPECT_EQ(1.0f, parsed.device_scale_factor);
}

// Test parsing path with a 'size' parameter.
TEST_F(FaviconUrlParserTest, ParsingSizeParam) {
  const std::string url("https://www.google.ca/imghp?hl=en&tab=wi");
  chrome::ParsedFaviconPath parsed;

  // Test that we can still parse the legacy 'size' parameter format.
  const std::string path2 = "size/32/" + url;
  EXPECT_TRUE(chrome::ParseFaviconPath(path2, &parsed));
  EXPECT_FALSE(parsed.is_icon_url);
  EXPECT_EQ(url, parsed.url);
  EXPECT_EQ(32, parsed.size_in_dip);
  EXPECT_EQ(1.0f, parsed.device_scale_factor);

  // Test parsing current 'size' parameter format.
  const std::string path3 = "size/32@1.4x/" + url;
  EXPECT_TRUE(chrome::ParseFaviconPath(path3, &parsed));
  EXPECT_FALSE(parsed.is_icon_url);
  EXPECT_EQ(url, parsed.url);
  EXPECT_EQ(32, parsed.size_in_dip);
  EXPECT_EQ(1.4f, parsed.device_scale_factor);

  // Test that we pick the ui::ScaleFactor which is closest to the passed in
  // scale factor.
  const std::string path4 = "size/16@1.41x/" + url;
  EXPECT_TRUE(chrome::ParseFaviconPath(path4, &parsed));
  EXPECT_FALSE(parsed.is_icon_url);
  EXPECT_EQ(url, parsed.url);
  EXPECT_EQ(16, parsed.size_in_dip);
  EXPECT_EQ(1.41f, parsed.device_scale_factor);

  // Invalid cases.
  const std::string path5 = "size/" + url;
  EXPECT_FALSE(chrome::ParseFaviconPath(path5, &parsed));
  const std::string path6 = "size/@1x/" + url;
  EXPECT_FALSE(chrome::ParseFaviconPath(path6, &parsed));
  const std::string path7 = "size/abc@1x/" + url;
  EXPECT_FALSE(chrome::ParseFaviconPath(path7, &parsed));

  // Part of url looks like 'size' parameter.
  const std::string path8 = "http://www.google.com/size/32@1.4x";
  EXPECT_TRUE(chrome::ParseFaviconPath(path8, &parsed));
  EXPECT_FALSE(parsed.is_icon_url);
  EXPECT_EQ(path8, parsed.url);
  EXPECT_EQ(16, parsed.size_in_dip);
  EXPECT_EQ(1.0f, parsed.device_scale_factor);
}

// Test parsing path with 'iconurl' parameter.
TEST_F(FaviconUrlParserTest, ParsingIconUrlParam) {
  const std::string url("https://www.google.ca/imghp?hl=en&tab=wi");
  chrome::ParsedFaviconPath parsed;

  const std::string path10 = "iconurl/http://www.google.com/favicon.ico";
  EXPECT_TRUE(chrome::ParseFaviconPath(path10, &parsed));
  EXPECT_TRUE(parsed.is_icon_url);
  EXPECT_EQ("http://www.google.com/favicon.ico", parsed.url);
  EXPECT_EQ(16, parsed.size_in_dip);
  EXPECT_EQ(1.0f, parsed.device_scale_factor);
}

// Test parsing paths with both a 'size' parameter and a 'url modifier'
// parameter.
TEST_F(FaviconUrlParserTest, ParsingSizeParamAndUrlModifier) {
  const std::string url("https://www.google.ca/imghp?hl=en&tab=wi");
  chrome::ParsedFaviconPath parsed;

  const std::string path14 =
      "size/32/iconurl/http://www.google.com/favicon.ico";
  EXPECT_TRUE(chrome::ParseFaviconPath(path14, &parsed));
  EXPECT_TRUE(parsed.is_icon_url);
  EXPECT_EQ("http://www.google.com/favicon.ico", parsed.url);
  EXPECT_EQ(32, parsed.size_in_dip);
}
