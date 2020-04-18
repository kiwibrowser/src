// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>
#include <vector>

#include "chrome/browser/extensions/webstore_inline_installer.h"
#include "chrome/common/extensions/webstore_install_result.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/browser/web_contents.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace extensions {

namespace {

// Wraps WebstoreInlineInstaller to provide access to domain verification
// methods for testing.
class TestWebstoreInlineInstaller : public WebstoreInlineInstaller {
 public:
  explicit TestWebstoreInlineInstaller(content::WebContents* contents,
                                       const std::string& requestor_url);

  bool TestCheckRequestorPermitted(const base::DictionaryValue& webstore_data) {
    std::string error;
    return CheckRequestorPermitted(webstore_data, &error);
  }

 protected:
  ~TestWebstoreInlineInstaller() override;
};

void TestInstallerCallback(bool success,
                           const std::string& error,
                           webstore_install::Result result) {}

TestWebstoreInlineInstaller::TestWebstoreInlineInstaller(
    content::WebContents* contents,
    const std::string& requestor_url)
    : WebstoreInlineInstaller(contents,
                              contents->GetMainFrame(),
                              "",
                              GURL(requestor_url),
                              base::Bind(&TestInstallerCallback)) {
}

TestWebstoreInlineInstaller::~TestWebstoreInlineInstaller() {}

// We inherit from ChromeRenderViewHostTestHarness only for
// CreateTestWebContents, because we need a mock WebContents to support the
// underlying WebstoreInlineInstaller in each test case.
class WebstoreInlineInstallerTest : public ChromeRenderViewHostTestHarness {
 public:
  // testing::Test
  void SetUp() override;
  void TearDown() override;

  bool TestSingleVerifiedSite(const std::string& requestor_url,
                              const std::string& verified_site);

  bool TestMultipleVerifiedSites(
      const std::string& requestor_url,
      const std::vector<std::string>& verified_sites);

 protected:
  std::unique_ptr<content::WebContents> web_contents_;
};

void WebstoreInlineInstallerTest::SetUp() {
  ChromeRenderViewHostTestHarness::SetUp();
  web_contents_ = CreateTestWebContents();
}

void WebstoreInlineInstallerTest::TearDown() {
  web_contents_.reset(NULL);
  ChromeRenderViewHostTestHarness::TearDown();
}

// Simulates a test against the verified site string from a Webstore item's
// "verified_site" manifest entry.
bool WebstoreInlineInstallerTest::TestSingleVerifiedSite(
    const std::string& requestor_url,
    const std::string& verified_site) {
  base::DictionaryValue webstore_data;
  webstore_data.SetString("verified_site", verified_site);

  scoped_refptr<TestWebstoreInlineInstaller> installer =
    new TestWebstoreInlineInstaller(web_contents_.get(), requestor_url);
  return installer->TestCheckRequestorPermitted(webstore_data);
}

// Simulates a test against a list of verified site strings from a Webstore
// item's "verified_sites" manifest entry.
bool WebstoreInlineInstallerTest::TestMultipleVerifiedSites(
    const std::string& requestor_url,
    const std::vector<std::string>& verified_sites) {
  auto sites = std::make_unique<base::ListValue>();
  for (std::vector<std::string>::const_iterator it = verified_sites.begin();
       it != verified_sites.end(); ++it) {
    sites->AppendString(*it);
  }
  base::DictionaryValue webstore_data;
  webstore_data.Set("verified_sites", std::move(sites));

  scoped_refptr<TestWebstoreInlineInstaller> installer =
    new TestWebstoreInlineInstaller(web_contents_.get(), requestor_url);
  return installer->TestCheckRequestorPermitted(webstore_data);
}

}  // namespace

TEST_F(WebstoreInlineInstallerTest, DomainVerification) {
  // Exact domain match.
  EXPECT_TRUE(TestSingleVerifiedSite("http://example.com", "example.com"));

  // The HTTPS scheme is allowed.
  EXPECT_TRUE(TestSingleVerifiedSite("https://example.com", "example.com"));

  // The file: scheme is not allowed.
  EXPECT_FALSE(TestSingleVerifiedSite("file:///example.com", "example.com"));

  // Trailing slash in URL.
  EXPECT_TRUE(TestSingleVerifiedSite("http://example.com/", "example.com"));

  // Page on the domain.
  EXPECT_TRUE(TestSingleVerifiedSite("http://example.com/page.html",
                                     "example.com"));

  // Page on a subdomain.
  EXPECT_TRUE(TestSingleVerifiedSite("http://sub.example.com/page.html",
                                     "example.com"));

  // Root domain when only a subdomain is verified.
  EXPECT_FALSE(TestSingleVerifiedSite("http://example.com/",
                                      "sub.example.com"));

  // Different subdomain when only a subdomain is verified.
  EXPECT_FALSE(TestSingleVerifiedSite("http://www.example.com/",
                                      "sub.example.com"));

  // Port matches.
  EXPECT_TRUE(TestSingleVerifiedSite("http://example.com:123/",
                                     "example.com:123"));

  // Port doesn't match.
  EXPECT_FALSE(TestSingleVerifiedSite("http://example.com:456/",
                                      "example.com:123"));

  // Port is missing in the requestor URL.
  EXPECT_FALSE(TestSingleVerifiedSite("http://example.com/",
                                      "example.com:123"));

  // Port is missing in the verified site (any port matches).
  EXPECT_TRUE(TestSingleVerifiedSite("http://example.com:123/", "example.com"));

  // Path matches.
  EXPECT_TRUE(TestSingleVerifiedSite("http://example.com/path",
                                     "example.com/path"));

  // Path doesn't match.
  EXPECT_FALSE(TestSingleVerifiedSite("http://example.com/foo",
                                      "example.com/path"));

  // Path is missing.
  EXPECT_FALSE(TestSingleVerifiedSite("http://example.com",
                                      "example.com/path"));

  // Path matches (with trailing slash).
  EXPECT_TRUE(TestSingleVerifiedSite("http://example.com/path/",
                                     "example.com/path"));

  // Path matches (is a file under the path).
  EXPECT_TRUE(TestSingleVerifiedSite("http://example.com/path/page.html",
                                     "example.com/path"));

  // Path and port match.
  EXPECT_TRUE(TestSingleVerifiedSite(
      "http://example.com:123/path/page.html", "example.com:123/path"));

  // Match specific valid schemes
  EXPECT_TRUE(TestSingleVerifiedSite("http://example.com",
                                     "http://example.com"));
  EXPECT_TRUE(TestSingleVerifiedSite("https://example.com",
                                     "https://example.com"));

  // Mismatch specific vaild schemes
  EXPECT_FALSE(TestSingleVerifiedSite("https://example.com",
                                      "http://example.com"));
  EXPECT_FALSE(TestSingleVerifiedSite("http://example.com",
                                      "https://example.com"));

  // Invalid scheme spec
  EXPECT_FALSE(TestSingleVerifiedSite("file://example.com",
                                      "file://example.com"));

  std::vector<std::string> verified_sites;
  verified_sites.push_back("foo.example.com");
  verified_sites.push_back("bar.example.com:123");
  verified_sites.push_back("example.com/unicorns");

  // Test valid examples against the site list.

  EXPECT_TRUE(TestMultipleVerifiedSites("http://foo.example.com",
                                        verified_sites));

  EXPECT_TRUE(TestMultipleVerifiedSites("http://bar.example.com:123",
                                        verified_sites));

  EXPECT_TRUE(TestMultipleVerifiedSites(
      "http://cooking.example.com/unicorns/bacon.html", verified_sites));

  // Test invalid examples against the site list.

  EXPECT_FALSE(TestMultipleVerifiedSites("http://example.com",
                                         verified_sites));

  EXPECT_FALSE(TestMultipleVerifiedSites("file://foo.example.com",
                                         verified_sites));

  EXPECT_FALSE(TestMultipleVerifiedSites("http://baz.example.com",
                                         verified_sites));

  EXPECT_FALSE(TestMultipleVerifiedSites("http://bar.example.com:456",
                                         verified_sites));
}

}  // namespace extensions
