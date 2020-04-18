// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/autofill/save_card_bubble_controller_impl.h"

#include <memory>

#include "base/json/json_reader.h"
#include "base/macros.h"
#include "base/values.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace autofill {

class SaveCardBubbleControllerImplTest : public DialogBrowserTest {
 public:
  SaveCardBubbleControllerImplTest() {}

  std::unique_ptr<base::DictionaryValue> GetTestLegalMessage() {
    std::unique_ptr<base::Value> value(base::JSONReader::Read(
        "{"
        "  \"line\" : [ {"
        "     \"template\": \"This is the entire message.\""
        "  } ]"
        "}"));
    base::DictionaryValue* dictionary;
    value->GetAsDictionary(&dictionary);
    return dictionary->CreateDeepCopy();
  }

  // DialogBrowserTest:
  void ShowUi(const std::string& name) override {
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();

    // Do lazy initialization of SaveCardBubbleControllerImpl. Alternative:
    // invoke via ChromeAutofillClient.
    SaveCardBubbleControllerImpl::CreateForWebContents(web_contents);
    controller_ = SaveCardBubbleControllerImpl::FromWebContents(web_contents);
    DCHECK(controller_);

    // Behavior depends on the test case name (not the most robust, but it will
    // do).
    if (name == "Local") {
      controller_->ShowBubbleForLocalSave(test::GetCreditCard(),
                                          base::DoNothing());
    } else {
      controller_->ShowBubbleForUpload(test::GetMaskedServerCard(),
                                       GetTestLegalMessage(),
                                       base::DoNothing());
    }
  }

  SaveCardBubbleControllerImpl* controller() { return controller_; }

 private:
  SaveCardBubbleControllerImpl* controller_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(SaveCardBubbleControllerImplTest);
};

// Invokes a bubble asking the user if they want to save a credit card locally.
IN_PROC_BROWSER_TEST_F(SaveCardBubbleControllerImplTest, InvokeUi_Local) {
  ShowAndVerifyUi();
}

// Invokes a bubble asking the user if they want to save a credit card to the
// server.
IN_PROC_BROWSER_TEST_F(SaveCardBubbleControllerImplTest, InvokeUi_Server) {
  ShowAndVerifyUi();
}

// Tests that opening a new tab will hide the save card bubble.
IN_PROC_BROWSER_TEST_F(SaveCardBubbleControllerImplTest, NewTabHidesDialog) {
  ShowUi("Local");
  EXPECT_NE(nullptr, controller()->save_card_bubble_view());
  // Open a new tab page in the foreground.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(chrome::kChromeUINewTabURL),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_TAB |
          ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  EXPECT_EQ(nullptr, controller()->save_card_bubble_view());
}

}  // namespace autofill
