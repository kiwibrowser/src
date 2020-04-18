// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Foundation/Foundation.h>

#import "ios/chrome/browser/passwords/js_password_manager.h"
#import "ios/web/public/test/web_js_test.h"
#import "ios/web/public/test/web_test_with_web_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Unit tests for ios/chrome/browser/web/resources/password_controller.js
namespace {

// Text fixture to test password controller.
class PasswordControllerJsTest
    : public web::WebJsTest<web::WebTestWithWebState> {
 public:
  PasswordControllerJsTest()
      : web::WebJsTest<web::WebTestWithWebState>(
            @[ @"chrome_bundle_all_frames", @"chrome_bundle_main_frame" ]) {}
};

// IDs used in the Username and Password <input> elements.
NSString* const kEmailInputID = @"Email";
NSString* const kPasswordInputID = @"Passwd";

// Returns an autoreleased string of an HTML form that is similar to the
// Google Accounts sign in form. |email| may be nil if the form does not
// need to be pre-filled with the username. Use |isReadOnly| flag to indicate
// if the email field should be read-only.
NSString* GAIASignInForm(NSString* formAction,
                         NSString* email,
                         BOOL isReadOnly) {
  return [NSString
      stringWithFormat:
          @"<html><body>"
           "<form novalidate action=\"%@\" "
           "id=\"gaia_loginform\">"
           "  <input name=\"GALX\" type=\"hidden\" value=\"abcdefghij\">"
           "  <input name=\"service\" type=\"hidden\" value=\"mail\">"
           "  <input id=\"%@\" name=\"Email\" type=\"email\" value=\"%@\" %@>"
           "  <input id=\"%@\" name=\"Passwd\" type=\"password\" "
           "    placeholder=\"Password\">"
           "</form></body></html>",
          formAction, kEmailInputID, email ? email : @"",
          isReadOnly ? @"readonly" : @"", kPasswordInputID];
}

// Returns an autoreleased string of JSON for a parsed form.
NSString* GAIASignInFormData(NSString* formAction) {
  return [NSString stringWithFormat:@"{"
                                     "  \"action\":\"%@\","
                                     "  \"origin\":\"%@\","
                                     "  \"fields\":["
                                     "    {\"name\":\"%@\", \"value\":\"\"},"
                                     "    {\"name\":\"%@\",\"value\":\"\"}"
                                     "  ]"
                                     "}",
                                    formAction, formAction, kEmailInputID,
                                    kPasswordInputID];
}

// Loads a page with a password form containing a username value already.
// Checks that an attempt to fill in credentials with the same username
// succeeds.
TEST_F(PasswordControllerJsTest,
       FillPasswordFormWithPrefilledUsername_SucceedsWhenUsernameMatches) {
  NSString* const formAction = @"https://accounts.google.com/ServiceLoginAuth";
  NSString* const username = @"john.doe@gmail.com";
  NSString* const password = @"super!secret";
  LoadHtmlAndInject(GAIASignInForm(formAction, username, YES));
  EXPECT_NSEQ(
      @YES,
      ExecuteJavaScriptWithFormat(
          @"__gCrWeb.passwords.fillPasswordForm(%@, '%@', '%@', '%@')",
          GAIASignInFormData(formAction), username, password, formAction));
  // Verifies that the sign-in form has been filled with username/password.
  ExecuteJavaScriptOnElementsAndCheck(@"document.getElementById('%@').value",
                                      @[ kEmailInputID, kPasswordInputID ],
                                      @[ username, password ]);
}

// Loads a page with a password form containing a username value already.
// Checks that an attempt to fill in credentials with a different username
// fails, as long as the field is read-only.
TEST_F(PasswordControllerJsTest,
       FillPasswordFormWithPrefilledUsername_FailsWhenUsernameMismatched) {
  NSString* const formAction = @"https://accounts.google.com/ServiceLoginAuth";
  NSString* const username1 = @"john.doe@gmail.com";
  NSString* const username2 = @"jean.dubois@gmail.com";
  NSString* const password = @"super!secret";
  LoadHtmlAndInject(GAIASignInForm(formAction, username1, YES));
  EXPECT_NSEQ(
      @NO,
      ExecuteJavaScriptWithFormat(
          @"__gCrWeb.passwords.fillPasswordForm(%@, '%@', '%@', '%@')",
          GAIASignInFormData(formAction), username2, password, formAction));
  // Verifies that the sign-in form has not been filled.
  ExecuteJavaScriptOnElementsAndCheck(@"document.getElementById('%@').value",
                                      @[ kEmailInputID, kPasswordInputID ],
                                      @[ username1, @"" ]);
}

// Loads a page with a password form containing a username value already.
// Checks that an attempt to fill in credentials with a different username
// succeeds, as long as the field is writeable.
TEST_F(PasswordControllerJsTest,
       FillPasswordFormWithPrefilledUsername_SucceedsByOverridingUsername) {
  NSString* const formAction = @"https://accounts.google.com/ServiceLoginAuth";
  NSString* const username1 = @"john.doe@gmail.com";
  NSString* const username2 = @"jane.doe@gmail.com";
  NSString* const password = @"super!secret";
  LoadHtmlAndInject(GAIASignInForm(formAction, username1, NO));
  EXPECT_NSEQ(
      @YES,
      ExecuteJavaScriptWithFormat(
          @"__gCrWeb.passwords.fillPasswordForm(%@, '%@', '%@', '%@')",
          GAIASignInFormData(formAction), username2, password, formAction));
  // Verifies that the sign-in form has been filled with the new username
  // and password.
  ExecuteJavaScriptOnElementsAndCheck(@"document.getElementById('%@').value",
                                      @[ kEmailInputID, kPasswordInputID ],
                                      @[ username2, password ]);
}

// Check that one password form is identified and serialized correctly.
TEST_F(PasswordControllerJsTest,
       FindAndPreparePasswordFormsSingleFrameSingleForm) {
  LoadHtmlAndInject(
      @"<html><body>"
       "<form action='/generic_submit' method='post' name='login_form'>"
       "  Name: <input type='text' name='name'>"
       "  Password: <input type='password' name='password'>"
       "  <input type='submit' value='Submit'>"
       "</form>"
       "</body></html>");

  const std::string base_url = BaseUrl();
  NSString* result = [NSString
      stringWithFormat:
          @"[{\"name\":\"login_form\",\"origin\":\"%s\",\"action\":\"https://"
          @"chromium.test/"
          @"generic_submit\",\"fields\":[{\"identifier\":\"name\",\"name\":"
          @"\"name\",\"form_control_type\":\"text\",\"should_autocomplete\":"
          @"true,\"is_focusable\":true,\"max_length\":524288,\"is_checkable\":"
          @"false,\"value\":\"\",\"label\":\"Name:\"},{\"identifier\":"
          @"\"password\",\"name\":\"password\",\"form_control_type\":"
          @"\"password\",\"should_autocomplete\":true,\"is_focusable\":true,"
          @"\"max_length\":524288,\"is_checkable\":false,\"value\":\"\","
          @"\"label\":\"Password:\"}]}]",
          base_url.c_str()];
  EXPECT_NSEQ(result, ExecuteJavaScriptWithFormat(
                          @"__gCrWeb.passwords.findPasswordForms()"));
};

// Check that multiple password forms are identified and serialized correctly.
TEST_F(PasswordControllerJsTest,
       FindAndPreparePasswordFormsSingleFrameMultipleForms) {
  LoadHtmlAndInject(
      @"<html><body>"
       "<form action='/generic_submit' id='login_form1'>"
       "  Name: <input type='text' name='name'>"
       "  Password: <input type='password' name='password'>"
       "  <input type='submit' value='Submit'>"
       "</form>"
       "<form action='/generic_s2' name='login_form2'>"
       "  Name: <input type='text' name='name2'>"
       "  Password: <input type='password' name='password2'>"
       "  <input type='submit' value='Submit'>"
       "</form>"
       "</body></html>");

  const std::string base_url = BaseUrl();
  NSString* result = [NSString
      stringWithFormat:
          @"[{\"name\":\"login_form1\",\"origin\":\"%s\",\"action\":\"%s"
          @"generic_submit\",\"fields\":[{\"identifier\":\"name\",\"name\":"
          @"\"name\",\"form_control_type\":\"text\",\"should_autocomplete\":"
          @"true,\"is_focusable\":true,\"max_length\":524288,\"is_checkable\":"
          @"false,\"value\":\"\",\"label\":\"Name:\"},{\"identifier\":"
          @"\"password\",\"name\":\"password\",\"form_control_type\":"
          @"\"password\",\"should_autocomplete\":true,\"is_focusable\":true,"
          @"\"max_length\":524288,\"is_checkable\":false,\"value\":\"\","
          @"\"label\":\"Password:\"}]},{\"name\":\"login_form2\",\"origin\":"
          @"\"https://chromium.test/\",\"action\":\"https://chromium.test/"
          @"generic_s2\",\"fields\":[{\"identifier\":\"name2\",\"name\":"
          @"\"name2\",\"form_control_type\":\"text\",\"should_autocomplete\":"
          @"true,\"is_focusable\":true,\"max_length\":524288,\"is_checkable\":"
          @"false,\"value\":\"\",\"label\":\"Name:\"},{\"identifier\":"
          @"\"password2\",\"name\":\"password2\",\"form_control_type\":"
          @"\"password\",\"should_autocomplete\":true,\"is_focusable\":true,"
          @"\"max_length\":524288,\"is_checkable\":false,\"value\":\"\","
          @"\"label\":\"Password:\"}]}]",
          base_url.c_str(), base_url.c_str()];

  EXPECT_NSEQ(result, ExecuteJavaScriptWithFormat(
                          @"__gCrWeb.passwords.findPasswordForms()"));
};

// Test serializing of password forms.
TEST_F(PasswordControllerJsTest, GetPasswordFormData) {
  LoadHtmlAndInject(
      @"<html><body>"
       "<form name='np' id='np1' action='/generic_submit'>"
       "  Name: <input type='text' name='name'>"
       "  Password: <input type='password' name='password'>"
       "  <input type='submit' value='Submit'>"
       "</form>"
       "</body></html>");

  const std::string base_url = BaseUrl();
  NSString* parameter = @"window.document.getElementsByTagName('form')[0]";

  NSString* result = [NSString
      stringWithFormat:
          @"{\"name\":\"np\",\"origin\":\"%s\",\"action\":\"%sgeneric_submit\","
          @"\"fields\":[{\"identifier\":\"name\",\"name\":\"name\",\"form_"
          @"control_type\":\"text\",\"should_autocomplete\":true,\"is_"
          @"focusable\":true,\"max_length\":524288,\"is_checkable\":false,"
          @"\"value\":\"\",\"label\":\"Name:\"},{\"identifier\":\"password\","
          @"\"name\":\"password\",\"form_control_type\":\"password\",\"should_"
          @"autocomplete\":true,\"is_focusable\":true,\"max_length\":524288,"
          @"\"is_checkable\":false,\"value\":\"\",\"label\":\"Password:\"}]}",
          base_url.c_str(), base_url.c_str()];

  EXPECT_NSEQ(
      result,
      ExecuteJavaScriptWithFormat(
          @"__gCrWeb.stringify(__gCrWeb.passwords.getPasswordFormData(%@))",
          parameter));
};

// Check that if a form action is not set then the action is parsed to the
// current url.
TEST_F(PasswordControllerJsTest, FormActionIsNotSet) {
  LoadHtmlAndInject(
      @"<html><body>"
       "<form name='login_form'>"
       "  Name: <input type='text' name='name'>"
       "  Password: <input type='password' name='password'>"
       "  <input type='submit' value='Submit'>"
       "</form>"
       "</body></html>");

  const std::string base_url = BaseUrl();
  NSString* result = [NSString
      stringWithFormat:
          @"[{\"name\":\"login_form\",\"origin\":\"%s\",\"action\":\"%s\","
          @"\"fields\":[{\"identifier\":\"name\",\"name\":\"name\",\"form_"
          @"control_type\":\"text\",\"should_autocomplete\":true,\"is_"
          @"focusable\":true,\"max_length\":524288,\"is_checkable\":false,"
          @"\"value\":\"\",\"label\":\"Name:\"},{\"identifier\":\"password\","
          @"\"name\":\"password\",\"form_control_type\":\"password\",\"should_"
          @"autocomplete\":true,\"is_focusable\":true,\"max_length\":524288,"
          @"\"is_checkable\":false,\"value\":\"\",\"label\":\"Password:\"}]}]",
          base_url.c_str(), base_url.c_str()];
  EXPECT_NSEQ(result, ExecuteJavaScriptWithFormat(
                          @"__gCrWeb.passwords.findPasswordForms()"));
};

// Checks that a touchend event from a button which contains in a password form
// works as a submission indicator for this password form.
TEST_F(PasswordControllerJsTest, TouchendAsSubmissionIndicator) {
  LoadHtmlAndInject(
      @"<html><body>"
       "<form name='login_form' id='login_form'>"
       "  Name: <input type='text' name='username'>"
       "  Password: <input type='password' name='password'>"
       "  <button id='submit_button' value='Submit'>"
       "</form>"
       "</body></html>");

  // Call __gCrWeb.passwords.findPasswordForms in order to set an event handler
  // on the button touchend event.
  ExecuteJavaScriptWithFormat(@"__gCrWeb.passwords.findPasswordForms()");

  // Replace __gCrWeb.message.invokeOnHost with mock method for checking of call
  // arguments.
  ExecuteJavaScriptWithFormat(
      @"var invokeOnHostArgument = null;"
       "var invokeOnHostCalls = 0;"
       "__gCrWeb.message.invokeOnHost = function(command) {"
       "  invokeOnHostArgument = command;"
       "  invokeOnHostCalls++;"
       "}");

  // Simulate touchend event on the button.
  ExecuteJavaScriptWithFormat(
      @"document.getElementsByName('username')[0].value = 'user1';"
       "document.getElementsByName('password')[0].value = 'password1';"
       "var e = new UIEvent('touchend');"
       "document.getElementsByTagName('button')[0].dispatchEvent(e);");

  // Check that there was only 1 call for invokeOnHost.
  EXPECT_NSEQ(@1, ExecuteJavaScriptWithFormat(@"invokeOnHostCalls"));

  NSString* expected_command = [NSString
      stringWithFormat:
          @"{\"name\":\"login_form\",\"origin\":\"https://chromium.test/"
          @"\",\"action\":\"%s\",\"fields\":[{\"identifier\":\"username\","
          @"\"name\":\"username\",\"form_control_type\":\"text\",\"should_"
          @"autocomplete\":true,\"is_focusable\":true,\"max_length\":524288,"
          @"\"is_checkable\":false,\"value\":\"user1\",\"label\":\"Name:\"},{"
          @"\"identifier\":\"password\",\"name\":\"password\",\"form_control_"
          @"type\":\"password\",\"should_autocomplete\":true,\"is_focusable\":"
          @"true,\"max_length\":524288,\"is_checkable\":false,\"value\":"
          @"\"password1\",\"label\":\"Password:\"}],\"command\":\"passwordForm."
          @"submitButtonClick\"}",
          BaseUrl().c_str()];

  // Check that invokeOnHost was called with the correct argument.
  EXPECT_NSEQ(
      expected_command,
      ExecuteJavaScriptWithFormat(@"__gCrWeb.stringify(invokeOnHostArgument)"));
};

// Check that a form is filled if url of a page and url in form fill data are
// different only in pathes.
TEST_F(PasswordControllerJsTest, OriginsAreDifferentInPathes) {
  LoadHtmlAndInject(
      @"<html><body>"
       "<form name='login_form' action='action1'>"
       "  Name: <input type='text' name='name' id='name'>"
       "  Password: <input type='password' name='password' id='password'>"
       "  <input type='submit' value='Submit'>"
       "</form>"
       "</body></html>");

  NSString* const username = @"john.doe@gmail.com";
  NSString* const password = @"super!secret";
  std::string page_origin = BaseUrl() + "origin1";
  std::string form_fill_data_origin = BaseUrl() + "origin2";

  NSString* form_fill_data =
      [NSString stringWithFormat:
                    @"{"
                     "  \"action\":\"%s\","
                     "  \"origin\":\"%s\","
                     "  \"fields\":["
                     "    {\"name\":\"name\", \"value\":\"name\"},"
                     "    {\"name\":\"password\",\"value\":\"password\"}"
                     "  ]"
                     "}",
                    page_origin.c_str(), form_fill_data_origin.c_str()];
  EXPECT_NSEQ(@YES,
              ExecuteJavaScriptWithFormat(
                  @"__gCrWeb.passwords.fillPasswordForm(%@, '%@', '%@', '%s')",
                  form_fill_data, username, password, page_origin.c_str()));
  // Verifies that the sign-in form has been filled with username/password.
  ExecuteJavaScriptOnElementsAndCheck(@"document.getElementById('%@').value",
                                      @[ @"name", @"password" ],
                                      @[ username, password ]);
}
}  // namespace
