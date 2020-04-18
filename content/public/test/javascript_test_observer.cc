// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/javascript_test_observer.h"

#include "base/run_loop.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/test_utils.h"

namespace content {

TestMessageHandler::TestMessageHandler() : ok_(true) {
}

TestMessageHandler::~TestMessageHandler() {
}

void TestMessageHandler::SetError(const std::string& message) {
  ok_ = false;
  error_message_ = message;
}

void TestMessageHandler::Reset() {
  ok_ = true;
  error_message_.clear();
}

JavascriptTestObserver::JavascriptTestObserver(
    WebContents* web_contents, TestMessageHandler* handler)
    : handler_(handler),
      running_(false),
      finished_(false) {
  Reset();
  registrar_.Add(this,
                 NOTIFICATION_DOM_OPERATION_RESPONSE,
                 Source<WebContents>(web_contents));
}

JavascriptTestObserver::~JavascriptTestObserver() {
}

bool JavascriptTestObserver::Run() {
  // Messages may have arrived before Run was called.
  if (!finished_) {
    CHECK(!running_);
    running_ = true;
    RunMessageLoop();
    running_ = false;
  }
  return handler_->ok();
}

void JavascriptTestObserver::Reset() {
  CHECK(!running_);
  running_ = false;
  finished_ = false;
  handler_->Reset();
}

void JavascriptTestObserver::Observe(
    int type,
    const NotificationSource& source,
    const NotificationDetails& details) {
  CHECK(type == NOTIFICATION_DOM_OPERATION_RESPONSE);
  Details<std::string> dom_op_result(details);
  // We might receive responses for other script execution, but we only
  // care about the test finished message.
  TestMessageHandler::MessageResponse response =
      handler_->HandleMessage(*dom_op_result.ptr());

  if (response == TestMessageHandler::DONE) {
    EndTest();
  } else {
    Continue();
  }
}

void JavascriptTestObserver::Continue() {
}

void JavascriptTestObserver::EndTest() {
  finished_ = true;
  if (running_) {
    running_ = false;
    base::RunLoop::QuitCurrentWhenIdleDeprecated();
  }
}

}  // namespace content
