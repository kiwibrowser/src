// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/translate/translate_bubble_view.h"

#include <memory>
#include <string>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/translate/chrome_translate_client.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/translate/core/browser/translate_manager.h"
#include "components/translate/core/common/language_detection_details.h"
#include "content/public/browser/notification_details.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_base_features.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/views/controls/button/menu_button.h"

class TranslateBubbleViewBrowserTest : public InProcessBrowserTest {
 public:
  TranslateBubbleViewBrowserTest() {}
  ~TranslateBubbleViewBrowserTest() override {}

  void SetUp() override {
#if defined(OS_MACOSX)
    // Enable toolkit-views bubbles on Mac (otherwise Cocoa bubbles are used).
    feature_list_.InitAndEnableFeature(features::kSecondaryUiMd);
#endif
    set_open_about_blank_on_browser_launch(true);
    translate::TranslateManager::SetIgnoreMissingKeyForTesting(true);
    InProcessBrowserTest::SetUp();
  }

 protected:
  void NavigateAndWaitForLanguageDetection(const GURL& url,
                                           const std::string& expected_lang) {
    expected_lang_ = expected_lang;
    content::WindowedNotificationObserver language_detected_signal(
        chrome::NOTIFICATION_TAB_LANGUAGE_DETERMINED,
        base::Bind(&TranslateBubbleViewBrowserTest::OnLanguageDetermined,
                   base::Unretained(this)));

    ui_test_utils::NavigateToURL(browser(), url);
    language_detected_signal.Wait();
  }

 private:
  std::string expected_lang_;
  base::test::ScopedFeatureList feature_list_;

  bool OnLanguageDetermined(const content::NotificationSource& source,
                            const content::NotificationDetails& details) {
    const std::string& language =
        content::Details<translate::LanguageDetectionDetails>(details)
            ->cld_language;
    return language == expected_lang_;
  }

  DISALLOW_COPY_AND_ASSIGN(TranslateBubbleViewBrowserTest);
};

IN_PROC_BROWSER_TEST_F(TranslateBubbleViewBrowserTest,
                       CloseBrowserWithoutTranslating) {
  EXPECT_FALSE(TranslateBubbleView::GetCurrentBubble());

  // Show a French page and wait until the bubble is shown.
  GURL french_url = ui_test_utils::GetTestUrl(
      base::FilePath(), base::FilePath(FILE_PATH_LITERAL("french_page.html")));
  NavigateAndWaitForLanguageDetection(french_url, "fr");
  EXPECT_TRUE(TranslateBubbleView::GetCurrentBubble());

  // Close the window without translating. Spin the runloop to allow
  // asynchronous window closure to happen.
  chrome::CloseWindow(browser());
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(TranslateBubbleView::GetCurrentBubble());
}

IN_PROC_BROWSER_TEST_F(TranslateBubbleViewBrowserTest,
                       CloseLastTabWithoutTranslating) {
  EXPECT_FALSE(TranslateBubbleView::GetCurrentBubble());

  // Show a French page and wait until the bubble is shown.
  content::WebContents* current_web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  GURL french_url = ui_test_utils::GetTestUrl(
      base::FilePath(), base::FilePath(FILE_PATH_LITERAL("french_page.html")));
  NavigateAndWaitForLanguageDetection(french_url, "fr");
  EXPECT_TRUE(TranslateBubbleView::GetCurrentBubble());

  // Close the tab without translating. Spin the runloop to allow asynchronous
  // window closure to happen.
  EXPECT_EQ(1, browser()->tab_strip_model()->count());
  chrome::CloseWebContents(browser(), current_web_contents, false);
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(TranslateBubbleView::GetCurrentBubble());
}

IN_PROC_BROWSER_TEST_F(TranslateBubbleViewBrowserTest,
                       CloseAnotherTabWithoutTranslating) {
  EXPECT_FALSE(TranslateBubbleView::GetCurrentBubble());

  int active_index = browser()->tab_strip_model()->active_index();

  // Open another tab to load a French page on background.
  int french_index = active_index + 1;
  GURL french_url = ui_test_utils::GetTestUrl(
      base::FilePath(), base::FilePath(FILE_PATH_LITERAL("french_page.html")));
  chrome::AddTabAt(browser(), french_url, french_index, false);
  EXPECT_EQ(active_index, browser()->tab_strip_model()->active_index());
  EXPECT_EQ(2, browser()->tab_strip_model()->count());

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetWebContentsAt(french_index);

  // The bubble is not shown because the tab is not activated.
  EXPECT_FALSE(TranslateBubbleView::GetCurrentBubble());

  // Close the French page tab immediately.
  chrome::CloseWebContents(browser(), web_contents, false);
  EXPECT_EQ(active_index, browser()->tab_strip_model()->active_index());
  EXPECT_EQ(1, browser()->tab_strip_model()->count());
  EXPECT_FALSE(TranslateBubbleView::GetCurrentBubble());

  // Close the last tab.
  chrome::CloseWebContents(browser(),
                           browser()->tab_strip_model()->GetActiveWebContents(),
                           false);
}
