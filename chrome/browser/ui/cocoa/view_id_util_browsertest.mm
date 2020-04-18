// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/devtools/devtools_window_testing.h"
#include "chrome/browser/download/download_shelf.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/cocoa/view_id_util.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_context.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/views/scoped_macviews_browser_mode.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "components/prefs/pref_service.h"
#include "extensions/common/switches.h"

using bookmarks::BookmarkModel;
using content::OpenURLParams;
using content::Referrer;

// Basic sanity check of ViewID use on the Mac.
class ViewIDTest : public InProcessBrowserTest {
 public:
  ViewIDTest() : root_window_(nil) {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        extensions::switches::kEnableExperimentalExtensionApis);
  }

  void CheckViewID(ViewID view_id, bool should_have) {
    if (!root_window_)
      root_window_ = browser()->window()->GetNativeWindow();

    ASSERT_TRUE(root_window_);
    NSView* view = view_id_util::GetView(root_window_, view_id);
    EXPECT_EQ(should_have, !!view) << " Failed id=" << view_id;
  }

  void DoTest() {
    // Make sure FindBar is created to test VIEW_ID_FIND_IN_PAGE_TEXT_FIELD.
    chrome::ShowFindBar(browser());

    // Make sure docked devtools is created to test VIEW_ID_DEV_TOOLS_DOCKED
    DevToolsWindow* devtools_window =
        DevToolsWindowTesting::OpenDevToolsWindowSync(browser(), true);

    // Make sure download shelf is created to test VIEW_ID_DOWNLOAD_SHELF
    browser()->window()->GetDownloadShelf()->Open();

    // Create a bookmark to test VIEW_ID_BOOKMARK_BAR_ELEMENT
    BookmarkModel* bookmark_model =
        BookmarkModelFactory::GetForBrowserContext(browser()->profile());
    if (bookmark_model) {
      if (!bookmark_model->loaded())
        bookmarks::test::WaitForBookmarkModelToLoad(bookmark_model);

      bookmarks::AddIfNotBookmarked(bookmark_model,
                                    GURL(url::kAboutBlankURL),
                                    base::ASCIIToUTF16("about"));
    }

    for (int i = VIEW_ID_TOOLBAR; i < VIEW_ID_PREDEFINED_COUNT; ++i) {
      // Mac implementation does not support following ids yet.
      // TODO(palmer): crbug.com/536257: Enable VIEW_ID_LOCATION_ICON.
      if (i == VIEW_ID_STAR_BUTTON ||
          i == VIEW_ID_CONTENTS_SPLIT ||
          i == VIEW_ID_BROWSER_ACTION ||
          i == VIEW_ID_FEEDBACK_BUTTON ||
          i == VIEW_ID_SCRIPT_BUBBLE ||
          i == VIEW_ID_SAVE_CREDIT_CARD_BUTTON ||
          i == VIEW_ID_TRANSLATE_BUTTON ||
          i == VIEW_ID_LOCATION_ICON ||
          i == VIEW_ID_FIND_IN_PAGE_PREVIOUS_BUTTON ||
          i == VIEW_ID_FIND_IN_PAGE_NEXT_BUTTON ||
          i == VIEW_ID_FIND_IN_PAGE_CLOSE_BUTTON) {
        continue;
      }

      CheckViewID(static_cast<ViewID>(i), true);
    }

    CheckViewID(VIEW_ID_TAB, true);
    CheckViewID(VIEW_ID_TAB_STRIP, true);
    CheckViewID(VIEW_ID_PREDEFINED_COUNT, false);

    DevToolsWindowTesting::CloseDevToolsWindowSync(devtools_window);
  }

 private:
  NSWindow* root_window_;

  test::ScopedMacViewsBrowserMode cocoa_browser_mode_{false};
};

IN_PROC_BROWSER_TEST_F(ViewIDTest, Basic) {
  ASSERT_NO_FATAL_FAILURE(DoTest());
}

// Flaky on Mac: http://crbug.com/90557.
IN_PROC_BROWSER_TEST_F(ViewIDTest, DISABLED_Fullscreen) {
  browser()->exclusive_access_manager()->context()->EnterFullscreen(
      GURL(), EXCLUSIVE_ACCESS_BUBBLE_TYPE_BROWSER_FULLSCREEN_EXIT_INSTRUCTION);
  ASSERT_NO_FATAL_FAILURE(DoTest());
}

IN_PROC_BROWSER_TEST_F(ViewIDTest, Tab) {
  CheckViewID(VIEW_ID_TAB_0, true);
  CheckViewID(VIEW_ID_TAB_LAST, true);

  // Open 9 new tabs.
  for (int i = 1; i <= 9; ++i) {
    CheckViewID(static_cast<ViewID>(VIEW_ID_TAB_0 + i), false);
    browser()->OpenURL(OpenURLParams(GURL(url::kAboutBlankURL), Referrer(),
                                     WindowOpenDisposition::NEW_BACKGROUND_TAB,
                                     ui::PAGE_TRANSITION_TYPED, false));
    CheckViewID(static_cast<ViewID>(VIEW_ID_TAB_0 + i), true);
    // VIEW_ID_TAB_LAST should always be available.
    CheckViewID(VIEW_ID_TAB_LAST, true);
  }

  // Open the 11th tab.
  browser()->OpenURL(OpenURLParams(GURL(url::kAboutBlankURL), Referrer(),
                                   WindowOpenDisposition::NEW_BACKGROUND_TAB,
                                   ui::PAGE_TRANSITION_TYPED, false));
  CheckViewID(VIEW_ID_TAB_LAST, true);
}
