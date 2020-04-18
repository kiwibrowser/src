// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/test/fakes/test_java_script_dialog_presenter.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

TestJavaScriptDialog::TestJavaScriptDialog() = default;

TestJavaScriptDialog::TestJavaScriptDialog(const TestJavaScriptDialog&) =
    default;

TestJavaScriptDialog::~TestJavaScriptDialog() = default;

TestJavaScriptDialogPresenter::TestJavaScriptDialogPresenter() = default;

TestJavaScriptDialogPresenter::~TestJavaScriptDialogPresenter() = default;

void TestJavaScriptDialogPresenter::RunJavaScriptDialog(
    WebState* web_state,
    const GURL& origin_url,
    JavaScriptDialogType java_script_dialog_type,
    NSString* message_text,
    NSString* default_prompt_text,
    const DialogClosedCallback& callback) {
  TestJavaScriptDialog dialog;
  dialog.web_state = web_state;
  dialog.origin_url = origin_url;
  dialog.java_script_dialog_type = java_script_dialog_type;
  dialog.message_text = [message_text copy];
  dialog.default_prompt_text = [default_prompt_text copy];

  requested_dialogs_.push_back(dialog);

  callback.Run(callback_success_argument_, callback_user_input_argument_);
}

void TestJavaScriptDialogPresenter::CancelDialogs(WebState* web_state) {
  cancel_dialogs_called_ = true;
}

}  // namespace web
