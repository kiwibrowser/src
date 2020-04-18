// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_MD_BOOKMARKS_MD_BOOKMARKS_BROWSERTEST_H_
#define CHROME_BROWSER_UI_WEBUI_MD_BOOKMARKS_MD_BOOKMARKS_BROWSERTEST_H_

#include "chrome/test/base/web_ui_browser_test.h"
#include "content/public/browser/web_ui_message_handler.h"

class MdBookmarksBrowserTest : public WebUIBrowserTest,
                               public content::WebUIMessageHandler {
 public:
  MdBookmarksBrowserTest();
  ~MdBookmarksBrowserTest() override;

  void SetIncognitoAvailability(int availability);
  void SetCanEditBookmarks(bool canEdit);

 private:
  void HandleSetIncognitoAvailability(const base::ListValue* args);
  void HandleSetCanEditBookmarks(const base::ListValue* args);

  // content::WebUIMessageHandler:
  void RegisterMessages() override;

  // WebUIBrowserTest:
  content::WebUIMessageHandler* GetMockMessageHandler() override;

  DISALLOW_COPY_AND_ASSIGN(MdBookmarksBrowserTest);
};

#endif  // CHROME_BROWSER_UI_WEBUI_MD_BOOKMARKS_MD_BOOKMARKS_BROWSERTEST_H_
