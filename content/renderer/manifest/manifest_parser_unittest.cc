// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/manifest/manifest_parser.h"

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "base/optional.h"
#include "base/strings/string_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/manifest/manifest.h"

namespace content {

class ManifestParserTest : public testing::Test  {
 protected:
  ManifestParserTest() {}
  ~ManifestParserTest() override {}

  blink::Manifest ParseManifestWithURLs(const base::StringPiece& data,
                                        const GURL& manifest_url,
                                        const GURL& document_url) {
    ManifestParser parser(data, manifest_url, document_url);
    parser.Parse();
    std::vector<blink::mojom::ManifestErrorPtr> errors;
    parser.TakeErrors(&errors);

    errors_.clear();
    for (auto& error : errors)
      errors_.push_back(std::move(error->message));
    return parser.manifest();
  }

  blink::Manifest ParseManifest(const base::StringPiece& data) {
    return ParseManifestWithURLs(
        data, default_manifest_url, default_document_url);
  }

  const std::vector<std::string>& errors() const {
    return errors_;
  }

  unsigned int GetErrorCount() const {
    return errors_.size();
  }

  static const GURL default_document_url;
  static const GURL default_manifest_url;

 private:
  std::vector<std::string> errors_;

  DISALLOW_COPY_AND_ASSIGN(ManifestParserTest);
};

const GURL ManifestParserTest::default_document_url(
    "http://foo.com/index.html");
const GURL ManifestParserTest::default_manifest_url(
    "http://foo.com/manifest.json");

TEST_F(ManifestParserTest, CrashTest) {
  // Passing temporary variables should not crash.
  const base::StringPiece json = "{\"start_url\": \"/\"}";
  GURL url("http://example.com");
  ManifestParser parser(json, url, url);
  parser.Parse();
  std::vector<blink::mojom::ManifestErrorPtr> errors;
  parser.TakeErrors(&errors);

  // .Parse() should have been call without crashing and succeeded.
  EXPECT_EQ(0u, errors.size());
  EXPECT_FALSE(parser.manifest().IsEmpty());
}

TEST_F(ManifestParserTest, EmptyStringNull) {
  blink::Manifest manifest = ParseManifest("");

  // This Manifest is not a valid JSON object, it's a parsing error.
  EXPECT_EQ(1u, GetErrorCount());
  EXPECT_EQ("Line: 1, column: 1, Unexpected token.",
            errors()[0]);

  // A parsing error is equivalent to an empty manifest.
  ASSERT_TRUE(manifest.IsEmpty());
  ASSERT_TRUE(manifest.name.is_null());
  ASSERT_TRUE(manifest.short_name.is_null());
  ASSERT_TRUE(manifest.start_url.is_empty());
  ASSERT_EQ(manifest.display, blink::kWebDisplayModeUndefined);
  ASSERT_EQ(manifest.orientation, blink::kWebScreenOrientationLockDefault);
  ASSERT_FALSE(manifest.theme_color.has_value());
  ASSERT_FALSE(manifest.background_color.has_value());
  ASSERT_TRUE(manifest.splash_screen_url.is_empty());
  ASSERT_TRUE(manifest.gcm_sender_id.is_null());
  ASSERT_TRUE(manifest.scope.is_empty());
}

TEST_F(ManifestParserTest, ValidNoContentParses) {
  blink::Manifest manifest = ParseManifest("{}");

  // Empty Manifest is not a parsing error.
  EXPECT_EQ(0u, GetErrorCount());

  // Check that all the fields are null in that case.
  ASSERT_TRUE(manifest.IsEmpty());
  ASSERT_TRUE(manifest.name.is_null());
  ASSERT_TRUE(manifest.short_name.is_null());
  ASSERT_TRUE(manifest.start_url.is_empty());
  ASSERT_EQ(manifest.display, blink::kWebDisplayModeUndefined);
  ASSERT_EQ(manifest.orientation, blink::kWebScreenOrientationLockDefault);
  ASSERT_FALSE(manifest.theme_color.has_value());
  ASSERT_FALSE(manifest.background_color.has_value());
  ASSERT_TRUE(manifest.splash_screen_url.is_empty());
  ASSERT_TRUE(manifest.gcm_sender_id.is_null());
  ASSERT_TRUE(manifest.scope.is_empty());
}

TEST_F(ManifestParserTest, MultipleErrorsReporting) {
  blink::Manifest manifest = ParseManifest(
      "{ \"name\": 42, \"short_name\": 4,"
      "\"orientation\": {}, \"display\": \"foo\","
      "\"start_url\": null, \"icons\": {}, \"theme_color\": 42,"
      "\"background_color\": 42 }");

  EXPECT_EQ(8u, GetErrorCount());

  EXPECT_EQ("property 'name' ignored, type string expected.",
            errors()[0]);
  EXPECT_EQ("property 'short_name' ignored, type string expected.",
            errors()[1]);
  EXPECT_EQ("property 'start_url' ignored, type string expected.",
            errors()[2]);
  EXPECT_EQ("unknown 'display' value ignored.",
            errors()[3]);
  EXPECT_EQ("property 'orientation' ignored, type string expected.",
            errors()[4]);
  EXPECT_EQ("property 'icons' ignored, type array expected.",
            errors()[5]);
  EXPECT_EQ("property 'theme_color' ignored, type string expected.",
            errors()[6]);
  EXPECT_EQ("property 'background_color' ignored, type string expected.",
            errors()[7]);
}

TEST_F(ManifestParserTest, NameParseRules) {
  // Smoke test.
  {
    blink::Manifest manifest = ParseManifest("{ \"name\": \"foo\" }");
    ASSERT_TRUE(base::EqualsASCII(manifest.name.string(), "foo"));
    ASSERT_FALSE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Trim whitespaces.
  {
    blink::Manifest manifest = ParseManifest("{ \"name\": \"  foo  \" }");
    ASSERT_TRUE(base::EqualsASCII(manifest.name.string(), "foo"));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if name isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"name\": {} }");
    ASSERT_TRUE(manifest.name.is_null());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'name' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if name isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"name\": 42 }");
    ASSERT_TRUE(manifest.name.is_null());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'name' ignored, type string expected.",
              errors()[0]);
  }
}

TEST_F(ManifestParserTest, ShortNameParseRules) {
  // Smoke test.
  {
    blink::Manifest manifest = ParseManifest("{ \"short_name\": \"foo\" }");
    ASSERT_TRUE(base::EqualsASCII(manifest.short_name.string(), "foo"));
    ASSERT_FALSE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Trim whitespaces.
  {
    blink::Manifest manifest = ParseManifest("{ \"short_name\": \"  foo  \" }");
    ASSERT_TRUE(base::EqualsASCII(manifest.short_name.string(), "foo"));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if name isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"short_name\": {} }");
    ASSERT_TRUE(manifest.short_name.is_null());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'short_name' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if name isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"short_name\": 42 }");
    ASSERT_TRUE(manifest.short_name.is_null());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'short_name' ignored, type string expected.",
              errors()[0]);
  }
}

TEST_F(ManifestParserTest, StartURLParseRules) {
  // Smoke test.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"start_url\": \"land.html\" }");
    ASSERT_EQ(manifest.start_url.spec(),
              default_document_url.Resolve("land.html").spec());
    ASSERT_FALSE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Whitespaces.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"start_url\": \"  land.html  \" }");
    ASSERT_EQ(manifest.start_url.spec(),
              default_document_url.Resolve("land.html").spec());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if property isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"start_url\": {} }");
    ASSERT_TRUE(manifest.start_url.is_empty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'start_url' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if property isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"start_url\": 42 }");
    ASSERT_TRUE(manifest.start_url.is_empty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'start_url' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if property isn't a valid URL.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"start_url\": \"http://www.google.ca:a\" }");
    ASSERT_TRUE(manifest.start_url.is_empty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'start_url' ignored, URL is invalid.", errors()[0]);
  }

  // Absolute start_url, same origin with document.
  {
    blink::Manifest manifest =
        ParseManifestWithURLs("{ \"start_url\": \"http://foo.com/land.html\" }",
                              GURL("http://foo.com/manifest.json"),
                              GURL("http://foo.com/index.html"));
    ASSERT_EQ(manifest.start_url.spec(), "http://foo.com/land.html");
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Absolute start_url, cross origin with document.
  {
    blink::Manifest manifest =
        ParseManifestWithURLs("{ \"start_url\": \"http://bar.com/land.html\" }",
                              GURL("http://foo.com/manifest.json"),
                              GURL("http://foo.com/index.html"));
    ASSERT_TRUE(manifest.start_url.is_empty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'start_url' ignored, should "
              "be same origin as document.",
              errors()[0]);
  }

  // Resolving has to happen based on the manifest_url.
  {
    blink::Manifest manifest =
        ParseManifestWithURLs("{ \"start_url\": \"land.html\" }",
                              GURL("http://foo.com/landing/manifest.json"),
                              GURL("http://foo.com/index.html"));
    ASSERT_EQ(manifest.start_url.spec(), "http://foo.com/landing/land.html");
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, ScopeParseRules) {
  // Smoke test.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"scope\": \"land\", \"start_url\": \"land/landing.html\" }");
    ASSERT_EQ(manifest.scope.spec(),
              default_document_url.Resolve("land").spec());
    ASSERT_FALSE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Whitespaces.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"scope\": \"  land  \", \"start_url\": \"land/landing.html\" }");
    ASSERT_EQ(manifest.scope.spec(),
              default_document_url.Resolve("land").spec());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if property isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"scope\": {} }");
    ASSERT_TRUE(manifest.scope.is_empty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'scope' ignored, type string expected.", errors()[0]);
  }

  // Don't parse if property isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"scope\": 42 }");
    ASSERT_TRUE(manifest.scope.is_empty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'scope' ignored, type string expected.", errors()[0]);
  }

  // Absolute scope, start URL is in scope.
  {
    blink::Manifest manifest = ParseManifestWithURLs(
        "{ \"scope\": \"http://foo.com/land\", "
        "\"start_url\": \"http://foo.com/land/landing.html\" }",
        GURL("http://foo.com/manifest.json"),
        GURL("http://foo.com/index.html"));
    ASSERT_EQ(manifest.scope.spec(), "http://foo.com/land");
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Absolute scope, start URL is not in scope.
  {
    blink::Manifest manifest = ParseManifestWithURLs(
        "{ \"scope\": \"http://foo.com/land\", "
        "\"start_url\": \"http://foo.com/index.html\" }",
        GURL("http://foo.com/manifest.json"),
        GURL("http://foo.com/index.html"));
    ASSERT_TRUE(manifest.scope.is_empty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'scope' ignored. Start url should be within scope "
              "of scope URL.",
              errors()[0]);
  }

  // Absolute scope, start URL has different origin than scope URL.
  {
    blink::Manifest manifest = ParseManifestWithURLs(
        "{ \"scope\": \"http://foo.com/land\", "
        "\"start_url\": \"http://bar.com/land/landing.html\" }",
        GURL("http://foo.com/manifest.json"),
        GURL("http://foo.com/index.html"));
    ASSERT_TRUE(manifest.scope.is_empty());
    ASSERT_EQ(2u, GetErrorCount());
    EXPECT_EQ(
        "property 'start_url' ignored, should be same origin as document.",
        errors()[0]);
    EXPECT_EQ("property 'scope' ignored. Start url should be within scope "
              "of scope URL.",
              errors()[1]);
  }

  // scope and start URL have diferent origin than document URL.
  {
    blink::Manifest manifest = ParseManifestWithURLs(
        "{ \"scope\": \"http://foo.com/land\", "
        "\"start_url\": \"http://foo.com/land/landing.html\" }",
        GURL("http://foo.com/manifest.json"),
        GURL("http://bar.com/index.html"));
    ASSERT_TRUE(manifest.scope.is_empty());
    ASSERT_EQ(2u, GetErrorCount());
    EXPECT_EQ(
        "property 'start_url' ignored, should be same origin as document.",
        errors()[0]);
    EXPECT_EQ("property 'scope' ignored, should be same origin as document.",
              errors()[1]);
  }

  // No start URL. Document URL is in scope.
  {
    blink::Manifest manifest =
        ParseManifestWithURLs("{ \"scope\": \"http://foo.com/land\" }",
                              GURL("http://foo.com/manifest.json"),
                              GURL("http://foo.com/land/index.html"));
    ASSERT_EQ(manifest.scope.spec(), "http://foo.com/land");
    ASSERT_EQ(0u, GetErrorCount());
  }

  // No start URL. Document is out of scope.
  {
    blink::Manifest manifest =
        ParseManifestWithURLs("{ \"scope\": \"http://foo.com/land\" }",
                              GURL("http://foo.com/manifest.json"),
                              GURL("http://foo.com/index.html"));
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'scope' ignored. Start url should be within scope "
              "of scope URL.",
              errors()[0]);
  }

  // Resolving has to happen based on the manifest_url.
  {
    blink::Manifest manifest = ParseManifestWithURLs(
        "{ \"scope\": \"treasure\" }", GURL("http://foo.com/map/manifest.json"),
        GURL("http://foo.com/map/treasure/island/index.html"));
    ASSERT_EQ(manifest.scope.spec(), "http://foo.com/map/treasure");
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Scope is parent directory.
  {
    blink::Manifest manifest = ParseManifestWithURLs(
        "{ \"scope\": \"..\" }", GURL("http://foo.com/map/manifest.json"),
        GURL("http://foo.com/index.html"));
    ASSERT_EQ(manifest.scope.spec(), "http://foo.com/");
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Scope tries to go up past domain.
  {
    blink::Manifest manifest = ParseManifestWithURLs(
        "{ \"scope\": \"../..\" }", GURL("http://foo.com/map/manifest.json"),
        GURL("http://foo.com/index.html"));
    ASSERT_EQ(manifest.scope.spec(), "http://foo.com/");
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, DisplayParserRules) {
  // Smoke test.
  {
    blink::Manifest manifest = ParseManifest("{ \"display\": \"browser\" }");
    EXPECT_EQ(manifest.display, blink::kWebDisplayModeBrowser);
    EXPECT_FALSE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Trim whitespaces.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"display\": \"  browser  \" }");
    EXPECT_EQ(manifest.display, blink::kWebDisplayModeBrowser);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if name isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"display\": {} }");
    EXPECT_EQ(manifest.display, blink::kWebDisplayModeUndefined);
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'display' ignored,"
              " type string expected.",
              errors()[0]);
  }

  // Don't parse if name isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"display\": 42 }");
    EXPECT_EQ(manifest.display, blink::kWebDisplayModeUndefined);
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'display' ignored,"
              " type string expected.",
              errors()[0]);
  }

  // Parse fails if string isn't known.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"display\": \"browser_something\" }");
    EXPECT_EQ(manifest.display, blink::kWebDisplayModeUndefined);
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("unknown 'display' value ignored.",
              errors()[0]);
  }

  // Accept 'fullscreen'.
  {
    blink::Manifest manifest = ParseManifest("{ \"display\": \"fullscreen\" }");
    EXPECT_EQ(manifest.display, blink::kWebDisplayModeFullscreen);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept 'fullscreen'.
  {
    blink::Manifest manifest = ParseManifest("{ \"display\": \"standalone\" }");
    EXPECT_EQ(manifest.display, blink::kWebDisplayModeStandalone);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept 'minimal-ui'.
  {
    blink::Manifest manifest = ParseManifest("{ \"display\": \"minimal-ui\" }");
    EXPECT_EQ(manifest.display, blink::kWebDisplayModeMinimalUi);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept 'browser'.
  {
    blink::Manifest manifest = ParseManifest("{ \"display\": \"browser\" }");
    EXPECT_EQ(manifest.display, blink::kWebDisplayModeBrowser);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Case insensitive.
  {
    blink::Manifest manifest = ParseManifest("{ \"display\": \"BROWSER\" }");
    EXPECT_EQ(manifest.display, blink::kWebDisplayModeBrowser);
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, OrientationParserRules) {
  // Smoke test.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"orientation\": \"natural\" }");
    EXPECT_EQ(manifest.orientation, blink::kWebScreenOrientationLockNatural);
    EXPECT_FALSE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Trim whitespaces.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"orientation\": \"natural\" }");
    EXPECT_EQ(manifest.orientation, blink::kWebScreenOrientationLockNatural);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if name isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"orientation\": {} }");
    EXPECT_EQ(manifest.orientation, blink::kWebScreenOrientationLockDefault);
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'orientation' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if name isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"orientation\": 42 }");
    EXPECT_EQ(manifest.orientation, blink::kWebScreenOrientationLockDefault);
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'orientation' ignored, type string expected.",
              errors()[0]);
  }

  // Parse fails if string isn't known.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"orientation\": \"naturalish\" }");
    EXPECT_EQ(manifest.orientation, blink::kWebScreenOrientationLockDefault);
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("unknown 'orientation' value ignored.",
              errors()[0]);
  }

  // Accept 'any'.
  {
    blink::Manifest manifest = ParseManifest("{ \"orientation\": \"any\" }");
    EXPECT_EQ(manifest.orientation, blink::kWebScreenOrientationLockAny);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept 'natural'.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"orientation\": \"natural\" }");
    EXPECT_EQ(manifest.orientation, blink::kWebScreenOrientationLockNatural);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept 'landscape'.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"orientation\": \"landscape\" }");
    EXPECT_EQ(manifest.orientation, blink::kWebScreenOrientationLockLandscape);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept 'landscape-primary'.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"orientation\": \"landscape-primary\" }");
    EXPECT_EQ(manifest.orientation,
              blink::kWebScreenOrientationLockLandscapePrimary);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept 'landscape-secondary'.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"orientation\": \"landscape-secondary\" }");
    EXPECT_EQ(manifest.orientation,
              blink::kWebScreenOrientationLockLandscapeSecondary);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept 'portrait'.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"orientation\": \"portrait\" }");
    EXPECT_EQ(manifest.orientation, blink::kWebScreenOrientationLockPortrait);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept 'portrait-primary'.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"orientation\": \"portrait-primary\" }");
    EXPECT_EQ(manifest.orientation,
              blink::kWebScreenOrientationLockPortraitPrimary);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept 'portrait-secondary'.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"orientation\": \"portrait-secondary\" }");
    EXPECT_EQ(manifest.orientation,
              blink::kWebScreenOrientationLockPortraitSecondary);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Case insensitive.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"orientation\": \"LANDSCAPE\" }");
    EXPECT_EQ(manifest.orientation, blink::kWebScreenOrientationLockLandscape);
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, IconsParseRules) {
  // Smoke test: if no icon, empty list.
  {
    blink::Manifest manifest = ParseManifest("{ \"icons\": [] }");
    EXPECT_EQ(manifest.icons.size(), 0u);
    EXPECT_TRUE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Smoke test: if empty icon, empty list.
  {
    blink::Manifest manifest = ParseManifest("{ \"icons\": [ {} ] }");
    EXPECT_EQ(manifest.icons.size(), 0u);
    EXPECT_TRUE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Smoke test: icon with invalid src, empty list.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"icons\": [ { \"icons\": [] } ] }");
    EXPECT_EQ(manifest.icons.size(), 0u);
    EXPECT_TRUE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Smoke test: if icon with empty src, it will be present in the list.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"icons\": [ { \"src\": \"\" } ] }");
    EXPECT_EQ(manifest.icons.size(), 1u);
    EXPECT_EQ(manifest.icons[0].src.spec(), "http://foo.com/manifest.json");
    EXPECT_FALSE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Smoke test: if one icons with valid src, it will be present in the list.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"icons\": [{ \"src\": \"foo.jpg\" }] }");
    EXPECT_EQ(manifest.icons.size(), 1u);
    EXPECT_EQ(manifest.icons[0].src.spec(), "http://foo.com/foo.jpg");
    EXPECT_FALSE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, IconSrcParseRules) {
  // Smoke test.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"icons\": [ {\"src\": \"foo.png\" } ] }");
    EXPECT_EQ(manifest.icons[0].src.spec(),
              default_document_url.Resolve("foo.png").spec());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Whitespaces.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"icons\": [ {\"src\": \"   foo.png   \" } ] }");
    EXPECT_EQ(manifest.icons[0].src.spec(),
              default_document_url.Resolve("foo.png").spec());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if property isn't a string.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"icons\": [ {\"src\": {} } ] }");
    EXPECT_TRUE(manifest.icons.empty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'src' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if property isn't a string.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"icons\": [ {\"src\": 42 } ] }");
    EXPECT_TRUE(manifest.icons.empty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'src' ignored, type string expected.",
              errors()[0]);
  }

  // Resolving has to happen based on the document_url.
  {
    blink::Manifest manifest = ParseManifestWithURLs(
        "{ \"icons\": [ {\"src\": \"icons/foo.png\" } ] }",
        GURL("http://foo.com/landing/index.html"), default_manifest_url);
    EXPECT_EQ(manifest.icons[0].src.spec(),
              "http://foo.com/landing/icons/foo.png");
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, IconTypeParseRules) {
  // Smoke test.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"icons\": [ {\"src\": \"\", \"type\": \"foo\" } ] }");
    EXPECT_TRUE(base::EqualsASCII(manifest.icons[0].type, "foo"));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Trim whitespaces.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        " \"type\": \"  foo  \" } ] }");
    EXPECT_TRUE(base::EqualsASCII(manifest.icons[0].type, "foo"));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if property isn't a string.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"icons\": [ {\"src\": \"\", \"type\": {} } ] }");
    EXPECT_TRUE(manifest.icons[0].type.empty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'type' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if property isn't a string.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"icons\": [ {\"src\": \"\", \"type\": 42 } ] }");
    EXPECT_TRUE(manifest.icons[0].type.empty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'type' ignored, type string expected.",
              errors()[0]);
  }
}

TEST_F(ManifestParserTest, IconSizesParseRules) {
  // Smoke test.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"sizes\": \"42x42\" } ] }");
    EXPECT_EQ(manifest.icons[0].sizes.size(), 1u);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Trim whitespaces.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"sizes\": \"  42x42  \" } ] }");
    EXPECT_EQ(manifest.icons[0].sizes.size(), 1u);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Ignore sizes if property isn't a string.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"sizes\": {} } ] }");
    EXPECT_EQ(manifest.icons[0].sizes.size(), 0u);
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'sizes' ignored, type string expected.",
              errors()[0]);
  }

  // Ignore sizes if property isn't a string.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"sizes\": 42 } ] }");
    EXPECT_EQ(manifest.icons[0].sizes.size(), 0u);
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'sizes' ignored, type string expected.",
              errors()[0]);
  }

  // Smoke test: value correctly parsed.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"sizes\": \"42x42  48x48\" } ] }");
    EXPECT_EQ(manifest.icons[0].sizes[0], gfx::Size(42, 42));
    EXPECT_EQ(manifest.icons[0].sizes[1], gfx::Size(48, 48));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // <WIDTH>'x'<HEIGHT> and <WIDTH>'X'<HEIGHT> are equivalent.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"sizes\": \"42X42  48X48\" } ] }");
    EXPECT_EQ(manifest.icons[0].sizes[0], gfx::Size(42, 42));
    EXPECT_EQ(manifest.icons[0].sizes[1], gfx::Size(48, 48));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Twice the same value is parsed twice.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"sizes\": \"42X42  42x42\" } ] }");
    EXPECT_EQ(manifest.icons[0].sizes[0], gfx::Size(42, 42));
    EXPECT_EQ(manifest.icons[0].sizes[1], gfx::Size(42, 42));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Width or height can't start with 0.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"sizes\": \"004X007  042x00\" } ] }");
    EXPECT_EQ(manifest.icons[0].sizes.size(), 0u);
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("found icon with no valid size.",
              errors()[0]);
  }

  // Width and height MUST contain digits.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"sizes\": \"e4X1.0  55ax1e10\" } ] }");
    EXPECT_EQ(manifest.icons[0].sizes.size(), 0u);
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("found icon with no valid size.",
              errors()[0]);
  }

  // 'any' is correctly parsed and transformed to gfx::Size(0,0).
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"sizes\": \"any AnY ANY aNy\" } ] }");
    gfx::Size any = gfx::Size(0, 0);
    EXPECT_EQ(manifest.icons[0].sizes.size(), 4u);
    EXPECT_EQ(manifest.icons[0].sizes[0], any);
    EXPECT_EQ(manifest.icons[0].sizes[1], any);
    EXPECT_EQ(manifest.icons[0].sizes[2], any);
    EXPECT_EQ(manifest.icons[0].sizes[3], any);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Some invalid width/height combinations.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"sizes\": \"x 40xx 1x2x3 x42 42xx42\" } ] }");
    EXPECT_EQ(manifest.icons[0].sizes.size(), 0u);
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("found icon with no valid size.",
              errors()[0]);
  }
}

TEST_F(ManifestParserTest, IconPurposeParseRules) {
  const std::string kPurposeParseStringError =
      "property 'purpose' ignored, type string expected.";
  const std::string kPurposeInvalidValueError =
      "found icon with invalid purpose. "
      "Using default value 'any'.";

  // Smoke test.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"purpose\": \"any\" } ] }");
    EXPECT_EQ(manifest.icons[0].purpose.size(), 1u);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Trim leading and trailing whitespaces.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"purpose\": \"  any  \" } ] }");
    EXPECT_EQ(manifest.icons[0].purpose.size(), 1u);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // 'any' is added when property isn't present.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"icons\": [ {\"src\": \"\" } ] }");
    EXPECT_EQ(manifest.icons[0].purpose.size(), 1u);
    EXPECT_EQ(manifest.icons[0].purpose[0],
              blink::Manifest::Icon::IconPurpose::ANY);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // 'any' is added with error message when property isn't a string (is a
  // number).
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"purpose\": 42 } ] }");
    EXPECT_EQ(manifest.icons[0].purpose.size(), 1u);
    EXPECT_EQ(manifest.icons[0].purpose[0],
              blink::Manifest::Icon::IconPurpose::ANY);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(kPurposeParseStringError, errors()[0]);
  }

  // 'any' is added with error message when property isn't a string (is a
  // dictionary).
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"purpose\": {} } ] }");
    EXPECT_EQ(manifest.icons[0].purpose.size(), 1u);
    EXPECT_EQ(manifest.icons[0].purpose[0],
              blink::Manifest::Icon::IconPurpose::ANY);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(kPurposeParseStringError, errors()[0]);
  }

  // Smoke test: values correctly parsed.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"purpose\": \"Any Badge\" } ] }");
    ASSERT_EQ(manifest.icons[0].purpose.size(), 2u);
    EXPECT_EQ(manifest.icons[0].purpose[0],
              blink::Manifest::Icon::IconPurpose::ANY);
    EXPECT_EQ(manifest.icons[0].purpose[1],
              blink::Manifest::Icon::IconPurpose::BADGE);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Trim whitespaces between values.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"purpose\": \"  Any   Badge  \" } ] }");
    ASSERT_EQ(manifest.icons[0].purpose.size(), 2u);
    EXPECT_EQ(manifest.icons[0].purpose[0],
              blink::Manifest::Icon::IconPurpose::ANY);
    EXPECT_EQ(manifest.icons[0].purpose[1],
              blink::Manifest::Icon::IconPurpose::BADGE);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Twice the same value is parsed twice.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"purpose\": \"badge badge\" } ] }");
    ASSERT_EQ(manifest.icons[0].purpose.size(), 2u);
    EXPECT_EQ(manifest.icons[0].purpose[0],
              blink::Manifest::Icon::IconPurpose::BADGE);
    EXPECT_EQ(manifest.icons[0].purpose[1],
              blink::Manifest::Icon::IconPurpose::BADGE);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Invalid icon purpose is ignored.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"purpose\": \"badge notification\" } ] }");
    ASSERT_EQ(manifest.icons[0].purpose.size(), 1u);
    EXPECT_EQ(manifest.icons[0].purpose[0],
              blink::Manifest::Icon::IconPurpose::BADGE);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(kPurposeInvalidValueError, errors()[0]);
  }

  // 'any' is added when developer-supplied purpose is invalid.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"purpose\": \"notification\" } ] }");
    ASSERT_EQ(manifest.icons[0].purpose.size(), 1u);
    EXPECT_EQ(manifest.icons[0].purpose[0],
              blink::Manifest::Icon::IconPurpose::ANY);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(kPurposeInvalidValueError, errors()[0]);
  }
}

TEST_F(ManifestParserTest, ShareTargetParseRules) {
  // Contains share_target field but no keys.
  {
    blink::Manifest manifest = ParseManifest("{ \"share_target\": {} }");
    EXPECT_FALSE(manifest.share_target.has_value());
    EXPECT_TRUE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Key in share_target that isn't valid.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"share_target\": {\"incorrect_key\": \"some_value\" } }");
    ASSERT_FALSE(manifest.share_target.has_value());
    EXPECT_TRUE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, ShareTargetUrlTemplateParseRules) {
  GURL manifest_url = GURL("https://foo.com/manifest.json");
  GURL document_url = GURL("https://foo.com/index.html");

  // Contains share_target and url_template, but url_template is empty.
  {
    blink::Manifest manifest = ParseManifestWithURLs(
        "{ \"share_target\": { \"url_template\": \"\" } }", manifest_url,
        document_url);
    ASSERT_TRUE(manifest.share_target.has_value());
    EXPECT_EQ(manifest.share_target.value().url_template.spec(),
              manifest_url.spec());
    EXPECT_FALSE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if property isn't a string.
  {
    blink::Manifest manifest =
        ParseManifestWithURLs("{ \"share_target\": { \"url_template\": {} } }",
                              manifest_url, document_url);
    EXPECT_FALSE(manifest.share_target.has_value());
    EXPECT_TRUE(manifest.IsEmpty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'url_template' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if property isn't a string.
  {
    blink::Manifest manifest =
        ParseManifestWithURLs("{ \"share_target\": { \"url_template\": 42 } }",
                              manifest_url, document_url);
    EXPECT_FALSE(manifest.share_target.has_value());
    EXPECT_TRUE(manifest.IsEmpty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'url_template' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if property isn't a valid URL.
  {
    blink::Manifest manifest = ParseManifestWithURLs(
        "{ \"share_target\": { \"url_template\": \"https://foo.com:a\" } "
        "}",
        manifest_url, document_url);
    EXPECT_FALSE(manifest.share_target.has_value());
    EXPECT_TRUE(manifest.IsEmpty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'url_template' ignored, URL is invalid.", errors()[0]);
  }

  // Fail parsing if url_template is at a different origin than the Web
  // Manifest.
  {
    blink::Manifest manifest = ParseManifestWithURLs(
        "{ \"share_target\": { \"url_template\": \"https://foo2.com/\" } }",
        manifest_url, document_url);
    EXPECT_FALSE(manifest.share_target.has_value());
    EXPECT_TRUE(manifest.IsEmpty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'url_template' ignored, should be same origin as document.",
        errors()[0]);
  }

  // Smoke test: Contains share_target and url_template, and url_template is
  // valid template.
  {
    blink::Manifest manifest = ParseManifestWithURLs(
        "{ \"share_target\": {\"url_template\": \"share/?title={title}\" } }",
        manifest_url, document_url);
    ASSERT_TRUE(manifest.share_target.has_value());
    EXPECT_EQ(manifest.share_target.value().url_template.spec(),
              "https://foo.com/share/?title={title}");
    EXPECT_FALSE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Smoke test: Contains share_target and url_template, and url_template is
  // invalid template.
  {
    blink::Manifest manifest = ParseManifestWithURLs(
        "{ \"share_target\": {\"url_template\": \"share/?title={title\" } }",
        manifest_url, document_url);
    ASSERT_FALSE(manifest.share_target.has_value());
    EXPECT_TRUE(manifest.IsEmpty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'url_template' ignored. Placeholders have incorrect "
        "syntax.",
        errors()[0]);
  }

  // Smoke test: Contains share_target and url_template, and url_template
  // contains unknown placeholder.
  {
    blink::Manifest manifest = ParseManifestWithURLs(
        "{ \"share_target\": {\"url_template\": \"share/?title={abcxyz}\" } }",
        manifest_url, document_url);
    ASSERT_TRUE(manifest.share_target.has_value());
    EXPECT_EQ(manifest.share_target.value().url_template.spec(),
              "https://foo.com/share/?title={abcxyz}");
    EXPECT_FALSE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Smoke test: Contains share_target and url_template, and url_template has
  // '{' and '}' in path, query and fragment. Only '{' and '}' in path should be
  // escaped.
  {
    blink::Manifest manifest = ParseManifestWithURLs(
        "{ \"share_target\": {\"url_template\": "
        "\"share/a{text}/?title={title}#{frag}\" } }",
        manifest_url, document_url);
    ASSERT_TRUE(manifest.share_target.has_value());
    EXPECT_EQ(manifest.share_target.value().url_template.spec(),
              "https://foo.com/share/a%7Btext%7D/?title={title}#{frag}");
    EXPECT_FALSE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Smoke test: Contains share_target and url_template. url_template is
  // valid template and is absolute.
  {
    blink::Manifest manifest = ParseManifestWithURLs(
        "{ \"share_target\": { \"url_template\": \"https://foo.com/#{text}\" } "
        "}",
        manifest_url, document_url);
    ASSERT_TRUE(manifest.share_target.has_value());
    EXPECT_EQ(manifest.share_target.value().url_template.spec(),
              "https://foo.com/#{text}");
    EXPECT_FALSE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, RelatedApplicationsParseRules) {
  // If no application, empty list.
  {
    blink::Manifest manifest = ParseManifest("{ \"related_applications\": []}");
    EXPECT_EQ(manifest.related_applications.size(), 0u);
    EXPECT_TRUE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // If empty application, empty list.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"related_applications\": [{}]}");
    EXPECT_EQ(manifest.related_applications.size(), 0u);
    EXPECT_TRUE(manifest.IsEmpty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("'platform' is a required field, related application ignored.",
              errors()[0]);
  }

  // If invalid platform, application is ignored.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"related_applications\": [{\"platform\": 123}]}");
    EXPECT_EQ(manifest.related_applications.size(), 0u);
    EXPECT_TRUE(manifest.IsEmpty());
    EXPECT_EQ(2u, GetErrorCount());
    EXPECT_EQ(
        "property 'platform' ignored, type string expected.",
        errors()[0]);
    EXPECT_EQ("'platform' is a required field, "
              "related application ignored.",
              errors()[1]);
  }

  // If missing platform, application is ignored.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"related_applications\": [{\"id\": \"foo\"}]}");
    EXPECT_EQ(manifest.related_applications.size(), 0u);
    EXPECT_TRUE(manifest.IsEmpty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("'platform' is a required field, related application ignored.",
              errors()[0]);
  }

  // If missing id and url, application is ignored.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"related_applications\": [{\"platform\": \"play\"}]}");
    EXPECT_EQ(manifest.related_applications.size(), 0u);
    EXPECT_TRUE(manifest.IsEmpty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("one of 'url' or 'id' is required, related application ignored.",
              errors()[0]);
  }

  // Valid application, with url.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"related_applications\": ["
        "{\"platform\": \"play\", \"url\": \"http://www.foo.com\"}]}");
    EXPECT_EQ(manifest.related_applications.size(), 1u);
    EXPECT_TRUE(base::EqualsASCII(
        manifest.related_applications[0].platform.string(),
        "play"));
    EXPECT_EQ(manifest.related_applications[0].url.spec(),
              "http://www.foo.com/");
    EXPECT_FALSE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Application with an invalid url.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"related_applications\": ["
        "{\"platform\": \"play\", \"url\": \"http://www.foo.com:co&uk\"}]}");
    EXPECT_TRUE(manifest.IsEmpty());
    EXPECT_EQ(2u, GetErrorCount());
    EXPECT_EQ("property 'url' ignored, URL is invalid.", errors()[0]);
    EXPECT_EQ("one of 'url' or 'id' is required, related application ignored.",
              errors()[1]);
  }

  // Valid application, with id.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"related_applications\": ["
        "{\"platform\": \"itunes\", \"id\": \"foo\"}]}");
    EXPECT_EQ(manifest.related_applications.size(), 1u);
    EXPECT_TRUE(base::EqualsASCII(
        manifest.related_applications[0].platform.string(),
        "itunes"));
    EXPECT_TRUE(base::EqualsASCII(manifest.related_applications[0].id.string(),
                                  "foo"));
    EXPECT_FALSE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // All valid applications are in list.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"related_applications\": ["
        "{\"platform\": \"play\", \"id\": \"foo\"},"
        "{\"platform\": \"itunes\", \"id\": \"bar\"}]}");
    EXPECT_EQ(manifest.related_applications.size(), 2u);
    EXPECT_TRUE(base::EqualsASCII(
        manifest.related_applications[0].platform.string(),
        "play"));
    EXPECT_TRUE(base::EqualsASCII(manifest.related_applications[0].id.string(),
                                  "foo"));
    EXPECT_TRUE(base::EqualsASCII(
        manifest.related_applications[1].platform.string(),
        "itunes"));
    EXPECT_TRUE(base::EqualsASCII(manifest.related_applications[1].id.string(),
                                  "bar"));
    EXPECT_FALSE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Two invalid applications and one valid. Only the valid application should
  // be in the list.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"related_applications\": ["
        "{\"platform\": \"itunes\"},"
        "{\"platform\": \"play\", \"id\": \"foo\"},"
        "{}]}");
    EXPECT_EQ(manifest.related_applications.size(), 1u);
    EXPECT_TRUE(base::EqualsASCII(
        manifest.related_applications[0].platform.string(),
        "play"));
    EXPECT_TRUE(base::EqualsASCII(manifest.related_applications[0].id.string(),
                                  "foo"));
    EXPECT_FALSE(manifest.IsEmpty());
    EXPECT_EQ(2u, GetErrorCount());
    EXPECT_EQ("one of 'url' or 'id' is required, related application ignored.",
              errors()[0]);
    EXPECT_EQ("'platform' is a required field, related application ignored.",
              errors()[1]);
  }
}

TEST_F(ManifestParserTest, ParsePreferRelatedApplicationsParseRules) {
  // Smoke test.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"prefer_related_applications\": true }");
    EXPECT_TRUE(manifest.prefer_related_applications);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if the property isn't a boolean.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"prefer_related_applications\": {} }");
    EXPECT_FALSE(manifest.prefer_related_applications);
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'prefer_related_applications' "
        "ignored, type boolean expected.",
        errors()[0]);
  }
  {
    blink::Manifest manifest =
        ParseManifest("{ \"prefer_related_applications\": \"true\" }");
    EXPECT_FALSE(manifest.prefer_related_applications);
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'prefer_related_applications' "
        "ignored, type boolean expected.",
        errors()[0]);
  }
  {
    blink::Manifest manifest =
        ParseManifest("{ \"prefer_related_applications\": 1 }");
    EXPECT_FALSE(manifest.prefer_related_applications);
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'prefer_related_applications' "
        "ignored, type boolean expected.",
        errors()[0]);
  }

  // "False" should set the boolean false without throwing errors.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"prefer_related_applications\": false }");
    EXPECT_FALSE(manifest.prefer_related_applications);
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, ThemeColorParserRules) {
  // Smoke test.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"theme_color\": \"#FF0000\" }");
    EXPECT_EQ(*manifest.theme_color, 0xFFFF0000u);
    EXPECT_FALSE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Trim whitespaces.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"theme_color\": \"  blue   \" }");
    EXPECT_EQ(*manifest.theme_color, 0xFF0000FFu);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if theme_color isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"theme_color\": {} }");
    EXPECT_FALSE(manifest.theme_color.has_value());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'theme_color' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if theme_color isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"theme_color\": false }");
    EXPECT_FALSE(manifest.theme_color.has_value());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'theme_color' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if theme_color isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"theme_color\": null }");
    EXPECT_FALSE(manifest.theme_color.has_value());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'theme_color' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if theme_color isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"theme_color\": [] }");
    EXPECT_FALSE(manifest.theme_color.has_value());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'theme_color' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if theme_color isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"theme_color\": 42 }");
    EXPECT_FALSE(manifest.theme_color.has_value());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'theme_color' ignored, type string expected.",
              errors()[0]);
  }

  // Parse fails if string is not in a known format.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"theme_color\": \"foo(bar)\" }");
    EXPECT_FALSE(manifest.theme_color.has_value());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'theme_color' ignored,"
              " 'foo(bar)' is not a valid color.",
              errors()[0]);
  }

  // Parse fails if string is not in a known format.
  {
    blink::Manifest manifest = ParseManifest("{ \"theme_color\": \"bleu\" }");
    EXPECT_FALSE(manifest.theme_color.has_value());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'theme_color' ignored, 'bleu' is not a valid color.",
              errors()[0]);
  }

  // Parse fails if string is not in a known format.
  {
    blink::Manifest manifest = ParseManifest("{ \"theme_color\": \"FF00FF\" }");
    EXPECT_FALSE(manifest.theme_color.has_value());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'theme_color' ignored, 'FF00FF'"
              " is not a valid color.",
              errors()[0]);
  }

  // Parse fails if multiple values for theme_color are given.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"theme_color\": \"#ABC #DEF\" }");
    EXPECT_FALSE(manifest.theme_color.has_value());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'theme_color' ignored, "
              "'#ABC #DEF' is not a valid color.",
              errors()[0]);
  }

  // Parse fails if multiple values for theme_color are given.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"theme_color\": \"#AABBCC #DDEEFF\" }");
    EXPECT_FALSE(manifest.theme_color.has_value());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'theme_color' ignored, "
              "'#AABBCC #DDEEFF' is not a valid color.",
              errors()[0]);
  }

  // Accept CSS color keyword format.
  {
    blink::Manifest manifest = ParseManifest("{ \"theme_color\": \"blue\" }");
    EXPECT_EQ(*manifest.theme_color, 0xFF0000FFu);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept CSS color keyword format.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"theme_color\": \"chartreuse\" }");
    EXPECT_EQ(*manifest.theme_color, 0xFF7FFF00u);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept CSS RGB format.
  {
    blink::Manifest manifest = ParseManifest("{ \"theme_color\": \"#FFF\" }");
    EXPECT_EQ(*manifest.theme_color, 0xFFFFFFFFu);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept CSS RGB format.
  {
    blink::Manifest manifest = ParseManifest("{ \"theme_color\": \"#ABC\" }");
    EXPECT_EQ(*manifest.theme_color, 0xFFAABBCCu);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept CSS RRGGBB format.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"theme_color\": \"#FF0000\" }");
    EXPECT_EQ(*manifest.theme_color, 0xFFFF0000u);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept translucent colors.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"theme_color\": \"rgba(255,0,0,"
        "0.4)\" }");
    EXPECT_EQ(*manifest.theme_color, 0x66FF0000u);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept transparent colors.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"theme_color\": \"rgba(0,0,0,0)\" }");
    EXPECT_EQ(*manifest.theme_color, 0x00000000u);
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, BackgroundColorParserRules) {
  // Smoke test.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"background_color\": \"#FF0000\" }");
    EXPECT_EQ(*manifest.background_color, 0xFFFF0000u);
    EXPECT_FALSE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Trim whitespaces.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"background_color\": \"  blue   \" }");
    EXPECT_EQ(*manifest.background_color, 0xFF0000FFu);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if background_color isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"background_color\": {} }");
    EXPECT_FALSE(manifest.background_color.has_value());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'background_color' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if background_color isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"background_color\": false }");
    EXPECT_FALSE(manifest.background_color.has_value());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'background_color' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if background_color isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"background_color\": null }");
    EXPECT_FALSE(manifest.background_color.has_value());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'background_color' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if background_color isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"background_color\": [] }");
    EXPECT_FALSE(manifest.background_color.has_value());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'background_color' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if background_color isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"background_color\": 42 }");
    EXPECT_FALSE(manifest.background_color.has_value());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'background_color' ignored, type string expected.",
              errors()[0]);
  }

  // Parse fails if string is not in a known format.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"background_color\": \"foo(bar)\" }");
    EXPECT_FALSE(manifest.background_color.has_value());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'background_color' ignored,"
              " 'foo(bar)' is not a valid color.",
              errors()[0]);
  }

  // Parse fails if string is not in a known format.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"background_color\": \"bleu\" }");
    EXPECT_FALSE(manifest.background_color.has_value());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'background_color' ignored,"
              " 'bleu' is not a valid color.",
              errors()[0]);
  }

  // Parse fails if string is not in a known format.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"background_color\": \"FF00FF\" }");
    EXPECT_FALSE(manifest.background_color.has_value());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'background_color' ignored,"
              " 'FF00FF' is not a valid color.",
              errors()[0]);
  }

  // Parse fails if multiple values for background_color are given.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"background_color\": \"#ABC #DEF\" }");
    EXPECT_FALSE(manifest.background_color.has_value());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'background_color' ignored, "
              "'#ABC #DEF' is not a valid color.",
              errors()[0]);
  }

  // Parse fails if multiple values for background_color are given.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"background_color\": \"#AABBCC #DDEEFF\" }");
    EXPECT_FALSE(manifest.background_color.has_value());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'background_color' ignored, "
              "'#AABBCC #DDEEFF' is not a valid color.",
              errors()[0]);
  }

  // Accept CSS color keyword format.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"background_color\": \"blue\" }");
    EXPECT_EQ(*manifest.background_color, 0xFF0000FFu);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept CSS color keyword format.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"background_color\": \"chartreuse\" }");
    EXPECT_EQ(*manifest.background_color, 0xFF7FFF00u);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept CSS RGB format.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"background_color\": \"#FFF\" }");
    EXPECT_EQ(*manifest.background_color, 0xFFFFFFFFu);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept CSS RGB format.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"background_color\": \"#ABC\" }");
    EXPECT_EQ(*manifest.background_color, 0xFFAABBCCu);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept CSS RRGGBB format.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"background_color\": \"#FF0000\" }");
    EXPECT_EQ(*manifest.background_color, 0xFFFF0000u);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept translucent colors.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"background_color\": \"rgba(255,0,0,"
        "0.4)\" }");
    EXPECT_EQ(*manifest.background_color, 0x66FF0000u);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept transparent colors.
  {
    blink::Manifest manifest = ParseManifest(
        "{ \"background_color\": \"rgba(0,0,0,"
        "0)\" }");
    EXPECT_EQ(*manifest.background_color, 0x00000000u);
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, SplashScreenUrlParseRules) {
  // Smoke test.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"splash_screen_url\": \"splash.html\" }");
    ASSERT_EQ(manifest.splash_screen_url.spec(),
              default_document_url.Resolve("splash.html").spec());
    ASSERT_FALSE(manifest.IsEmpty());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Whitespaces.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"splash_screen_url\": \"    splash.html\" }");
    ASSERT_EQ(manifest.splash_screen_url.spec(),
              default_document_url.Resolve("splash.html").spec());
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if property isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"splash_screen_url\": {} }");
    ASSERT_TRUE(manifest.splash_screen_url.is_empty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'splash_screen_url' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if property isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"splash_screen_url\": 42 }");
    ASSERT_TRUE(manifest.splash_screen_url.is_empty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'splash_screen_url' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if property isn't a valid URL.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"splash_screen_url\": \"http://www.google.ca:a\" }");
    ASSERT_TRUE(manifest.splash_screen_url.is_empty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'splash_screen_url' ignored, URL is invalid.",
              errors()[0]);
  }

  // Absolute splash_screen_url, same origin with document.
  {
    blink::Manifest manifest = ParseManifestWithURLs(
        "{ \"splash_screen_url\": \"http://foo.com/splash.html\" }",
        GURL("http://foo.com/manifest.json"),
        GURL("http://foo.com/index.html"));
    ASSERT_EQ(manifest.splash_screen_url.spec(), "http://foo.com/splash.html");
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Absolute splash_screen_url, cross origin with document.
  {
    blink::Manifest manifest = ParseManifestWithURLs(
        "{ \"splash_screen_url\": \"http://bar.com/splash.html\" }",
        GURL("http://foo.com/manifest.json"),
        GURL("http://foo.com/index.html"));
    ASSERT_TRUE(manifest.splash_screen_url.is_empty());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'splash_screen_url' ignored, should "
        "be same origin as document.",
        errors()[0]);
  }

  // Resolving has to happen based on the manifest_url.
  {
    blink::Manifest manifest =
        ParseManifestWithURLs("{ \"splash_screen_url\": \"splash.html\" }",
                              GURL("http://foo.com/splashy/manifest.json"),
                              GURL("http://foo.com/index.html"));
    ASSERT_EQ(manifest.splash_screen_url.spec(),
              "http://foo.com/splashy/splash.html");
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, GCMSenderIDParseRules) {
  // Smoke test.
  {
    blink::Manifest manifest = ParseManifest("{ \"gcm_sender_id\": \"foo\" }");
    EXPECT_TRUE(base::EqualsASCII(manifest.gcm_sender_id.string(), "foo"));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Trim whitespaces.
  {
    blink::Manifest manifest =
        ParseManifest("{ \"gcm_sender_id\": \"  foo  \" }");
    EXPECT_TRUE(base::EqualsASCII(manifest.gcm_sender_id.string(), "foo"));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if the property isn't a string.
  {
    blink::Manifest manifest = ParseManifest("{ \"gcm_sender_id\": {} }");
    EXPECT_TRUE(manifest.gcm_sender_id.is_null());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'gcm_sender_id' ignored, type string expected.",
              errors()[0]);
  }
  {
    blink::Manifest manifest = ParseManifest("{ \"gcm_sender_id\": 42 }");
    EXPECT_TRUE(manifest.gcm_sender_id.is_null());
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'gcm_sender_id' ignored, type string expected.",
              errors()[0]);
  }
}

} // namespace content
