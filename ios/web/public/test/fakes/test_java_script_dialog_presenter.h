// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_TEST_FAKES_TEST_JAVA_SCRIPT_DIALOG_PRESENTER_H_
#define IOS_WEB_PUBLIC_TEST_FAKES_TEST_JAVA_SCRIPT_DIALOG_PRESENTER_H_

#include <vector>

#import "ios/web/public/java_script_dialog_presenter.h"

namespace web {

struct TestJavaScriptDialog {
  TestJavaScriptDialog();
  TestJavaScriptDialog(const TestJavaScriptDialog&);
  ~TestJavaScriptDialog();
  WebState* web_state = nullptr;
  GURL origin_url;
  JavaScriptDialogType java_script_dialog_type;
  NSString* message_text;
  NSString* default_prompt_text;
};

// Test presenter to check that the JavaScriptDialogPresenter methods are called
// as expected. |RunJavaScriptDialog| always calls |callback| with
// |callback_success_argument| and |callback_user_input_argument| values.
class TestJavaScriptDialogPresenter : public JavaScriptDialogPresenter {
 public:
  TestJavaScriptDialogPresenter();
  ~TestJavaScriptDialogPresenter();

  // JavaScriptDialogPresenter overrides:
  void RunJavaScriptDialog(WebState* web_state,
                           const GURL& origin_url,
                           JavaScriptDialogType java_script_dialog_type,
                           NSString* message_text,
                           NSString* default_prompt_text,
                           const DialogClosedCallback& callback) override;
  void CancelDialogs(WebState* web_state) override;

  // True if the JavaScriptDialogPresenter CancelDialogs method has been called.
  bool cancel_dialogs_called() const { return cancel_dialogs_called_; }

  // Returns a vector of requested dialogs.
  const std::vector<TestJavaScriptDialog>& requested_dialogs() const {
    return requested_dialogs_;
  }

  // Sets |success| argument to be used for RunJavaScriptDialog callback.
  void set_callback_success_argument(bool success) {
    callback_success_argument_ = success;
  }

  // Sets |user_input| argument to be used for RunJavaScriptDialog callback.
  void set_callback_user_input_argument(NSString* user_input) {
    callback_user_input_argument_ = user_input;
  }

 private:
  bool cancel_dialogs_called_ = false;
  std::vector<TestJavaScriptDialog> requested_dialogs_;
  bool callback_success_argument_ = false;
  NSString* callback_user_input_argument_;
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_TEST_FAKES_TEST_JAVA_SCRIPT_DIALOG_PRESENTER_H_
