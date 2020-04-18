// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/desktop_capture/desktop_media_picker_views.h"

#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/media/webrtc/desktop_media_list.h"
#include "chrome/browser/media/webrtc/fake_desktop_media_list.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "content/public/browser/desktop_media_id.h"

class DesktopMediaPickerViewsBrowserTest : public DialogBrowserTest {
 public:
  DesktopMediaPickerViewsBrowserTest() {}

  // DialogBrowserTest:
  void ShowUi(const std::string& name) override {
    picker_ = std::make_unique<DesktopMediaPickerViews>();
    auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();
    gfx::NativeWindow native_window = browser()->window()->GetNativeWindow();

    std::vector<std::unique_ptr<DesktopMediaList>> source_lists;
    for (auto type : {content::DesktopMediaID::TYPE_SCREEN,
                      content::DesktopMediaID::TYPE_WINDOW,
                      content::DesktopMediaID::TYPE_WEB_CONTENTS}) {
      source_lists.push_back(std::make_unique<FakeDesktopMediaList>(type));
    }

    DesktopMediaPicker::Params picker_params;
    picker_params.web_contents = web_contents;
    picker_params.context = native_window;
    picker_params.app_name = base::ASCIIToUTF16("app_name");
    picker_params.target_name = base::ASCIIToUTF16("target_name");
    picker_params.request_audio = true;
    picker_->Show(picker_params, std::move(source_lists),
                  DesktopMediaPicker::DoneCallback());
  }

 private:
  std::unique_ptr<DesktopMediaPickerViews> picker_;

  DISALLOW_COPY_AND_ASSIGN(DesktopMediaPickerViewsBrowserTest);
};

// Invokes a dialog that allows the user to select what view of their desktop
// they would like to share.
IN_PROC_BROWSER_TEST_F(DesktopMediaPickerViewsBrowserTest, InvokeUi_default) {
  ShowAndVerifyUi();
}
