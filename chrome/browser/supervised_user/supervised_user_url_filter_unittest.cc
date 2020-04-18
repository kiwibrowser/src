// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/supervised_user/supervised_user_url_filter.h"

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "chrome/browser/supervised_user/supervised_user_site_list.h"
#include "extensions/buildflags/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class SupervisedUserURLFilterTest : public ::testing::Test,
                                    public SupervisedUserURLFilter::Observer {
 public:
  SupervisedUserURLFilterTest() {
    filter_.SetDefaultFilteringBehavior(SupervisedUserURLFilter::BLOCK);
    filter_.AddObserver(this);
  }

  ~SupervisedUserURLFilterTest() override { filter_.RemoveObserver(this); }

  // SupervisedUserURLFilter::Observer:
  void OnSiteListUpdated() override { run_loop_.Quit(); }

 protected:
  bool IsURLWhitelisted(const std::string& url) {
    return filter_.GetFilteringBehaviorForURL(GURL(url)) ==
           SupervisedUserURLFilter::ALLOW;
  }

  GURL GetEmbeddedURL(const std::string& url) {
    return filter_.GetEmbeddedURL(GURL(url));
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  base::RunLoop run_loop_;
  SupervisedUserURLFilter filter_;
};

TEST_F(SupervisedUserURLFilterTest, Basic) {
  std::vector<std::string> list;
  // Allow domain and all subdomains, for any filtered scheme.
  list.push_back("google.com");
  filter_.SetFromPatternsForTesting(list);
  run_loop_.Run();

  EXPECT_TRUE(IsURLWhitelisted("http://google.com"));
  EXPECT_TRUE(IsURLWhitelisted("http://google.com/"));
  EXPECT_TRUE(IsURLWhitelisted("http://google.com/whatever"));
  EXPECT_TRUE(IsURLWhitelisted("https://google.com/"));
  EXPECT_FALSE(IsURLWhitelisted("http://notgoogle.com/"));
  EXPECT_TRUE(IsURLWhitelisted("http://mail.google.com"));
  EXPECT_TRUE(IsURLWhitelisted("http://x.mail.google.com"));
  EXPECT_TRUE(IsURLWhitelisted("https://x.mail.google.com/"));
  EXPECT_TRUE(IsURLWhitelisted("http://x.y.google.com/a/b"));
  EXPECT_FALSE(IsURLWhitelisted("http://youtube.com/"));

  EXPECT_TRUE(IsURLWhitelisted("bogus://youtube.com/"));
  EXPECT_TRUE(IsURLWhitelisted("chrome://youtube.com/"));
  EXPECT_TRUE(IsURLWhitelisted("chrome://extensions/"));
  EXPECT_TRUE(IsURLWhitelisted("chrome-extension://foo/main.html"));
  EXPECT_TRUE(IsURLWhitelisted("file:///home/chronos/user/Downloads/img.jpg"));
}

TEST_F(SupervisedUserURLFilterTest, EffectiveURL) {
  std::vector<std::string> list;
  // Allow domain and all subdomains, for any filtered scheme.
  list.push_back("example.com");
  filter_.SetFromPatternsForTesting(list);
  run_loop_.Run();

  ASSERT_TRUE(IsURLWhitelisted("http://example.com"));
  ASSERT_TRUE(IsURLWhitelisted("https://example.com"));

  // AMP Cache URLs.
  EXPECT_FALSE(IsURLWhitelisted("https://cdn.ampproject.org"));
  EXPECT_TRUE(IsURLWhitelisted("https://cdn.ampproject.org/c/example.com"));
  EXPECT_TRUE(IsURLWhitelisted("https://cdn.ampproject.org/c/www.example.com"));
  EXPECT_TRUE(
      IsURLWhitelisted("https://cdn.ampproject.org/c/example.com/path"));
  EXPECT_TRUE(IsURLWhitelisted("https://cdn.ampproject.org/c/s/example.com"));
  EXPECT_FALSE(IsURLWhitelisted("https://cdn.ampproject.org/c/other.com"));

  EXPECT_FALSE(IsURLWhitelisted("https://sub.cdn.ampproject.org"));
  EXPECT_TRUE(IsURLWhitelisted("https://sub.cdn.ampproject.org/c/example.com"));
  EXPECT_TRUE(
      IsURLWhitelisted("https://sub.cdn.ampproject.org/c/www.example.com"));
  EXPECT_TRUE(
      IsURLWhitelisted("https://sub.cdn.ampproject.org/c/example.com/path"));
  EXPECT_TRUE(
      IsURLWhitelisted("https://sub.cdn.ampproject.org/c/s/example.com"));
  EXPECT_FALSE(IsURLWhitelisted("https://sub.cdn.ampproject.org/c/other.com"));

  // Google AMP viewer URLs.
  EXPECT_FALSE(IsURLWhitelisted("https://www.google.com"));
  EXPECT_FALSE(IsURLWhitelisted("https://www.google.com/amp/"));
  EXPECT_TRUE(IsURLWhitelisted("https://www.google.com/amp/example.com"));
  EXPECT_TRUE(IsURLWhitelisted("https://www.google.com/amp/www.example.com"));
  EXPECT_TRUE(IsURLWhitelisted("https://www.google.com/amp/s/example.com"));
  EXPECT_TRUE(
      IsURLWhitelisted("https://www.google.com/amp/s/example.com/path"));
  EXPECT_FALSE(IsURLWhitelisted("https://www.google.com/amp/other.com"));

  // Google web cache URLs.
  EXPECT_FALSE(IsURLWhitelisted("https://webcache.googleusercontent.com"));
  EXPECT_FALSE(
      IsURLWhitelisted("https://webcache.googleusercontent.com/search"));
  EXPECT_FALSE(IsURLWhitelisted(
      "https://webcache.googleusercontent.com/search?q=example.com"));
  EXPECT_TRUE(IsURLWhitelisted(
      "https://webcache.googleusercontent.com/search?q=cache:example.com"));
  EXPECT_TRUE(
      IsURLWhitelisted("https://webcache.googleusercontent.com/"
                       "search?q=cache:example.com+search_query"));
  EXPECT_TRUE(
      IsURLWhitelisted("https://webcache.googleusercontent.com/"
                       "search?q=cache:123456789-01:example.com+search_query"));
  EXPECT_FALSE(IsURLWhitelisted(
      "https://webcache.googleusercontent.com/search?q=cache:other.com"));
  EXPECT_FALSE(
      IsURLWhitelisted("https://webcache.googleusercontent.com/"
                       "search?q=cache:other.com+example.com"));
  EXPECT_FALSE(
      IsURLWhitelisted("https://webcache.googleusercontent.com/"
                       "search?q=cache:123456789-01:other.com+example.com"));

  // Google Translate URLs.
  EXPECT_FALSE(IsURLWhitelisted("https://translate.google.com"));
  EXPECT_FALSE(IsURLWhitelisted("https://translate.googleusercontent.com"));
  EXPECT_TRUE(
      IsURLWhitelisted("https://translate.google.com/translate?u=example.com"));
  EXPECT_TRUE(IsURLWhitelisted(
      "https://translate.googleusercontent.com/translate?u=example.com"));
  EXPECT_TRUE(IsURLWhitelisted(
      "https://translate.google.com/translate?u=www.example.com"));
  EXPECT_TRUE(IsURLWhitelisted(
      "https://translate.google.com/translate?u=https://example.com"));
  EXPECT_FALSE(
      IsURLWhitelisted("https://translate.google.com/translate?u=other.com"));
}

TEST_F(SupervisedUserURLFilterTest, Inactive) {
  filter_.SetDefaultFilteringBehavior(SupervisedUserURLFilter::ALLOW);

  std::vector<std::string> list;
  list.push_back("google.com");
  filter_.SetFromPatternsForTesting(list);
  run_loop_.Run();

  // If the filter is inactive, every URL should be whitelisted.
  EXPECT_TRUE(IsURLWhitelisted("http://google.com"));
  EXPECT_TRUE(IsURLWhitelisted("https://www.example.com"));
}

TEST_F(SupervisedUserURLFilterTest, Scheme) {
  std::vector<std::string> list;
  // Filter only http, ftp and ws schemes.
  list.push_back("http://secure.com");
  list.push_back("ftp://secure.com");
  list.push_back("ws://secure.com");
  filter_.SetFromPatternsForTesting(list);
  run_loop_.Run();

  EXPECT_TRUE(IsURLWhitelisted("http://secure.com"));
  EXPECT_TRUE(IsURLWhitelisted("http://secure.com/whatever"));
  EXPECT_TRUE(IsURLWhitelisted("ftp://secure.com/"));
  EXPECT_TRUE(IsURLWhitelisted("ws://secure.com"));
  EXPECT_FALSE(IsURLWhitelisted("https://secure.com/"));
  EXPECT_FALSE(IsURLWhitelisted("wss://secure.com"));
  EXPECT_TRUE(IsURLWhitelisted("http://www.secure.com"));
  EXPECT_FALSE(IsURLWhitelisted("https://www.secure.com"));
  EXPECT_FALSE(IsURLWhitelisted("wss://www.secure.com"));
}

TEST_F(SupervisedUserURLFilterTest, Path) {
  std::vector<std::string> list;
  // Filter only a certain path prefix.
  list.push_back("path.to/ruin");
  filter_.SetFromPatternsForTesting(list);
  run_loop_.Run();

  EXPECT_TRUE(IsURLWhitelisted("http://path.to/ruin"));
  EXPECT_TRUE(IsURLWhitelisted("https://path.to/ruin"));
  EXPECT_TRUE(IsURLWhitelisted("http://path.to/ruins"));
  EXPECT_TRUE(IsURLWhitelisted("http://path.to/ruin/signup"));
  EXPECT_TRUE(IsURLWhitelisted("http://www.path.to/ruin"));
  EXPECT_FALSE(IsURLWhitelisted("http://path.to/fortune"));
}

TEST_F(SupervisedUserURLFilterTest, PathAndScheme) {
  std::vector<std::string> list;
  // Filter only a certain path prefix and scheme.
  list.push_back("https://s.aaa.com/path");
  filter_.SetFromPatternsForTesting(list);
  run_loop_.Run();

  EXPECT_TRUE(IsURLWhitelisted("https://s.aaa.com/path"));
  EXPECT_TRUE(IsURLWhitelisted("https://s.aaa.com/path/bbb"));
  EXPECT_FALSE(IsURLWhitelisted("http://s.aaa.com/path"));
  EXPECT_FALSE(IsURLWhitelisted("https://aaa.com/path"));
  EXPECT_FALSE(IsURLWhitelisted("https://x.aaa.com/path"));
  EXPECT_FALSE(IsURLWhitelisted("https://s.aaa.com/bbb"));
  EXPECT_FALSE(IsURLWhitelisted("https://s.aaa.com/"));
}

TEST_F(SupervisedUserURLFilterTest, Host) {
  std::vector<std::string> list;
  // Filter only a certain hostname, without subdomains.
  list.push_back(".www.example.com");
  filter_.SetFromPatternsForTesting(list);
  run_loop_.Run();

  EXPECT_TRUE(IsURLWhitelisted("http://www.example.com"));
  EXPECT_FALSE(IsURLWhitelisted("http://example.com"));
  EXPECT_FALSE(IsURLWhitelisted("http://subdomain.example.com"));
}

TEST_F(SupervisedUserURLFilterTest, IPAddress) {
  std::vector<std::string> list;
  // Filter an ip address.
  list.push_back("123.123.123.123");
  filter_.SetFromPatternsForTesting(list);
  run_loop_.Run();

  EXPECT_TRUE(IsURLWhitelisted("http://123.123.123.123/"));
  EXPECT_FALSE(IsURLWhitelisted("http://123.123.123.124/"));
}

TEST_F(SupervisedUserURLFilterTest, Canonicalization) {
  // We assume that the hosts and URLs are already canonicalized.
  std::map<std::string, bool> hosts;
  hosts["www.moose.org"] = true;
  hosts["www.xn--n3h.net"] = true;
  std::map<GURL, bool> urls;
  urls[GURL("http://www.example.com/foo/")] = true;
  urls[GURL("http://www.example.com/%C3%85t%C3%B8mstr%C3%B6m")] = true;
  filter_.SetManualHosts(std::move(hosts));
  filter_.SetManualURLs(std::move(urls));

  // Base cases.
  EXPECT_TRUE(IsURLWhitelisted("http://www.example.com/foo/"));
  EXPECT_TRUE(IsURLWhitelisted(
      "http://www.example.com/%C3%85t%C3%B8mstr%C3%B6m"));

  // Verify that non-URI characters are escaped.
  EXPECT_TRUE(IsURLWhitelisted(
      "http://www.example.com/\xc3\x85t\xc3\xb8mstr\xc3\xb6m"));

  // Verify that unnecessary URI escapes are unescaped.
  EXPECT_TRUE(IsURLWhitelisted("http://www.example.com/%66%6F%6F/"));

  // Verify that the default port are removed.
  EXPECT_TRUE(IsURLWhitelisted("http://www.example.com:80/foo/"));

  // Verify that scheme and hostname are lowercased.
  EXPECT_TRUE(IsURLWhitelisted("htTp://wWw.eXamPle.com/foo/"));
  EXPECT_TRUE(IsURLWhitelisted("HttP://WwW.mOOsE.orG/blurp/"));

  // Verify that UTF-8 in hostnames are converted to punycode.
  EXPECT_TRUE(IsURLWhitelisted("http://www.\xe2\x98\x83\x0a.net/bla/"));

  // Verify that query and ref are stripped.
  EXPECT_TRUE(IsURLWhitelisted("http://www.example.com/foo/?bar=baz#ref"));
}

TEST_F(SupervisedUserURLFilterTest, HasFilteredScheme) {
  EXPECT_TRUE(
      SupervisedUserURLFilter::HasFilteredScheme(GURL("http://example.com")));
  EXPECT_TRUE(
      SupervisedUserURLFilter::HasFilteredScheme(GURL("https://example.com")));
  EXPECT_TRUE(
      SupervisedUserURLFilter::HasFilteredScheme(GURL("ftp://example.com")));
  EXPECT_TRUE(
      SupervisedUserURLFilter::HasFilteredScheme(GURL("gopher://example.com")));
  EXPECT_TRUE(
      SupervisedUserURLFilter::HasFilteredScheme(GURL("ws://example.com")));
  EXPECT_TRUE(
      SupervisedUserURLFilter::HasFilteredScheme(GURL("wss://example.com")));

  EXPECT_FALSE(
      SupervisedUserURLFilter::HasFilteredScheme(GURL("file://example.com")));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HasFilteredScheme(
          GURL("filesystem://80cols.com")));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HasFilteredScheme(GURL("chrome://example.com")));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HasFilteredScheme(GURL("wtf://example.com")));
}

TEST_F(SupervisedUserURLFilterTest, HostMatchesPattern) {
  EXPECT_TRUE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.com",
                                                  "*.google.com"));
  EXPECT_TRUE(
      SupervisedUserURLFilter::HostMatchesPattern("google.com",
                                                  "*.google.com"));
  EXPECT_TRUE(
      SupervisedUserURLFilter::HostMatchesPattern("accounts.google.com",
                                                  "*.google.com"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.de",
                                                  "*.google.com"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("notgoogle.com",
                                                  "*.google.com"));


  EXPECT_TRUE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.com",
                                                  "www.google.*"));
  EXPECT_TRUE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.de",
                                                  "www.google.*"));
  EXPECT_TRUE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.co.uk",
                                                  "www.google.*"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.blogspot.com",
                                                  "www.google.*"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google",
                                                  "www.google.*"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("google.com",
                                                  "www.google.*"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("mail.google.com",
                                                  "www.google.*"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.googleplex.com",
                                                  "www.google.*"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.googleco.uk",
                                                  "www.google.*"));


  EXPECT_TRUE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.com",
                                                  "*.google.*"));
  EXPECT_TRUE(
      SupervisedUserURLFilter::HostMatchesPattern("google.com",
                                                  "*.google.*"));
  EXPECT_TRUE(
      SupervisedUserURLFilter::HostMatchesPattern("accounts.google.com",
                                                  "*.google.*"));
  EXPECT_TRUE(
      SupervisedUserURLFilter::HostMatchesPattern("mail.google.com",
                                                  "*.google.*"));
  EXPECT_TRUE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.de",
                                                  "*.google.*"));
  EXPECT_TRUE(
      SupervisedUserURLFilter::HostMatchesPattern("google.de",
                                                  "*.google.*"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("google.blogspot.com",
                                                  "*.google.*"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("google", "*.google.*"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("notgoogle.com",
                                                  "*.google.*"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.googleplex.com",
                                                  "*.google.*"));

  // Now test a few invalid patterns. They should never match.
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.com", ""));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.com", "."));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.com", "*"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.com", ".*"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.com", "*."));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.com", "*.*"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google..com", "*..*"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.com", "*.*.com"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.com", "www.*.*"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.com",
                                                  "*.goo.*le.*"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.com",
                                                  "*google*"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.com",
                                                  "www.*.google.com"));
}

TEST_F(SupervisedUserURLFilterTest, Patterns) {
  std::map<std::string, bool> hosts;

  // Initally, the second rule is ignored because has the same value as the
  // default (block). When we change the default to allow, the first rule is
  // ignored instead.
  hosts["*.google.com"] = true;
  hosts["www.google.*"] = false;

  hosts["accounts.google.com"] = false;
  hosts["mail.google.com"] = true;
  filter_.SetManualHosts(std::move(hosts));

  // Initially, the default filtering behavior is BLOCK.
  EXPECT_TRUE(IsURLWhitelisted("http://www.google.com/foo/"));
  EXPECT_FALSE(IsURLWhitelisted("http://accounts.google.com/bar/"));
  EXPECT_FALSE(IsURLWhitelisted("http://www.google.co.uk/blurp/"));
  EXPECT_TRUE(IsURLWhitelisted("http://mail.google.com/moose/"));

  filter_.SetDefaultFilteringBehavior(SupervisedUserURLFilter::ALLOW);
  EXPECT_FALSE(IsURLWhitelisted("http://www.google.com/foo/"));
  EXPECT_FALSE(IsURLWhitelisted("http://accounts.google.com/bar/"));
  EXPECT_FALSE(IsURLWhitelisted("http://www.google.co.uk/blurp/"));
  EXPECT_TRUE(IsURLWhitelisted("http://mail.google.com/moose/"));
}

TEST_F(SupervisedUserURLFilterTest, WhitelistsPatterns) {
  std::vector<std::string> patterns1;
  patterns1.push_back("google.com");
  patterns1.push_back("example.com");

  std::vector<std::string> patterns2;
  patterns2.push_back("secure.com");
  patterns2.push_back("example.com");

  const std::string id1 = "ID1";
  const std::string id2 = "ID2";
  const base::string16 title1 = base::ASCIIToUTF16("Title 1");
  const base::string16 title2 = base::ASCIIToUTF16("Title 2");
  const std::vector<std::string> hostname_hashes;
  const GURL entry_point("https://entry.com");

  scoped_refptr<SupervisedUserSiteList> site_list1 = base::WrapRefCounted(
      new SupervisedUserSiteList(id1, title1, entry_point, base::FilePath(),
                                 patterns1, hostname_hashes));
  scoped_refptr<SupervisedUserSiteList> site_list2 = base::WrapRefCounted(
      new SupervisedUserSiteList(id2, title2, entry_point, base::FilePath(),
                                 patterns2, hostname_hashes));

  std::vector<scoped_refptr<SupervisedUserSiteList>> site_lists;
  site_lists.push_back(site_list1);
  site_lists.push_back(site_list2);

  filter_.SetFromSiteListsForTesting(site_lists);
  filter_.SetDefaultFilteringBehavior(SupervisedUserURLFilter::BLOCK);
  run_loop_.Run();

  std::map<std::string, base::string16> expected_whitelists;
  expected_whitelists[id1] = title1;
  expected_whitelists[id2] = title2;

  std::map<std::string, base::string16> actual_whitelists =
      filter_.GetMatchingWhitelistTitles(GURL("https://example.com"));
  ASSERT_EQ(expected_whitelists, actual_whitelists);

  expected_whitelists.erase(id2);

  actual_whitelists =
      filter_.GetMatchingWhitelistTitles(GURL("https://google.com"));
  ASSERT_EQ(expected_whitelists, actual_whitelists);
}

TEST_F(SupervisedUserURLFilterTest, WhitelistsHostnameHashes) {
  std::vector<std::string> patterns1;
  patterns1.push_back("google.com");
  patterns1.push_back("example.com");

  std::vector<std::string> patterns2;
  patterns2.push_back("secure.com");
  patterns2.push_back("example.com");

  std::vector<std::string> patterns3;

  std::vector<std::string> hostname_hashes1;
  std::vector<std::string> hostname_hashes2;
  std::vector<std::string> hostname_hashes3;
  // example.com
  hostname_hashes3.push_back("0caaf24ab1a0c33440c06afe99df986365b0781f");
  // secure.com
  hostname_hashes3.push_back("529597fa818be828ffc7b59763fb2b185f419fc5");

  const std::string id1 = "ID1";
  const std::string id2 = "ID2";
  const std::string id3 = "ID3";
  const base::string16 title1 = base::ASCIIToUTF16("Title 1");
  const base::string16 title2 = base::ASCIIToUTF16("Title 2");
  const base::string16 title3 = base::ASCIIToUTF16("Title 3");
  const GURL entry_point("https://entry.com");

  scoped_refptr<SupervisedUserSiteList> site_list1 = base::WrapRefCounted(
      new SupervisedUserSiteList(id1, title1, entry_point, base::FilePath(),
                                 patterns1, hostname_hashes1));
  scoped_refptr<SupervisedUserSiteList> site_list2 = base::WrapRefCounted(
      new SupervisedUserSiteList(id2, title2, entry_point, base::FilePath(),
                                 patterns2, hostname_hashes2));
  scoped_refptr<SupervisedUserSiteList> site_list3 = base::WrapRefCounted(
      new SupervisedUserSiteList(id3, title3, entry_point, base::FilePath(),
                                 patterns3, hostname_hashes3));

  std::vector<scoped_refptr<SupervisedUserSiteList>> site_lists;
  site_lists.push_back(site_list1);
  site_lists.push_back(site_list2);
  site_lists.push_back(site_list3);

  filter_.SetFromSiteListsForTesting(site_lists);
  filter_.SetDefaultFilteringBehavior(SupervisedUserURLFilter::BLOCK);
  run_loop_.Run();

  std::map<std::string, base::string16> expected_whitelists;
  expected_whitelists[id1] = title1;
  expected_whitelists[id2] = title2;
  expected_whitelists[id3] = title3;

  std::map<std::string, base::string16> actual_whitelists =
      filter_.GetMatchingWhitelistTitles(GURL("http://example.com"));
  ASSERT_EQ(expected_whitelists, actual_whitelists);

  expected_whitelists.erase(id1);

  actual_whitelists =
      filter_.GetMatchingWhitelistTitles(GURL("https://secure.com"));
  ASSERT_EQ(expected_whitelists, actual_whitelists);
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
TEST_F(SupervisedUserURLFilterTest, ChromeWebstoreDownloadsAreAlwaysAllowed) {
  // When installing an extension from Chrome Webstore, it tries to download the
  // crx file from "https://clients2.google.com/service/update2/", which
  // redirects to "https://clients2.googleusercontent.com/crx/blobs/"
  // or "https://chrome.google.com/webstore/download/".
  // All URLs should be whitelisted regardless from the default filtering
  // behavior.
  GURL crx_download_url1(
      "https://clients2.google.com/service/update2/"
      "crx?response=redirect&os=linux&arch=x64&nacl_arch=x86-64&prod="
      "chromiumcrx&prodchannel=&prodversion=55.0.2882.0&lang=en-US&x=id%"
      "3Dciniambnphakdoflgeamacamhfllbkmo%26installsource%3Dondemand%26uc");
  GURL crx_download_url2(
      "https://clients2.googleusercontent.com/crx/blobs/"
      "QgAAAC6zw0qH2DJtnXe8Z7rUJP1iCQF099oik9f2ErAYeFAX7_"
      "CIyrNH5qBru1lUSBNvzmjILCGwUjcIBaJqxgegSNy2melYqfodngLxKtHsGBehAMZSmuWSg6"
      "FupAcPS3Ih6NSVCOB9KNh6Mw/extension_2_0.crx");
  GURL crx_download_url3(
      "https://chrome.google.com/webstore/download/"
      "QgAAAC6zw0qH2DJtnXe8Z7rUJP1iCQF099oik9f2ErAYeFAX7_"
      "CIyrNH5qBru1lUSBNvzmjILCGwUjcIBaJqxgegSNy2melYqfodngLxKtHsGBehAMZSmuWSg6"
      "FupAcPS3Ih6NSVCOB9KNh6Mw/extension_2_0.crx");

  filter_.SetDefaultFilteringBehavior(SupervisedUserURLFilter::BLOCK);
  EXPECT_EQ(SupervisedUserURLFilter::ALLOW,
            filter_.GetFilteringBehaviorForURL(crx_download_url1));
  EXPECT_EQ(SupervisedUserURLFilter::ALLOW,
            filter_.GetFilteringBehaviorForURL(crx_download_url2));
  EXPECT_EQ(SupervisedUserURLFilter::ALLOW,
            filter_.GetFilteringBehaviorForURL(crx_download_url3));

  // Set explicit host rules to block those website, and make sure the
  // update URLs still work.
  std::map<std::string, bool> hosts;
  hosts["clients2.google.com"] = false;
  hosts["clients2.googleusercontent.com"] = false;
  filter_.SetManualHosts(std::move(hosts));
  filter_.SetDefaultFilteringBehavior(SupervisedUserURLFilter::ALLOW);
  EXPECT_EQ(SupervisedUserURLFilter::ALLOW,
            filter_.GetFilteringBehaviorForURL(crx_download_url1));
  EXPECT_EQ(SupervisedUserURLFilter::ALLOW,
            filter_.GetFilteringBehaviorForURL(crx_download_url2));
  EXPECT_EQ(SupervisedUserURLFilter::ALLOW,
            filter_.GetFilteringBehaviorForURL(crx_download_url3));
}
#endif

TEST_F(SupervisedUserURLFilterTest, GoogleFamiliesAlwaysAllowed) {
  filter_.SetDefaultFilteringBehavior(SupervisedUserURLFilter::BLOCK);
  EXPECT_TRUE(IsURLWhitelisted("https://families.google.com/"));
  EXPECT_TRUE(IsURLWhitelisted("https://families.google.com"));
  EXPECT_TRUE(IsURLWhitelisted("https://families.google.com/something"));
  EXPECT_TRUE(IsURLWhitelisted("http://families.google.com/"));
  EXPECT_FALSE(IsURLWhitelisted("https://families.google.com:8080/"));
  EXPECT_FALSE(IsURLWhitelisted("https://subdomain.families.google.com/"));
}

TEST_F(SupervisedUserURLFilterTest, GetEmbeddedURLAmpCache) {
  // Base case.
  EXPECT_EQ(GURL("http://example.com"),
            GetEmbeddedURL("https://cdn.ampproject.org/c/example.com"));
  // "s/" means "use https".
  EXPECT_EQ(GURL("https://example.com"),
            GetEmbeddedURL("https://cdn.ampproject.org/c/s/example.com"));
  // With path and query. Fragment is not extracted.
  EXPECT_EQ(GURL("https://example.com/path/to/file.html?q=asdf"),
            GetEmbeddedURL("https://cdn.ampproject.org/c/s/example.com/path/to/"
                           "file.html?q=asdf#baz"));

  // Different host is not supported.
  EXPECT_EQ(GURL(), GetEmbeddedURL("https://www.ampproject.org/c/example.com"));
  // Different TLD is not supported.
  EXPECT_EQ(GURL(), GetEmbeddedURL("https://cdn.ampproject.com/c/example.com"));
  // Content type ("c/") is missing.
  EXPECT_EQ(GURL(), GetEmbeddedURL("https://cdn.ampproject.org/example.com"));
  // Content type is mis-formatted, must be a single character.
  EXPECT_EQ(GURL(),
            GetEmbeddedURL("https://cdn.ampproject.org/cd/example.com"));
}

TEST_F(SupervisedUserURLFilterTest, GetEmbeddedURLGoogleAmpViewer) {
  // Base case.
  EXPECT_EQ(GURL("http://example.com"),
            GetEmbeddedURL("https://www.google.com/amp/example.com"));
  // "s/" means "use https".
  EXPECT_EQ(GURL("https://example.com"),
            GetEmbeddedURL("https://www.google.com/amp/s/example.com"));
  // Different Google TLDs are supported.
  EXPECT_EQ(GURL("http://example.com"),
            GetEmbeddedURL("https://www.google.de/amp/example.com"));
  EXPECT_EQ(GURL("http://example.com"),
            GetEmbeddedURL("https://www.google.co.uk/amp/example.com"));
  // With path.
  EXPECT_EQ(GURL("http://example.com/path"),
            GetEmbeddedURL("https://www.google.com/amp/example.com/path"));
  // Query is *not* part of the embedded URL.
  EXPECT_EQ(
      GURL("http://example.com/path"),
      GetEmbeddedURL("https://www.google.com/amp/example.com/path?q=baz"));
  // Query and fragment in percent-encoded form *are* part of the embedded URL.
  EXPECT_EQ(
      GURL("http://example.com/path?q=foo#bar"),
      GetEmbeddedURL(
          "https://www.google.com/amp/example.com/path%3fq=foo%23bar?q=baz"));
  // "/" may also be percent-encoded.
  EXPECT_EQ(GURL("http://example.com/path?q=foo#bar"),
            GetEmbeddedURL("https://www.google.com/amp/"
                           "example.com%2fpath%3fq=foo%23bar?q=baz"));

  // Missing "amp/".
  EXPECT_EQ(GURL(), GetEmbeddedURL("https://www.google.com/example.com"));
  // Path component before the "amp/".
  EXPECT_EQ(GURL(),
            GetEmbeddedURL("https://www.google.com/foo/amp/example.com"));
  // Different host.
  EXPECT_EQ(GURL(), GetEmbeddedURL("https://www.other.com/amp/example.com"));
  // Different subdomain.
  EXPECT_EQ(GURL(), GetEmbeddedURL("https://mail.google.com/amp/example.com"));
  // Invalid TLD.
  EXPECT_EQ(GURL(), GetEmbeddedURL("https://www.google.nope/amp/example.com"));
}

TEST_F(SupervisedUserURLFilterTest, GetEmbeddedURLGoogleWebCache) {
  // Base case.
  EXPECT_EQ(GURL("http://example.com"),
            GetEmbeddedURL("https://webcache.googleusercontent.com/"
                           "search?q=cache:ABCDEFGHI-JK:example.com/"));
  // With search query.
  EXPECT_EQ(
      GURL("http://example.com"),
      GetEmbeddedURL("https://webcache.googleusercontent.com/"
                     "search?q=cache:ABCDEFGHI-JK:example.com/+search_query"));
  // Without fingerprint.
  EXPECT_EQ(GURL("http://example.com"),
            GetEmbeddedURL("https://webcache.googleusercontent.com/"
                           "search?q=cache:example.com/"));
  // With search query, without fingerprint.
  EXPECT_EQ(GURL("http://example.com"),
            GetEmbeddedURL("https://webcache.googleusercontent.com/"
                           "search?q=cache:example.com/+search_query"));
  // Query params other than "q=" don't matter.
  EXPECT_EQ(GURL("http://example.com"),
            GetEmbeddedURL("https://webcache.googleusercontent.com/"
                           "search?a=b&q=cache:example.com/&c=d"));
  // With scheme.
  EXPECT_EQ(GURL("http://example.com"),
            GetEmbeddedURL("https://webcache.googleusercontent.com/"
                           "search?q=cache:http://example.com/"));
  // Preserve https.
  EXPECT_EQ(GURL("https://example.com"),
            GetEmbeddedURL("https://webcache.googleusercontent.com/"
                           "search?q=cache:https://example.com/"));

  // Wrong host.
  EXPECT_EQ(GURL(), GetEmbeddedURL("https://www.googleusercontent.com/"
                                   "search?q=cache:example.com/"));
  // Wrong path.
  EXPECT_EQ(GURL(), GetEmbeddedURL("https://webcache.googleusercontent.com/"
                                   "path?q=cache:example.com/"));
  EXPECT_EQ(GURL(), GetEmbeddedURL("https://webcache.googleusercontent.com/"
                                   "path/search?q=cache:example.com/"));
  // Missing "cache:".
  EXPECT_EQ(GURL(), GetEmbeddedURL("https://webcache.googleusercontent.com/"
                                   "search?q=example.com"));
  // Wrong fingerprint.
  EXPECT_EQ(GURL(), GetEmbeddedURL("https://webcache.googleusercontent.com/"
                                   "search?q=cache:123:example.com/"));
  // Wrong query param.
  EXPECT_EQ(GURL(), GetEmbeddedURL("https://webcache.googleusercontent.com/"
                                   "search?a=cache:example.com/"));
  // Invalid scheme.
  EXPECT_EQ(GURL(), GetEmbeddedURL("https://webcache.googleusercontent.com/"
                                   "search?q=cache:abc://example.com/"));
}

TEST_F(SupervisedUserURLFilterTest, GetEmbeddedURLTranslate) {
  // Base case.
  EXPECT_EQ(GURL("http://example.com"),
            GetEmbeddedURL("https://translate.google.com/path?u=example.com"));
  // Different TLD.
  EXPECT_EQ(GURL("http://example.com"),
            GetEmbeddedURL("https://translate.google.de/path?u=example.com"));
  // Alternate base URL.
  EXPECT_EQ(GURL("http://example.com"),
            GetEmbeddedURL(
                "https://translate.googleusercontent.com/path?u=example.com"));
  // With scheme.
  EXPECT_EQ(
      GURL("http://example.com"),
      GetEmbeddedURL("https://translate.google.com/path?u=http://example.com"));
  // With https scheme.
  EXPECT_EQ(GURL("https://example.com"),
            GetEmbeddedURL(
                "https://translate.google.com/path?u=https://example.com"));
  // With other parameters.
  EXPECT_EQ(
      GURL("http://example.com"),
      GetEmbeddedURL(
          "https://translate.google.com/path?a=asdf&u=example.com&b=fdsa"));

  // Different subdomain is not supported.
  EXPECT_EQ(GURL(), GetEmbeddedURL(
                        "https://translate.foo.google.com/path?u=example.com"));
  EXPECT_EQ(GURL(), GetEmbeddedURL(
                        "https://translate.www.google.com/path?u=example.com"));
  EXPECT_EQ(
      GURL(),
      GetEmbeddedURL("https://translate.google.google.com/path?u=example.com"));
  EXPECT_EQ(GURL(), GetEmbeddedURL(
                        "https://foo.translate.google.com/path?u=example.com"));
  EXPECT_EQ(GURL(),
            GetEmbeddedURL("https://translate2.google.com/path?u=example.com"));
  EXPECT_EQ(GURL(),
            GetEmbeddedURL(
                "https://translate2.googleusercontent.com/path?u=example.com"));
  // Different TLD is not supported for googleusercontent.
  EXPECT_EQ(GURL(),
            GetEmbeddedURL(
                "https://translate.googleusercontent.de/path?u=example.com"));
  // Query parameter ("u=...") is missing.
  EXPECT_EQ(GURL(),
            GetEmbeddedURL("https://translate.google.com/path?t=example.com"));
}
