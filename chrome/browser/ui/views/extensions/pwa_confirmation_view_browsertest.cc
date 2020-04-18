// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/browser/ui/views/extensions/pwa_confirmation_view.h"
#include "chrome/common/web_application_info.h"
#include "components/constrained_window/constrained_window_views.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"

namespace {

// Helper class to display the PWAConfirmationView dialog for testing.
class PWAConfirmationViewTest : public DialogBrowserTest {
 public:
  PWAConfirmationViewTest() {}

  void ShowUi(const std::string& name) override {
    constexpr int kIconSize = 48;
    WebApplicationInfo::IconInfo icon_info;
    icon_info.data.allocN32Pixels(kIconSize, kIconSize, true);
    icon_info.data.eraseColor(SK_ColorBLUE);
    icon_info.width = kIconSize;
    icon_info.height = kIconSize;

    WebApplicationInfo web_app_info;
    web_app_info.icons.push_back(icon_info);
    web_app_info.open_as_window = true;
    if (name == "short_text") {
      web_app_info.title = base::ASCIIToUTF16("Title");
      web_app_info.app_url = GURL("https://www.example.com:9090/path");
    } else if (name == "long_text") {
      web_app_info.title =
          base::ASCIIToUTF16("abcd\n1234567890123456789012345678901234567890");

      web_app_info.app_url = GURL(
          "https://www"
          ".1234567890123456789012345678901234567890"
          ".com:443/path");
    } else if (name == "rtl") {
      web_app_info.title = base::UTF8ToUTF16("דוגמא");
      web_app_info.app_url = GURL("https://דוגמא.דוגמא.דוגמא.אחד.example.com");
    }
    constrained_window::ShowWebModalDialogViews(
        new PWAConfirmationView(web_app_info,
                                chrome::AppInstallationAcceptanceCallback()),
        browser()->tab_strip_model()->GetActiveWebContents());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(PWAConfirmationViewTest);
};

// Launches an installation confirmation dialog for a PWA with a short name and
// origin.
IN_PROC_BROWSER_TEST_F(PWAConfirmationViewTest, InvokeUi_short_text) {
  ShowAndVerifyUi();
}

// Launches an installation confirmation dialog for a PWA with name and origin
// long enough to be elided.
IN_PROC_BROWSER_TEST_F(PWAConfirmationViewTest, InvokeUi_long_text) {
  ShowAndVerifyUi();
}

// Launches an installation confirmation dialog for a PWA with an RTL subdomain
// which is long enough to be elided.
IN_PROC_BROWSER_TEST_F(PWAConfirmationViewTest, InvokeUi_rtl) {
  ShowAndVerifyUi();
}

}  // namespace
