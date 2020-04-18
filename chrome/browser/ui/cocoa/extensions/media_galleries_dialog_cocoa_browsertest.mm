// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/cocoa/extensions/media_galleries_dialog_cocoa.h"

#include "chrome/browser/media_galleries/media_galleries_dialog_controller_mock.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/cocoa/constrained_window/constrained_window_alert.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/views/scoped_macviews_browser_mode.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

class MediaGalleriesDialogBrowserTest : public InProcessBrowserTest {
 private:
  test::ScopedMacViewsBrowserMode cocoa_browser_mode_{false};
};

// Verify that programatically closing the constrained window correctly closes
// the sheet.
IN_PROC_BROWSER_TEST_F(MediaGalleriesDialogBrowserTest, Close) {
  NiceMock<MediaGalleriesDialogControllerMock> controller;

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_CALL(controller, WebContents()).
      WillRepeatedly(Return(web_contents));
  std::vector<base::string16> headers;
  headers.push_back(base::string16());  // The first section has no header.
  headers.push_back(base::ASCIIToUTF16("header2"));
  ON_CALL(controller, GetSectionHeaders()).
      WillByDefault(Return(headers));
  EXPECT_CALL(controller, GetAuxiliaryButtonText()).
      WillRepeatedly(Return(base::ASCIIToUTF16("button")));
  EXPECT_CALL(controller, GetSectionEntries(_)).
      Times(AnyNumber());

  std::unique_ptr<MediaGalleriesDialogCocoa> dialog(
      static_cast<MediaGalleriesDialogCocoa*>(
          MediaGalleriesDialog::Create(&controller)));
  base::scoped_nsobject<NSWindow> window([[dialog->alert_ window] retain]);
  EXPECT_TRUE([window isVisible]);

  web_modal::WebContentsModalDialogManager* manager =
      web_modal::WebContentsModalDialogManager::FromWebContents(web_contents);
  web_modal::WebContentsModalDialogManager::TestApi(manager).CloseAllDialogs();
  EXPECT_FALSE([window isVisible]);
}
