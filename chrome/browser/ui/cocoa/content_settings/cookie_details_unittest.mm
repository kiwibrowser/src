// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include "base/strings/sys_string_conversions.h"
#include "base/time/time.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"

#include "chrome/browser/ui/cocoa/content_settings/cookie_details.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_options.h"
#import "testing/gtest_mac.h"
#include "url/gurl.h"

namespace {

class CookiesDetailsTest : public CocoaTest {
};

TEST_F(CookiesDetailsTest, CreateForFolder) {
  base::scoped_nsobject<CocoaCookieDetails> details;
  details.reset([[CocoaCookieDetails alloc] initAsFolder]);

  EXPECT_EQ([details.get() type], kCocoaCookieDetailsTypeFolder);
}

TEST_F(CookiesDetailsTest, CreateForCookie) {
  base::scoped_nsobject<CocoaCookieDetails> details;
  GURL url("http://chromium.org");
  std::string cookieLine(
      "PHPSESSID=0123456789abcdef0123456789abcdef; path=/");
  std::unique_ptr<net::CanonicalCookie> cookie(net::CanonicalCookie::Create(
      url, cookieLine, base::Time::Now(), net::CookieOptions()));
  details.reset([[CocoaCookieDetails alloc] initWithCookie:cookie.get()
                                         canEditExpiration:NO]);

  EXPECT_EQ([details.get() type], kCocoaCookieDetailsTypeCookie);
  EXPECT_NSEQ(@"PHPSESSID", [details.get() name]);
  EXPECT_NSEQ(@"0123456789abcdef0123456789abcdef",
      [details.get() content]);
  EXPECT_NSEQ(@"chromium.org", [details.get() domain]);
  EXPECT_NSEQ(@"/", [details.get() path]);
  EXPECT_NSNE(@"", [details.get() lastModified]);
  EXPECT_NSNE(@"", [details.get() created]);
  EXPECT_NSNE(@"", [details.get() sendFor]);

  EXPECT_FALSE([details.get() shouldHideCookieDetailsView]);
  EXPECT_FALSE([details.get() shouldShowLocalStorageTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowDatabaseTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowAppCacheTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowIndexedDBTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowLocalStoragePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowDatabasePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowAppCachePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowServiceWorkerTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowCacheStorageTreeDetailsView]);
}

TEST_F(CookiesDetailsTest, CreateForTreeDatabase) {
  base::scoped_nsobject<CocoaCookieDetails> details;
  GURL origin("http://chromium.org");
  std::string database_name("sassolungo");
  std::string description("a great place to climb");
  int64_t size = 1234;
  base::Time last_modified = base::Time::Now();
  BrowsingDataDatabaseHelper::DatabaseInfo info(
      storage::DatabaseIdentifier::CreateFromOrigin(origin),
      database_name,
      description,
      size,
      last_modified);
  details.reset([[CocoaCookieDetails alloc] initWithDatabase:&info]);

  EXPECT_EQ([details.get() type], kCocoaCookieDetailsTypeTreeDatabase);
  EXPECT_NSEQ(@"a great place to climb", [details.get() databaseDescription]);
  EXPECT_NSEQ(@"1,234 B", [details.get() fileSize]);
  EXPECT_NSNE(@"", [details.get() lastModified]);

  EXPECT_TRUE([details.get() shouldHideCookieDetailsView]);
  EXPECT_FALSE([details.get() shouldShowLocalStorageTreeDetailsView]);
  EXPECT_TRUE([details.get() shouldShowDatabaseTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowAppCacheTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowIndexedDBTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowLocalStoragePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowDatabasePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowAppCachePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowServiceWorkerTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowCacheStorageTreeDetailsView]);
}

TEST_F(CookiesDetailsTest, CreateForTreeLocalStorage) {
  base::scoped_nsobject<CocoaCookieDetails> details;
  const GURL kOrigin("http://chromium.org/");
  int64_t size = 1234;
  base::Time last_modified = base::Time::Now();
  BrowsingDataLocalStorageHelper::LocalStorageInfo info(
      kOrigin, size, last_modified);
  details.reset([[CocoaCookieDetails alloc] initWithLocalStorage:&info]);

  EXPECT_EQ([details.get() type], kCocoaCookieDetailsTypeTreeLocalStorage);
  EXPECT_NSEQ(@"http://chromium.org/", [details.get() domain]);
  EXPECT_NSEQ(@"1,234 B", [details.get() fileSize]);
  EXPECT_NSNE(@"", [details.get() lastModified]);

  EXPECT_TRUE([details.get() shouldHideCookieDetailsView]);
  EXPECT_TRUE([details.get() shouldShowLocalStorageTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowDatabaseTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowAppCacheTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowIndexedDBTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowLocalStoragePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowDatabasePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowAppCachePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowServiceWorkerTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowCacheStorageTreeDetailsView]);
}

TEST_F(CookiesDetailsTest, CreateForTreeAppCache) {
  base::scoped_nsobject<CocoaCookieDetails> details;

  GURL url("http://chromium.org/stuff.manifest");
  content::AppCacheInfo info;
  info.creation_time = base::Time::Now();
  info.last_update_time = base::Time::Now();
  info.last_access_time = base::Time::Now();
  info.size=2678;
  info.manifest_url = url;
  details.reset([[CocoaCookieDetails alloc] initWithAppCacheInfo:&info]);

  EXPECT_EQ([details.get() type], kCocoaCookieDetailsTypeTreeAppCache);
  EXPECT_NSEQ(@"http://chromium.org/stuff.manifest",
              [details.get() manifestURL]);
  EXPECT_NSEQ(@"2,678 B", [details.get() fileSize]);
  EXPECT_NSNE(@"", [details.get() lastAccessed]);
  EXPECT_NSNE(@"", [details.get() created]);

  EXPECT_TRUE([details.get() shouldHideCookieDetailsView]);
  EXPECT_FALSE([details.get() shouldShowLocalStorageTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowDatabaseTreeDetailsView]);
  EXPECT_TRUE([details.get() shouldShowAppCacheTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowIndexedDBTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowLocalStoragePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowDatabasePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowAppCachePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowServiceWorkerTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowCacheStorageTreeDetailsView]);
}

TEST_F(CookiesDetailsTest, CreateForTreeIndexedDB) {
  base::scoped_nsobject<CocoaCookieDetails> details;

  GURL origin("http://moose.org/");
  int64_t size = 1234;
  base::Time last_modified = base::Time::Now();
  content::IndexedDBInfo info(origin,
                              size,
                              last_modified,
                              0);

  details.reset([[CocoaCookieDetails alloc] initWithIndexedDBInfo:&info]);

  EXPECT_EQ([details.get() type], kCocoaCookieDetailsTypeTreeIndexedDB);
  EXPECT_NSEQ(@"http://moose.org/", [details.get() domain]);
  EXPECT_NSEQ(@"1,234 B", [details.get() fileSize]);
  EXPECT_NSNE(@"", [details.get() lastModified]);

  EXPECT_TRUE([details.get() shouldHideCookieDetailsView]);
  EXPECT_FALSE([details.get() shouldShowLocalStorageTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowDatabaseTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowAppCacheTreeDetailsView]);
  EXPECT_TRUE([details.get() shouldShowIndexedDBTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowLocalStoragePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowDatabasePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowAppCachePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowServiceWorkerTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowCacheStorageTreeDetailsView]);
}

TEST_F(CookiesDetailsTest, CreateForPromptDatabase) {
  base::scoped_nsobject<CocoaCookieDetails> details;
  std::string domain("chromium.org");
  base::string16 name(base::SysNSStringToUTF16(@"wicked_name"));
  base::string16 desc(base::SysNSStringToUTF16(@"desc"));
  details.reset([[CocoaCookieDetails alloc] initWithDatabase:domain
                                                databaseName:name
                                         databaseDescription:desc
                                                    fileSize:94]);

  EXPECT_EQ([details.get() type], kCocoaCookieDetailsTypePromptDatabase);
  EXPECT_NSEQ(@"chromium.org", [details.get() domain]);
  EXPECT_NSEQ(@"wicked_name", [details.get() name]);
  EXPECT_NSEQ(@"desc", [details.get() databaseDescription]);
  EXPECT_NSEQ(@"94 B", [details.get() fileSize]);

  EXPECT_TRUE([details.get() shouldHideCookieDetailsView]);
  EXPECT_FALSE([details.get() shouldShowLocalStorageTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowDatabaseTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowAppCacheTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowIndexedDBTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowLocalStoragePromptDetailsView]);
  EXPECT_TRUE([details.get() shouldShowDatabasePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowAppCachePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowServiceWorkerTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowCacheStorageTreeDetailsView]);
}

TEST_F(CookiesDetailsTest, CreateForPromptLocalStorage) {
  base::scoped_nsobject<CocoaCookieDetails> details;
  std::string domain("chromium.org");
  base::string16 key(base::SysNSStringToUTF16(@"testKey"));
  base::string16 value(base::SysNSStringToUTF16(@"testValue"));
  details.reset([[CocoaCookieDetails alloc] initWithLocalStorage:domain
                                                             key:key
                                                           value:value]);

  EXPECT_EQ([details.get() type], kCocoaCookieDetailsTypePromptLocalStorage);
  EXPECT_NSEQ(@"chromium.org", [details.get() domain]);
  EXPECT_NSEQ(@"testKey", [details.get() localStorageKey]);
  EXPECT_NSEQ(@"testValue", [details.get() localStorageValue]);

  EXPECT_TRUE([details.get() shouldHideCookieDetailsView]);
  EXPECT_FALSE([details.get() shouldShowLocalStorageTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowDatabaseTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowAppCacheTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowIndexedDBTreeDetailsView]);
  EXPECT_TRUE([details.get() shouldShowLocalStoragePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowDatabasePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowAppCachePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowServiceWorkerTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowCacheStorageTreeDetailsView]);
}

TEST_F(CookiesDetailsTest, CreateForPromptAppCache) {
  base::scoped_nsobject<CocoaCookieDetails> details;
  std::string manifestURL("http://html5demos.com/html5demo.manifest");
  details.reset([[CocoaCookieDetails alloc]
      initWithAppCacheManifestURL:manifestURL.c_str()]);

  EXPECT_EQ([details.get() type], kCocoaCookieDetailsTypePromptAppCache);
  EXPECT_NSEQ(@"http://html5demos.com/html5demo.manifest",
              [details.get() manifestURL]);

  EXPECT_TRUE([details.get() shouldHideCookieDetailsView]);
  EXPECT_FALSE([details.get() shouldShowLocalStorageTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowDatabaseTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowAppCacheTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowIndexedDBTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowLocalStoragePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowDatabasePromptDetailsView]);
  EXPECT_TRUE([details.get() shouldShowAppCachePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowServiceWorkerTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowCacheStorageTreeDetailsView]);
}

TEST_F(CookiesDetailsTest, CreateForTreeServiceWorker) {
  base::scoped_nsobject<CocoaCookieDetails> details;

  GURL origin("https://example.com/");
  std::vector<GURL> scopes;
  scopes.push_back(GURL("https://example.com/app1/*"));
  scopes.push_back(GURL("https://example.com/app2/*"));
  content::ServiceWorkerUsageInfo info(origin,
                                       scopes);

  details.reset(
      [[CocoaCookieDetails alloc] initWithServiceWorkerUsageInfo:&info]);

  EXPECT_EQ([details.get() type], kCocoaCookieDetailsTypeTreeServiceWorker);
  EXPECT_NSEQ(@"https://example.com/", [details.get() domain]);
  EXPECT_NSEQ(@"https://example.com/app1/*,https://example.com/app2/*", [details.get() scopes]);
  EXPECT_NSEQ(@"0 B", [details.get() fileSize]);
  // TODO(jsbell): Plumb this through.
  EXPECT_NSEQ(nil, [details.get() lastModified]);

  EXPECT_TRUE([details.get() shouldHideCookieDetailsView]);
  EXPECT_FALSE([details.get() shouldShowLocalStorageTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowDatabaseTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowAppCacheTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowIndexedDBTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowLocalStoragePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowDatabasePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowAppCachePromptDetailsView]);
  EXPECT_TRUE([details.get() shouldShowServiceWorkerTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowCacheStorageTreeDetailsView]);
}

TEST_F(CookiesDetailsTest, CreateForTreeCacheStorage) {
  base::scoped_nsobject<CocoaCookieDetails> details;

  GURL origin("https://example.com/");
  int64_t size = 1234;
  base::Time last_modified = base::Time::Now();
  content::CacheStorageUsageInfo info(origin, size, last_modified);

  details.reset(
      [[CocoaCookieDetails alloc] initWithCacheStorageUsageInfo:&info]);

  EXPECT_EQ([details.get() type], kCocoaCookieDetailsTypeTreeCacheStorage);
  EXPECT_NSEQ(@"https://example.com/", [details.get() domain]);
  EXPECT_NSEQ(@"1,234 B", [details.get() fileSize]);
  EXPECT_NSNE(@"", [details.get() lastModified]);

  EXPECT_TRUE([details.get() shouldHideCookieDetailsView]);
  EXPECT_FALSE([details.get() shouldShowLocalStorageTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowDatabaseTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowAppCacheTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowIndexedDBTreeDetailsView]);
  EXPECT_FALSE([details.get() shouldShowLocalStoragePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowDatabasePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowAppCachePromptDetailsView]);
  EXPECT_FALSE([details.get() shouldShowServiceWorkerTreeDetailsView]);
  EXPECT_TRUE([details.get() shouldShowCacheStorageTreeDetailsView]);
}
}
