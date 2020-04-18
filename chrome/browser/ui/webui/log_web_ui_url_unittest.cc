// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/log_web_ui_url.h"

#include "extensions/buildflags/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

TEST(LogWebUIUrlTest, ValidUrls) {
  // Typical WebUI page.
  EXPECT_TRUE(webui::LogWebUIUrl(GURL("chrome://downloads")));

  // WebUI page with a subpage.
  EXPECT_TRUE(webui::LogWebUIUrl(GURL("chrome://settings/clearBrowserData")));

  // Developer tools scheme.
  EXPECT_TRUE(webui::LogWebUIUrl(GURL("chrome-devtools://devtools")));
}

TEST(LogWebUIUrlTest, InvalidUrls) {
  // HTTP/HTTPS/FTP/etc. schemes should be ignored.
  EXPECT_FALSE(webui::LogWebUIUrl(GURL("http://google.com?q=pii")));
  EXPECT_FALSE(webui::LogWebUIUrl(GURL("https://facebook.com")));
  EXPECT_FALSE(webui::LogWebUIUrl(GURL("ftp://ftp.mysite.com")));

  // Extensions schemes should also be ignored.
  EXPECT_FALSE(webui::LogWebUIUrl(GURL(
      "chrome-extension://mfehgcgbbipciphmccgaenjidiccnmng")));
}
