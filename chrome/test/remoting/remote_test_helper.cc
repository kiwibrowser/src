// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/remoting/remote_test_helper.h"

#include "base/bind.h"
#include "chrome/test/remoting/waiter.h"

namespace remoting {

Event::Event() : action(Action::None), value(0), modifiers(0) {}

RemoteTestHelper::RemoteTestHelper(content::WebContents* web_content)
    : web_content_(web_content) {}

// static
bool RemoteTestHelper::ExecuteScriptAndExtractBool(
    content::WebContents* web_contents, const std::string& script) {
  bool result;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      web_contents,
      "window.domAutomationController.send(" + script + ");",
      &result));

  return result;
}

// static
int RemoteTestHelper::ExecuteScriptAndExtractInt(
    content::WebContents* web_contents, const std::string& script) {
  int result;
  _ASSERT_TRUE(content::ExecuteScriptAndExtractInt(
      web_contents,
      "window.domAutomationController.send(" + script + ");",
      &result));

  return result;
}

// static
std::string RemoteTestHelper::ExecuteScriptAndExtractString(
    content::WebContents* web_contents, const std::string& script) {
  std::string result;
  _ASSERT_TRUE(content::ExecuteScriptAndExtractString(
      web_contents,
      "window.domAutomationController.send(" + script + ");",
      &result));

  return result;
}

void RemoteTestHelper::ExecuteRpc(const std::string& method,
                                  base::TimeDelta timeout,
                                  base::TimeDelta interval) {
  ASSERT_TRUE(content::ExecuteScript(web_content_, method));

  // Wait until we receive a response object from the server.
  // When this happens the jsonRpc.reponseObject becomes non-null.
  ConditionalTimeoutWaiter waiter(
      timeout,
      interval,
      base::Bind(
          &RemoteTestHelper::ExecuteScriptAndExtractBool,
          web_content_,
          "jsonRpc.responseObject != null"));
  EXPECT_TRUE(waiter.Wait());
}

void RemoteTestHelper::ClearLastEvent() {
  ExecuteRpc("jsonRpc.clearLastEvent();");
}

bool RemoteTestHelper::IsValidEvent() {
  // Call GetLastEvent on the server
  ExecuteRpc("jsonRpc.getLastEvent()",
             base::TimeDelta::FromMilliseconds(250),
             base::TimeDelta::FromMilliseconds(50));
  return ExecuteScriptAndExtractBool(web_content_,
                                     "jsonRpc.responseObject.action != 0");
}

void RemoteTestHelper::GetLastEvent(Event* event) {
  // Wait for a valid event
  ConditionalTimeoutWaiter waiter(
      base::TimeDelta::FromSeconds(2),
      base::TimeDelta::FromMilliseconds(500),
      base::Bind(&RemoteTestHelper::IsValidEvent,
                 base::Unretained(this)));
  EXPECT_TRUE(waiter.Wait());

  // Extract the event's values
  event->action = static_cast<Action>(
      ExecuteScriptAndExtractInt(
          web_content_,
          "jsonRpc.responseObject.action"));
  event->value = ExecuteScriptAndExtractInt(
      web_content_,
      "jsonRpc.responseObject.value");
  event->modifiers = ExecuteScriptAndExtractInt(
      web_content_,
      "jsonRpc.responseObject.modifiers");
}

}  // namespace remoting
