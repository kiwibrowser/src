// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/chrome_content_browser_client_extensions_part.h"

#include <string>
#include <vector>

#include "base/test/histogram_tester.h"
#include "chrome/common/chrome_content_client.h"
#include "extensions/common/url_pattern_set.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace {

void AddPattern(URLPatternSet* set, const std::string& pattern) {
  int schemes = URLPattern::SCHEME_ALL;
  set->AddPattern(URLPattern(schemes, pattern));
}

}  // namespace

typedef testing::Test ChromeContentBrowserClientExtensionsPartTest;

// Check that empty site URLs get recorded properly in ShouldAllowOpenURL
// failures.
TEST_F(ChromeContentBrowserClientExtensionsPartTest,
       ShouldAllowOpenURLMetricsForEmptySiteURL) {
  base::HistogramTester uma;

  auto failure_reason = ChromeContentBrowserClientExtensionsPart::
      FAILURE_SCHEME_NOT_HTTP_OR_HTTPS_OR_EXTENSION;
  ChromeContentBrowserClientExtensionsPart::RecordShouldAllowOpenURLFailure(
      failure_reason, GURL());
  uma.ExpectUniqueSample("Extensions.ShouldAllowOpenURL.Failure",
                         failure_reason, 1);
  uma.ExpectUniqueSample("Extensions.ShouldAllowOpenURL.Failure.Scheme",
                         1 /* SCHEME_EMPTY */, 1);
}

// Check that a non-exhaustive list of some known schemes get recorded properly
// in ShouldAllowOpenURL failures.
TEST_F(ChromeContentBrowserClientExtensionsPartTest,
       ShouldAllowOpenURLMetricsForKnownSchemes) {
  base::HistogramTester uma;

  ChromeContentClient content_client;
  content::ContentClient::Schemes schemes;
  content_client.AddAdditionalSchemes(&schemes);

  std::vector<std::string> test_schemes(schemes.savable_schemes);
  test_schemes.insert(test_schemes.end(), schemes.secure_schemes.begin(),
                      schemes.secure_schemes.end());
  test_schemes.insert(test_schemes.end(),
                      schemes.empty_document_schemes.begin(),
                      schemes.empty_document_schemes.end());
  test_schemes.push_back(url::kHttpScheme);
  test_schemes.push_back(url::kHttpsScheme);
  test_schemes.push_back(url::kFileScheme);

  auto failure_reason = ChromeContentBrowserClientExtensionsPart::
      FAILURE_RESOURCE_NOT_WEB_ACCESSIBLE;
  for (auto scheme : test_schemes) {
    ChromeContentBrowserClientExtensionsPart::RecordShouldAllowOpenURLFailure(
        failure_reason, GURL(scheme + "://foo.com/"));
  }

  // There should be no unknown schemes recorded.
  uma.ExpectUniqueSample("Extensions.ShouldAllowOpenURL.Failure",
                         failure_reason, test_schemes.size());
  uma.ExpectTotalCount("Extensions.ShouldAllowOpenURL.Failure.Scheme",
                       test_schemes.size());
  uma.ExpectBucketCount("Extensions.ShouldAllowOpenURL.Failure.Scheme",
                        0 /* SCHEME_UNKNOWN */, 0);
}

// Verify that DoesOriginMatchAllURLsInWebExtent properly determines when a URL
// extent is contained entirely within a particular origin.
TEST_F(ChromeContentBrowserClientExtensionsPartTest,
       IsolatedOriginsAndHostedAppWebExtents) {
  auto does_origin_contain_extent = [](const std::string& origin,
                                       const URLPatternSet& extent) {
    return ChromeContentBrowserClientExtensionsPart::
        DoesOriginMatchAllURLsInWebExtent(url::Origin::Create(GURL(origin)),
                                          extent);
  };

  {
    URLPatternSet extent;
    AddPattern(&extent, "https://mail.google.com/foo/");
    EXPECT_TRUE(does_origin_contain_extent("https://google.com", extent));
    EXPECT_TRUE(does_origin_contain_extent("https://mail.google.com", extent));
    EXPECT_FALSE(
        does_origin_contain_extent("https://xx.mail.google.com", extent));
    EXPECT_FALSE(does_origin_contain_extent("https://www.yahoo.com", extent));
  }

  {
    URLPatternSet extent;
    AddPattern(&extent, "https://www.google.com/*");
    AddPattern(&extent, "https://www.yahoo.com/*");
    // This extent matches two different origins, and so it is broader than any
    // one particular origin.
    EXPECT_FALSE(does_origin_contain_extent("https://google.com", extent));
    EXPECT_FALSE(does_origin_contain_extent("https://www.google.com", extent));
    EXPECT_FALSE(does_origin_contain_extent("https://www.yahoo.com", extent));
  }

  {
    URLPatternSet extent;
    AddPattern(&extent, "https://mail.google.com/foo/");
    AddPattern(&extent, "https://calendar.google.com/bar/");
    AddPattern(&extent, "https://google.com/baz/qux/");
    // This extent is contained within google.com, but not in the more specific
    // subdomains of google.com.
    EXPECT_TRUE(does_origin_contain_extent("https://google.com", extent));
    EXPECT_FALSE(does_origin_contain_extent("https://mail.google.com", extent));
    EXPECT_FALSE(
        does_origin_contain_extent("https://calendar.google.com", extent));
  }

  {
    URLPatternSet extent;
    AddPattern(&extent, "*://mail.google.com/foo/");
    // TODO(alexmos): A strict scheme match with the isolated origin is
    // currently not required to keep hosted apps with scheme wildcards
    // working. See https://crbug.com/799638 and https://crbug.com/791796.
    EXPECT_TRUE(does_origin_contain_extent("http://google.com", extent));
    EXPECT_TRUE(does_origin_contain_extent("http://mail.google.com", extent));
    EXPECT_TRUE(does_origin_contain_extent("https://google.com", extent));
    EXPECT_TRUE(does_origin_contain_extent("https://mail.google.com", extent));
  }
}

}  // namespace extensions
