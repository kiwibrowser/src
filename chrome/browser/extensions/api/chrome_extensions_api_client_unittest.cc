// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/chrome_extensions_api_client.h"

#include "base/macros.h"
#include "chrome/common/webui_url_constants.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "extensions/browser/api/web_request/web_request_info.h"
#include "google_apis/gaia/gaia_urls.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace extensions {

class ChromeExtensionsAPIClientTest : public testing::Test {
 public:
  ChromeExtensionsAPIClientTest() = default;

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  DISALLOW_COPY_AND_ASSIGN(ChromeExtensionsAPIClientTest);
};

TEST_F(ChromeExtensionsAPIClientTest, ShouldHideResponseHeader) {
  ChromeExtensionsAPIClient client;
  EXPECT_TRUE(client.ShouldHideResponseHeader(
      GaiaUrls::GetInstance()->gaia_url(), "X-Chrome-ID-Consistency-Response"));
  EXPECT_TRUE(client.ShouldHideResponseHeader(
      GaiaUrls::GetInstance()->gaia_url(), "x-cHroMe-iD-CoNsiStenCY-RESPoNSE"));
  EXPECT_FALSE(client.ShouldHideResponseHeader(
      GURL("http://www.example.com"), "X-Chrome-ID-Consistency-Response"));
  EXPECT_FALSE(client.ShouldHideResponseHeader(
      GaiaUrls::GetInstance()->gaia_url(), "Google-Accounts-SignOut"));
}

TEST_F(ChromeExtensionsAPIClientTest, ShouldHideBrowserNetworkRequest) {
  ChromeExtensionsAPIClient client;

  // Requests made by the browser with chrome://newtab as its initiator should
  // not be visible to extensions.
  WebRequestInfo request;
  request.url = GURL("https://example.com/script.js");
  request.initiator = url::Origin::Create(GURL(chrome::kChromeUINewTabURL));
  request.render_process_id = -1;
  request.type = content::ResourceType::RESOURCE_TYPE_SCRIPT;
  EXPECT_TRUE(client.ShouldHideBrowserNetworkRequest(request));

  // Main frame requests should always be visible to extensions.
  request.type = content::ResourceType::RESOURCE_TYPE_MAIN_FRAME;
  EXPECT_FALSE(client.ShouldHideBrowserNetworkRequest(request));

  // Similar requests made by the renderer should be visible to extensions.
  request.type = content::ResourceType::RESOURCE_TYPE_SCRIPT;
  request.render_process_id = 2;
  EXPECT_FALSE(client.ShouldHideBrowserNetworkRequest(request));
}

}  // namespace extensions
