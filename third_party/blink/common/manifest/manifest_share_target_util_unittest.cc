// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <utility>

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/manifest/manifest_share_target_util.h"
#include "url/gurl.h"

namespace blink {
namespace {

constexpr char kTitle[] = "My title";
constexpr char kText[] = "My text";
constexpr char kUrlSpec[] = "https://www.google.com/";

}  // namespace

TEST(ManifestShareTargetUtilTest, ReplaceUrlPlaceholdersInvalidTemplate) {
  // Badly nested placeholders.
  GURL url_template = GURL("http://example.com/?q={");
  EXPECT_FALSE(ValidateWebShareUrlTemplate(url_template));
  EXPECT_FALSE(
      ReplaceWebShareUrlPlaceholders(url_template, "", "", GURL(), nullptr));

  url_template = GURL("http://example.com/?q={title");
  EXPECT_FALSE(ValidateWebShareUrlTemplate(url_template));
  EXPECT_FALSE(
      ReplaceWebShareUrlPlaceholders(url_template, "", "", GURL(), nullptr));

  url_template = GURL("http://example.com/?q={title{text}}");
  EXPECT_FALSE(ValidateWebShareUrlTemplate(url_template));
  EXPECT_FALSE(
      ReplaceWebShareUrlPlaceholders(url_template, "", "", GURL(), nullptr));

  url_template = GURL("http://example.com/?q={title{}");
  EXPECT_FALSE(ValidateWebShareUrlTemplate(url_template));
  EXPECT_FALSE(
      ReplaceWebShareUrlPlaceholders(url_template, "", "", GURL(), nullptr));

  url_template = GURL("http://example.com/?q={{title}}");
  EXPECT_FALSE(ValidateWebShareUrlTemplate(url_template));
  EXPECT_FALSE(
      ReplaceWebShareUrlPlaceholders(url_template, "", "", GURL(), nullptr));

  // Placeholder with non-identifier character.
  url_template = GURL("http://example.com/?q={title?}");
  EXPECT_FALSE(ValidateWebShareUrlTemplate(url_template));
  EXPECT_FALSE(
      ReplaceWebShareUrlPlaceholders(url_template, "", "", GURL(), nullptr));

  // Placeholder with digit character.
  url_template = GURL("http://example.com/?q={title1}");
  EXPECT_TRUE(ValidateWebShareUrlTemplate(url_template));
  EXPECT_TRUE(
      ReplaceWebShareUrlPlaceholders(url_template, "", "", GURL(), nullptr));

  // Empty placeholder.
  url_template = GURL("http://example.com/?q={}");
  EXPECT_TRUE(ValidateWebShareUrlTemplate(url_template));
  EXPECT_TRUE(
      ReplaceWebShareUrlPlaceholders(url_template, "", "", GURL(), nullptr));

  // Invalid placeholder in URL fragment.
  url_template = GURL("http://example.com/#{title?}");
  EXPECT_FALSE(ValidateWebShareUrlTemplate(url_template));
  EXPECT_FALSE(
      ReplaceWebShareUrlPlaceholders(url_template, "", "", GURL(), nullptr));

  // { in path.
  url_template = GURL("http://example.com/subpath{/");
  EXPECT_TRUE(ValidateWebShareUrlTemplate(url_template));
  EXPECT_TRUE(
      ReplaceWebShareUrlPlaceholders(url_template, "", "", GURL(), nullptr));

  // Invalid placeholder. Non-empty title, text, share URL and non-empty output
  // parameter.
  GURL url_template_filled;
  url_template = GURL("http://example.com/?q={");
  EXPECT_FALSE(ReplaceWebShareUrlPlaceholders(url_template, "text", "title",
                                              GURL("http://www.google.com"),
                                              &url_template_filled));
}

TEST(ManifestShareTargetUtilTest, ReplaceWebShareUrlPlaceholders) {
  const GURL kUrl(kUrlSpec);

  // No placeholders.
  GURL url_template = GURL("http://example.com/?q=a#a");
  GURL url_template_filled;
  bool succeeded = ReplaceWebShareUrlPlaceholders(url_template, kTitle, kText,
                                                  kUrl, &url_template_filled);
  EXPECT_TRUE(succeeded);
  EXPECT_EQ(url_template, url_template_filled);

  // One title placeholder.
  url_template = GURL("http://example.com/#{title}");
  succeeded = ReplaceWebShareUrlPlaceholders(url_template, kTitle, kText, kUrl,
                                             &url_template_filled);
  EXPECT_TRUE(succeeded);
  EXPECT_EQ("http://example.com/#My%20title", url_template_filled.spec());

  // One text placeholder.
  url_template = GURL("http://example.com/#{text}");
  succeeded = ReplaceWebShareUrlPlaceholders(url_template, kTitle, kText, kUrl,
                                             &url_template_filled);
  EXPECT_TRUE(succeeded);
  EXPECT_EQ("http://example.com/#My%20text", url_template_filled.spec());

  // One url placeholder.
  url_template = GURL("http://example.com/#{url}");
  succeeded = ReplaceWebShareUrlPlaceholders(url_template, kTitle, kText, kUrl,
                                             &url_template_filled);
  EXPECT_TRUE(succeeded);
  EXPECT_EQ("http://example.com/#https%3A%2F%2Fwww.google.com%2F",
            url_template_filled.spec());

  // One of each placeholder, in title, text, url order.
  url_template = GURL("http://example.com/#{title}{text}{url}");
  succeeded = ReplaceWebShareUrlPlaceholders(url_template, kTitle, kText, kUrl,
                                             &url_template_filled);
  EXPECT_TRUE(succeeded);
  EXPECT_EQ(
      "http://example.com/#My%20titleMy%20texthttps%3A%2F%2Fwww.google.com%2F",
      url_template_filled.spec());

  // One of each placeholder, in url, text, title order.
  url_template = GURL("http://example.com/#{url}{text}{title}");
  succeeded = ReplaceWebShareUrlPlaceholders(url_template, kTitle, kText, kUrl,
                                             &url_template_filled);
  EXPECT_TRUE(succeeded);
  EXPECT_EQ(
      "http://example.com/#https%3A%2F%2Fwww.google.com%2FMy%20textMy%20title",
      url_template_filled.spec());

  // Two of each placeholder, some next to each other, others not.
  url_template =
      GURL("http://example.com/#{title}{url}{text}{text}{title}{url}");
  succeeded = ReplaceWebShareUrlPlaceholders(url_template, kTitle, kText, kUrl,
                                             &url_template_filled);
  EXPECT_TRUE(succeeded);
  EXPECT_EQ(
      "http://example.com/"
      "#My%20titlehttps%3A%2F%2Fwww.google.com%2FMy%20textMy%20textMy%"
      "20titlehttps%3A%2F%2Fwww.google.com%2F",
      url_template_filled.spec());

  // Placeholders are in a query string, as values. The expected use case.
  // Two of each placeholder, some next to each other, others not.
  url_template = GURL(
      "http://example.com?title={title}&url={url}&text={text}&text={text}&"
      "title={title}&url={url}");
  succeeded = ReplaceWebShareUrlPlaceholders(url_template, kTitle, kText, kUrl,
                                             &url_template_filled);
  EXPECT_TRUE(succeeded);
  EXPECT_EQ(
      "http://"
      "example.com/?title=My%20title&url=https%3A%2F%2Fwww.google.com%2F&"
      "text=My%20text&"
      "text=My%20text&title=My%20title&url=https%3A%2F%2Fwww.google.com%2F",
      url_template_filled.spec());

  // Empty placeholder.
  url_template = GURL("http://example.com/#{}");
  succeeded = ReplaceWebShareUrlPlaceholders(url_template, kTitle, kText, kUrl,
                                             &url_template_filled);
  EXPECT_TRUE(succeeded);
  EXPECT_EQ("http://example.com/#", url_template_filled.spec());

  // Unexpected placeholders.
  url_template = GURL("http://example.com/#{nonexistentplaceholder}");
  succeeded = ReplaceWebShareUrlPlaceholders(url_template, kTitle, kText, kUrl,
                                             &url_template_filled);
  EXPECT_TRUE(succeeded);
  EXPECT_EQ("http://example.com/#", url_template_filled.spec());

  // Placeholders should only be replaced in query and fragment.
  url_template = GURL("http://example.com/subpath{title}/?q={title}#{title}");
  succeeded = ReplaceWebShareUrlPlaceholders(url_template, kTitle, kText, kUrl,
                                             &url_template_filled);
  EXPECT_TRUE(succeeded);
  EXPECT_EQ("http://example.com/subpath%7Btitle%7D/?q=My%20title#My%20title",
            url_template_filled.spec());

  // Braces in the path, which would be invalid, but should parse fine as they
  // are escaped.
  url_template = GURL("http://example.com/subpath{/?q={title}");
  succeeded = ReplaceWebShareUrlPlaceholders(url_template, kTitle, kText, kUrl,
                                             &url_template_filled);
  EXPECT_TRUE(succeeded);
  EXPECT_EQ("http://example.com/subpath%7B/?q=My%20title",
            url_template_filled.spec());

  // |url_template| with % escapes.
  url_template = GURL("http://example.com#%20{title}%20");
  succeeded = ReplaceWebShareUrlPlaceholders(url_template, kTitle, kText, kUrl,
                                             &url_template_filled);
  EXPECT_TRUE(succeeded);
  EXPECT_EQ("http://example.com/#%20My%20title%20", url_template_filled.spec());
}

// Test URL escaping done by ReplaceWebShareUrlPlaceholders().
TEST(ManifestShareTargetUtilTest, ReplaceWebShareUrlPlaceholders_Escaping) {
  const GURL kUrl(kUrlSpec);
  const GURL kUrlTemplate("http://example.com/#{title}");

  // Share data that contains percent escapes.
  GURL url_template_filled;
  bool succeeded = ReplaceWebShareUrlPlaceholders(
      kUrlTemplate, "My%20title", kText, kUrl, &url_template_filled);
  EXPECT_TRUE(succeeded);
  EXPECT_EQ("http://example.com/#My%2520title", url_template_filled.spec());

  // Share data that contains placeholders. These should not be replaced.
  succeeded = ReplaceWebShareUrlPlaceholders(kUrlTemplate, "{title}", kText,
                                             kUrl, &url_template_filled);
  EXPECT_TRUE(succeeded);
  EXPECT_EQ("http://example.com/#%7Btitle%7D", url_template_filled.spec());

  // All characters that shouldn't be escaped.
  succeeded = ReplaceWebShareUrlPlaceholders(kUrlTemplate,
                                             "-_.!~*'()0123456789"
                                             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                             "abcdefghijklmnopqrstuvwxyz",
                                             kText, kUrl, &url_template_filled);
  EXPECT_TRUE(succeeded);
  EXPECT_EQ(
      "http://example.com/#-_.!~*'()0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz",
      url_template_filled.spec());

  // All characters that should be escaped.
  succeeded =
      ReplaceWebShareUrlPlaceholders(kUrlTemplate, " \"#$%&+,/:;<=>?@[\\]^`{|}",
                                     kText, kUrl, &url_template_filled);
  EXPECT_TRUE(succeeded);
  EXPECT_EQ(
      "http://example.com/"
      "#%20%22%23%24%25%26%2B%2C%2F%3A%3B%3C%3D%3E%3F%40%5B%5C%5D%5E%60%7B%7C%"
      "7D",
      url_template_filled.spec());

  // Unicode chars.
  // U+263B
  succeeded = ReplaceWebShareUrlPlaceholders(kUrlTemplate, "\xe2\x98\xbb",
                                             kText, kUrl, &url_template_filled);
  EXPECT_TRUE(succeeded);
  EXPECT_EQ("http://example.com/#%E2%98%BB", url_template_filled.spec());

  // U+00E9
  succeeded = ReplaceWebShareUrlPlaceholders(kUrlTemplate, "\xc3\xa9", kText,
                                             kUrl, &url_template_filled);
  EXPECT_TRUE(succeeded);
  EXPECT_EQ("http://example.com/#%C3%A9", url_template_filled.spec());

  // U+1F4A9
  succeeded = ReplaceWebShareUrlPlaceholders(kUrlTemplate, "\xf0\x9f\x92\xa9",
                                             kText, kUrl, &url_template_filled);
  EXPECT_TRUE(succeeded);
  EXPECT_EQ("http://example.com/#%F0%9F%92%A9", url_template_filled.spec());
}

}  // namespace blink
