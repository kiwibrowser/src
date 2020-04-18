// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/web_request/web_request_permissions.h"

#include "extensions/browser/api/extensions_api_client.h"
#include "extensions/browser/api/web_request/web_request_info.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace extensions {

TEST(ExtensionWebRequestPermissions, IsSensitiveRequest) {
  ExtensionsAPIClient api_client;
  struct TestCase {
    const char* url;
    bool is_sensitive_if_request_from_common_renderer;
    bool is_sensitive_if_request_from_browser_or_webui_renderer;
  } cases[] = {
      {"https://www.google.com", false, false},
      {"http://www.example.com", false, false},
      {"https://www.example.com", false, false},
      {"https://clients.google.com", false, true},
      {"https://clients4.google.com", false, true},
      {"https://clients9999.google.com", false, true},
      {"https://clients9999..google.com", false, false},
      {"https://clients9999.example.google.com", false, false},
      {"https://clients.google.com.", false, true},
      {"https://.clients.google.com.", false, true},
      {"http://google.example.com", false, false},
      {"http://www.example.com", false, false},
      {"https://www.example.com", false, false},
      {"https://clients.google.com", false, true},
      {"https://sb-ssl.google.com", true, true},
      {"https://sb-ssl.random.google.com", false, false},
      {"https://safebrowsing.googleapis.com", true, true},
      {"blob:https://safebrowsing.googleapis.com/"
       "fc3f440b-78ed-469f-8af8-7a1717ff39ae",
       true, true},
      {"filesystem:https://safebrowsing.googleapis.com/path", true, true},
      {"https://safebrowsing.googleapis.com.", true, true},
      {"https://safebrowsing.googleapis.com/v4", true, true},
      {"https://safebrowsing.googleapis.com:80/v4", true, true},
      {"https://safebrowsing.googleapis.com./v4", true, true},
      {"https://safebrowsing.googleapis.com/v5", true, true},
      {"https://safebrowsing.google.com/safebrowsing", true, true},
      {"https://safebrowsing.google.com/safebrowsing/anything", true, true},
      {"https://safebrowsing.google.com", false, false},
      {"https://chrome.google.com", false, false},
      {"https://chrome.google.com/webstore", true, true},
      {"https://chrome.google.com./webstore", true, true},
      {"blob:https://chrome.google.com/fc3f440b-78ed-469f-8af8-7a1717ff39ae",
       false, false},
      {"https://chrome.google.com:80/webstore", true, true},
      {"https://chrome.google.com/webstore?query", true, true},
  };
  for (const TestCase& test : cases) {
    WebRequestInfo request;
    request.url = GURL(test.url);
    EXPECT_TRUE(request.url.is_valid()) << test.url;

    request.initiator = url::Origin::Create(request.url);
    EXPECT_EQ(test.is_sensitive_if_request_from_common_renderer,
              IsSensitiveRequest(request, false /* is_request_from_browser */,
                                 false /* is_request_from_web_ui_renderer */))
        << test.url;

    const bool supported_in_webui_renderers =
        !request.url.SchemeIsHTTPOrHTTPS();
    request.initiator = base::nullopt;
    EXPECT_EQ(test.is_sensitive_if_request_from_browser_or_webui_renderer,
              IsSensitiveRequest(request, true /* is_request_from_browser */,
                                 supported_in_webui_renderers))
        << test.url;
  }
}

}  // namespace extensions
