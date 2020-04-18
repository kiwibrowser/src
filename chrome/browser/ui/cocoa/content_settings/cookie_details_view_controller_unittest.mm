// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/sys_string_conversions.h"
#include "base/time/time.h"
#include "chrome/browser/ui/cocoa/content_settings/cookie_details.h"
#include "chrome/browser/ui/cocoa/content_settings/cookie_details_view_controller.h"
#include "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_options.h"

namespace {

class CookieDetailsViewControllerTest : public CocoaTest {
};

static CocoaCookieDetails* CreateTestCookieDetails(BOOL canEditExpiration) {
  GURL url("http://chromium.org");
  std::string cookieLine(
      "PHPSESSID=0123456789abcdef0123456789abcdef; path=/");
  std::unique_ptr<net::CanonicalCookie> cookie(net::CanonicalCookie::Create(
      url, cookieLine, base::Time::Now(), net::CookieOptions()));
  CocoaCookieDetails* details = [CocoaCookieDetails alloc];
  [details initWithCookie:cookie.get()
        canEditExpiration:canEditExpiration];
  return [details autorelease];
}

static CookiePromptContentDetailsAdapter* CreateCookieTestContent(
    BOOL canEditExpiration) {
  CocoaCookieDetails* details = CreateTestCookieDetails(canEditExpiration);
  return [[[CookiePromptContentDetailsAdapter alloc] initWithDetails:details]
      autorelease];
}

static CocoaCookieDetails* CreateTestDatabaseDetails() {
  std::string domain("chromium.org");
  base::string16 name(base::SysNSStringToUTF16(@"wicked_name"));
  base::string16 desc(base::SysNSStringToUTF16(@"wicked_desc"));
  CocoaCookieDetails* details = [CocoaCookieDetails alloc];
  [details initWithDatabase:domain
               databaseName:name
        databaseDescription:desc
                   fileSize:2222];
  return [details autorelease];
}

static CookiePromptContentDetailsAdapter* CreateDatabaseTestContent() {
  CocoaCookieDetails* details = CreateTestDatabaseDetails();
  return [[[CookiePromptContentDetailsAdapter alloc] initWithDetails:details]
          autorelease];
}

TEST_F(CookieDetailsViewControllerTest, Create) {
  base::scoped_nsobject<CookieDetailsViewController> detailsViewController(
      [[CookieDetailsViewController alloc] init]);
}

TEST_F(CookieDetailsViewControllerTest, ShrinkToFit) {
  base::scoped_nsobject<CookieDetailsViewController> detailsViewController(
      [[CookieDetailsViewController alloc] init]);
  base::scoped_nsobject<CookiePromptContentDetailsAdapter> adapter(
      [CreateDatabaseTestContent() retain]);
  [detailsViewController.get() setContentObject:adapter.get()];
  NSRect beforeFrame = [[detailsViewController.get() view] frame];
  [detailsViewController.get() shrinkViewToFit];
  NSRect afterFrame = [[detailsViewController.get() view] frame];

  EXPECT_TRUE(afterFrame.size.height < beforeFrame.size.width);
}

TEST_F(CookieDetailsViewControllerTest, ExpirationEditability) {
  base::scoped_nsobject<CookieDetailsViewController> detailsViewController(
      [[CookieDetailsViewController alloc] init]);
  [detailsViewController view];
  base::scoped_nsobject<CookiePromptContentDetailsAdapter> adapter(
      [CreateCookieTestContent(YES) retain]);
  [detailsViewController.get() setContentObject:adapter.get()];

  EXPECT_FALSE([detailsViewController.get() hasExpiration]);
  [detailsViewController.get() setCookieHasExplicitExpiration:adapter.get()];
  EXPECT_TRUE([detailsViewController.get() hasExpiration]);
  [detailsViewController.get()
      setCookieDoesntHaveExplicitExpiration:adapter.get()];
  EXPECT_FALSE([detailsViewController.get() hasExpiration]);
}

}  // namespace
