// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_MEDIA_ROUTER_MEDIA_ROUTER_WEB_UI_TEST_H_
#define CHROME_BROWSER_UI_WEBUI_MEDIA_ROUTER_MEDIA_ROUTER_WEB_UI_TEST_H_

#include "chrome/test/base/browser_with_test_window_test.h"

class MediaRouterWebUITest : public BrowserWithTestWindowTest {
 public:
  // |require_mock_ui_service_| defaults to false in the default ctor.
  MediaRouterWebUITest();
  explicit MediaRouterWebUITest(bool require_mock_ui_service);
  ~MediaRouterWebUITest() override;

 protected:
  // BrowserWithTestWindowTest:
  TestingProfile::TestingFactories GetTestingFactories() override;
  BrowserWindow* CreateBrowserWindow() override;

 private:
  // When this is set to true, MockMediaRouterUIService is instantiated.
  // Otherwise, no MediaRouterUIService is instantiated.
  bool require_mock_ui_service_;

  DISALLOW_COPY_AND_ASSIGN(MediaRouterWebUITest);
};

#endif  // CHROME_BROWSER_UI_WEBUI_MEDIA_ROUTER_MEDIA_ROUTER_WEB_UI_TEST_H_
