// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/browser_state.h"
#import "ios/web/public/test/fakes/test_web_client.h"
#include "ios/web/public/test/fakes/test_web_state_observer.h"
#import "ios/web/public/test/web_js_test.h"
#import "ios/web/public/test/web_test_with_web_state.h"
#import "ios/web/web_state/js/page_script_util.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

class FormTestClient : public web::TestWebClient {
 public:
  NSString* GetDocumentStartScriptForAllFrames(
      web::BrowserState* browser_state) const override {
    return web::GetPageScript(@"form");
  }
};

// Text fixture to test password controller.
class FormJsTest : public web::WebJsTest<web::WebTestWithWebState> {
 public:
  FormJsTest()
      : web::WebJsTest<web::WebTestWithWebState>(
            std::make_unique<FormTestClient>()) {}
};

// Tests that keyup event correctly delivered to WebStateObserver if the element
// is focused.
TEST_F(FormJsTest, KeyUpEventFocused) {
  web::TestWebStateObserver observer(web_state());
  LoadHtml(@"<p><input id='test'/></p>");
  ASSERT_FALSE(observer.form_activity_info());
  ExecuteJavaScript(
      @"var e = document.getElementById('test');"
       "e.focus();"
       "var ev = new KeyboardEvent('keyup', {bubbles:true});"
       "e.dispatchEvent(ev);");
  web::TestWebStateObserver* block_observer = &observer;
  WaitForCondition(^bool {
    return block_observer->form_activity_info() != nullptr;
  });
  web::TestFormActivityInfo* info = observer.form_activity_info();
  ASSERT_TRUE(info);
  EXPECT_EQ("keyup", info->form_activity.type);
  EXPECT_FALSE(info->form_activity.input_missing);
}

// Tests that keyup event is not delivered to WebStateObserver if the element is
// not focused.
TEST_F(FormJsTest, KeyUpEventNotFocused) {
  web::TestWebStateObserver observer(web_state());
  LoadHtml(@"<p><input id='test'/></p>");
  ASSERT_FALSE(observer.form_activity_info());
  ExecuteJavaScript(
      @"var e = document.getElementById('test');"
       "var ev = new KeyboardEvent('keyup', {bubbles:true});"
       "e.dispatchEvent(ev);");
  WaitForBackgroundTasks();
  web::TestFormActivityInfo* info = observer.form_activity_info();
  ASSERT_FALSE(info);
}

// Tests that focus event correctly delivered to WebStateObserver.
TEST_F(FormJsTest, FocusMainFrame) {
  web::TestWebStateObserver observer(web_state());
  LoadHtml(
      @"<form>"
       "<input type='text' name='username' id='id1'>"
       "<input type='password' name='password' id='id2'>"
       "</form>");
  ASSERT_FALSE(observer.form_activity_info());
  ExecuteJavaScript(@"document.getElementById('id1').focus();");
  web::TestWebStateObserver* block_observer = &observer;
  WaitForCondition(^bool {
    return block_observer->form_activity_info() != nullptr;
  });
  web::TestFormActivityInfo* info = observer.form_activity_info();
  ASSERT_TRUE(info);
  EXPECT_EQ("focus", info->form_activity.type);
  EXPECT_FALSE(info->form_activity.input_missing);
}

// Tests that submit event correctly delivered to WebStateObserver.
TEST_F(FormJsTest, FormSubmitMainFrame) {
  web::TestWebStateObserver observer(web_state());
  LoadHtml(
      @"<form id='form1'>"
       "<input type='password'>"
       "<input type='submit' id='submit_input'/>"
       "</form>");
  ASSERT_FALSE(observer.submit_document_info());
  ExecuteJavaScript(@"document.getElementById('submit_input').click();");
  web::TestSubmitDocumentInfo* info = observer.submit_document_info();
  ASSERT_TRUE(info);
  EXPECT_EQ("form1", info->form_name);
}

// Tests that focus event from same-origin iframe correctly delivered to
// WebStateObserver.
TEST_F(FormJsTest, FocusSameOriginIFrame) {
  web::TestWebStateObserver observer(web_state());
  LoadHtml(@"<iframe id='frame1'></iframe>");
  ExecuteJavaScript(
      @"document.getElementById('frame1').contentDocument.body.innerHTML = "
       "'<form>"
       "<input type=\"text\" name=\"username\" id=\"id1\">"
       "<input type=\"password\" name=\"password\" id=\"id2\">"
       "</form>'");

  ExecuteJavaScript(
      @"document.getElementById('frame1').contentDocument.getElementById('id1')"
      @".focus()");
  web::TestWebStateObserver* block_observer = &observer;
  WaitForCondition(^bool {
    return block_observer->form_activity_info() != nullptr;
  });
  web::TestFormActivityInfo* info = observer.form_activity_info();
  ASSERT_TRUE(info);
  EXPECT_EQ("focus", info->form_activity.type);
  EXPECT_FALSE(info->form_activity.input_missing);
}

// Tests that submit event from same-origin iframe correctly delivered to
// WebStateObserver.
TEST_F(FormJsTest, FormSameOriginIFrame) {
  web::TestWebStateObserver observer(web_state());
  LoadHtml(@"<iframe id='frame1'></iframe>");
  ExecuteJavaScript(
      @"document.getElementById('frame1').contentDocument.body.innerHTML = "
       "'<form id=\"form1\">"
       "<input type=\"password\" name=\"password\" id=\"id2\">"
       "<input type=\"submit\" id=\"submit_input\"/>"
       "</form>'");
  ExecuteJavaScript(
      @"document.getElementById('frame1').contentDocument.getElementById('"
      @"submit_input').click();");
  web::TestSubmitDocumentInfo* info = observer.submit_document_info();
  ASSERT_TRUE(info);
  EXPECT_EQ("form1", info->form_name);
}

// Tests that a new form triggers form_changed event.
TEST_F(FormJsTest, AddForm) {
  web::TestWebStateObserver observer(web_state());
  LoadHtml(@"<body></body>");

  ExecuteJavaScript(
      @"__gCrWeb.form.trackFormMutations(10);"
      @"var form = document.createElement('form');"
      @"document.body.appendChild(form);");
  web::TestWebStateObserver* block_observer = &observer;
  __block web::TestFormActivityInfo* info = nil;
  WaitForCondition(^{
    info = block_observer->form_activity_info();
    return info != nil;
  });
  EXPECT_EQ("form_changed", info->form_activity.type);
  EXPECT_FALSE(info->form_activity.input_missing);
}

// Tests that a new input element triggers form_changed event.
TEST_F(FormJsTest, AddInput) {
  web::TestWebStateObserver observer(web_state());
  LoadHtml(@"<form id='formId'/>");

  ExecuteJavaScript(
      @"__gCrWeb.form.trackFormMutations(10);"
      @"var input = document.createElement('input');"
      @"document.getElementById('formId').appendChild(input);");
  web::TestWebStateObserver* block_observer = &observer;
  __block web::TestFormActivityInfo* info = nil;
  WaitForCondition(^{
    info = block_observer->form_activity_info();
    return info != nil;
  });
  EXPECT_EQ("form_changed", info->form_activity.type);
  EXPECT_FALSE(info->form_activity.input_missing);
}

// Tests that a new select element triggers form_changed event.
TEST_F(FormJsTest, AddSelect) {
  web::TestWebStateObserver observer(web_state());
  LoadHtml(@"<form id='formId'/>");

  ExecuteJavaScript(
      @"__gCrWeb.form.trackFormMutations(10);"
      @"var select = document.createElement('select');"
      @"document.getElementById('formId').appendChild(select);");
  web::TestWebStateObserver* block_observer = &observer;
  __block web::TestFormActivityInfo* info = nil;
  WaitForCondition(^{
    info = block_observer->form_activity_info();
    return info != nil;
  });
  EXPECT_EQ("form_changed", info->form_activity.type);
  EXPECT_FALSE(info->form_activity.input_missing);
}

// Tests that a new option element triggers form_changed event.
TEST_F(FormJsTest, AddOption) {
  web::TestWebStateObserver observer(web_state());
  LoadHtml(
      @"<form>"
       "<select id='select1'><option value='CA'>CA</option></select>"
       "</form>");

  ExecuteJavaScript(
      @"__gCrWeb.form.trackFormMutations(10);"
      @"var option = document.createElement('option');"
      @"document.getElementById('select1').appendChild(option);");
  web::TestWebStateObserver* block_observer = &observer;
  __block web::TestFormActivityInfo* info = nil;
  WaitForCondition(^{
    info = block_observer->form_activity_info();
    return info != nil;
  });
  EXPECT_EQ("form_changed", info->form_activity.type);
  EXPECT_FALSE(info->form_activity.input_missing);
}
