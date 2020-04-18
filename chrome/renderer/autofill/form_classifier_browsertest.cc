// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/test/base/chrome_render_view_test.h"
#include "components/autofill/content/renderer/form_classifier.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_runtime_features.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_form_element.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "ui/native_theme/native_theme_features.h"

namespace autofill {

class FormClassifierTest : public ChromeRenderViewTest {
 public:
  FormClassifierTest() {}

  void SetUp() override {
    blink::WebRuntimeFeatures::EnableOverlayScrollbars(
        ui::IsOverlayScrollbarEnabled());
    ChromeRenderViewTest::SetUp();
  }

  void TearDown() override {
    LoadHTML("");
    ChromeRenderViewTest::TearDown();
  }

  bool GetGenerationField(std::string* generation_field) {
    blink::WebDocument document = GetMainFrame()->GetDocument();
    blink::WebFormElement form =
        document.GetElementById("test_form").To<blink::WebFormElement>();
    base::string16 generation_field16;
    bool generation_availalbe =
        ClassifyFormAndFindGenerationField(form, &generation_field16);
    *generation_field = base::UTF16ToUTF8(generation_field16);
    return generation_availalbe;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FormClassifierTest);
};

const char kSigninFormHTML[] =
    "<FORM id = 'test_form'> "
    "  <SELECT id='account_type'>"
    "    <OPTION value = 'personal'>"
    "    <OPTION value = 'corporate'>"
    "  </SELECT>"
    "  <INPUT type = 'text' id = 'username'/>"
    "  <INPUT type = 'password' id = 'password'/>"
    "  <INPUT type = 'checkbox' id = 'remember_me'/>"
    "  <INPUT type = 'checkbox' id = 'secure_login'/>"
    "  <INPUT type = 'submit' id = 'signin' />"
    "  <INPUT type = 'hidden' id = 'ignore_this' />"
    "  <INPUT type = 'hidden' id = 'ignore_this_too' />"
    "  <INPUT type = 'submit' id = 'submit' />"
    "</FORM>";

const char kSignupFormWithSeveralTextFieldsFormHTML[] =
    "<FORM id = 'test_form'> "
    "  <INPUT type = 'text' id = 'full_name'/>"
    "  <INPUT type = 'text' id = 'username'/>"
    "  <INPUT type = 'password' id = 'password'/>"
    "  <INPUT type = 'submit' id = 'submit' />"
    "</FORM>";

const char kSignupFormWithSeveralPasswordFieldsHTML[] =
    "<FORM id = 'test_form'> "
    "  <INPUT type = 'text' id = 'username'/>"
    "  <INPUT type = 'password' id = 'password'/>"
    "  <INPUT type = 'password' id = 'confirm_password'/>"
    "  <INPUT type = 'submit' id = 'submit' />"
    "</FORM>";

const char kSignupFormWithManyCheckboxesHTML[] =
    "<FORM id = 'test_form'> "
    "  </SELECT>"
    "  <INPUT type = 'text' id = 'username' />"
    "  <INPUT type = 'password' id = 'password' />"
    "  <INPUT type = 'checkbox' id = 'subscribe_science' />"
    "  <INPUT type = 'checkbox' id = 'subscribe_music' />"
    "  <INPUT type = 'checkbox' id = 'subscribe_sport' />"
    "  <INPUT type = 'submit' id = 'submit' />"
    "</FORM>";

const char kSignupFormWithOtherFieldsHTML[] =
    "<FORM id = 'test_form'> "
    "  <INPUT type = 'text' id = 'username' />"
    "  <INPUT type = 'password' id = 'password' />"
    "  <INPUT type = 'color' id = 'account_color' />"
    "  <INPUT type = 'date' id = 'date_of_birth' />"
    "  <INPUT type = 'submit' id = 'submit' />"
    "</FORM>";

const char kSignupFormWithTextFeatureInInputElementHTML[] =
    "<FORM id = 'test_form'> "
    "  <INPUT type = 'text' id = 'username' class = 'sign-up_field' />"
    "  <INPUT type = 'password' id = 'password' class = 'sign-up_field' />"
    "  <INPUT type = 'submit' id = 'submit' />"
    "</FORM>";

const char kSignupFormWithTextFeatureInFormTagHTML[] =
    "<FORM id = 'test_form' some_attribute='sign_up_form' > "
    "  <INPUT type = 'text' id = 'username' />"
    "  <INPUT type = 'password' id = 'password' />"
    "  <INPUT type = 'submit' id = 'submit' />"
    "</FORM>";

const char kSigninFormWithInvisibleFieldsHTML[] =
    "<FORM id = 'test_form'> "
    "  <INPUT type = 'text' id = 'username' />"
    "  <INPUT type = 'password' id = 'password' />"
    "  <INPUT type = 'input' hidden id = 'hidden_field1' />"
    "  <INPUT type = 'password' hidden id = 'hidden_field2'/>"
    "  <INPUT type = 'submit' id = 'submit' />"
    "</FORM>";

const char kSignupFormWithSigninButtonHTML[] =
    "<FORM id = 'test_form' >"
    "  <INPUT type = 'text' id = 'username' />"
    "  <INPUT type = 'password' id = 'password' />"
    "  <INPUT type = 'password' id = 'confirm_password' />"
    "  <INPUT type = 'submit' id = 'submit' />"
    "  <INPUT type = 'button' id = 'goto_signin_form' />"
    "  <INPUT type = 'image' id = 'goto_auth_form' />"
    "</FORM>";

const char kSomeFormWithoutPasswordFields[] =
    "<FORM id = 'test_form' >"
    "  <INPUT type = 'text' id = 'username' />"
    "  <INPUT type = 'text' id = 'fullname' />"
    "  <INPUT type = 'text' id = 'address' />"
    "  <INPUT type = 'text' id = 'phone' />"
    "  <INPUT type = 'submit' id = 'submit' />"
    "</FORM>";

const char kSignupFormWithSigninTextFeatureAndManyFieldsHTML[] =
    "<FORM id = 'test_form' class = 'log-on_container'> "
    "  <INPUT type = 'text' id = 'fullname' />"
    "  <INPUT type = 'text' id = 'username' />"
    "  <INPUT type = 'password' id = 'password' />"
    "  <INPUT type = 'submit' id = 'submit' />"
    "</FORM>";

const char kChangeFormWithTreePasswordFieldsHTML[] =
    "<FORM id = 'test_form' >"
    "  <INPUT type = 'password' id = 'old_password' />"
    "  <INPUT type = 'password' id = 'password' />"
    "  <INPUT type = 'password' id = 'confirm_password' />"
    "  <INPUT type = 'submit' id = 'submit' />"
    "</FORM>";

TEST_F(FormClassifierTest, SigninForm) {
  // Signin form with as many as possible visible elements,
  // i.e. if one more text/password/checkbox/other field is added, the form
  // will be recognized as a signup form.
  LoadHTML(kSigninFormHTML);
  std::string generation_field;
  EXPECT_FALSE(GetGenerationField(&generation_field));
}

TEST_F(FormClassifierTest, SignupFormWithSeveralTextFields) {
  LoadHTML(kSignupFormWithSeveralTextFieldsFormHTML);
  std::string generation_field;
  EXPECT_TRUE(GetGenerationField(&generation_field));
  EXPECT_EQ("password", generation_field);
}

TEST_F(FormClassifierTest, SignupFormWithSeveralPasswordFieldsHTML) {
  LoadHTML(kSignupFormWithSeveralPasswordFieldsHTML);
  std::string generation_field;
  EXPECT_TRUE(GetGenerationField(&generation_field));
  EXPECT_EQ("password", generation_field);
}

TEST_F(FormClassifierTest, SignupFormWithManyCheckboxesHTML) {
  LoadHTML(kSignupFormWithManyCheckboxesHTML);
  std::string generation_field;
  EXPECT_TRUE(GetGenerationField(&generation_field));
  EXPECT_EQ("password", generation_field);
}

TEST_F(FormClassifierTest, SignupFormWithOtherFieldsHTML) {
  LoadHTML(kSignupFormWithOtherFieldsHTML);
  std::string generation_field;
  EXPECT_TRUE(GetGenerationField(&generation_field));
  EXPECT_EQ("password", generation_field);
}

TEST_F(FormClassifierTest, SignupFormWithTextFeatureInInputElementHTML) {
  LoadHTML(kSignupFormWithTextFeatureInInputElementHTML);
  std::string generation_field;
  EXPECT_TRUE(GetGenerationField(&generation_field));
  EXPECT_EQ("password", generation_field);
}

TEST_F(FormClassifierTest, SigninFormWithTextFeatureInFormTagHTML) {
  LoadHTML(kSignupFormWithTextFeatureInFormTagHTML);
  std::string generation_field;
  EXPECT_TRUE(GetGenerationField(&generation_field));
  EXPECT_EQ("password", generation_field);
}

TEST_F(FormClassifierTest, SigninFormWithInvisibleFieldsHTML) {
  LoadHTML(kSigninFormWithInvisibleFieldsHTML);
  std::string generation_field;
  EXPECT_FALSE(GetGenerationField(&generation_field));
}

TEST_F(FormClassifierTest, SignupFormWithSigninButtonHTML) {
  LoadHTML(kSignupFormWithSigninButtonHTML);
  std::string generation_field;
  EXPECT_TRUE(GetGenerationField(&generation_field));
  EXPECT_EQ("password", generation_field);
}

TEST_F(FormClassifierTest, SomeFormWithoutPasswordFields) {
  LoadHTML(kSomeFormWithoutPasswordFields);
  std::string generation_field;
  EXPECT_FALSE(GetGenerationField(&generation_field));
}

TEST_F(FormClassifierTest, SignupFormWithSigninTextFeatureAndManyFieldsHTML) {
  // Even if there is signin text feature, the number of fields is more reliable
  // signal of signup form. So, this form should be classified as signup.
  LoadHTML(kSignupFormWithSigninTextFeatureAndManyFieldsHTML);
  std::string generation_field;
  EXPECT_TRUE(GetGenerationField(&generation_field));
  EXPECT_EQ("password", generation_field);
}

TEST_F(FormClassifierTest, kChangeFormWithTreePasswordFieldsHTML) {
  LoadHTML(kChangeFormWithTreePasswordFieldsHTML);
  std::string generation_field;
  EXPECT_TRUE(GetGenerationField(&generation_field));
  EXPECT_EQ("password", generation_field);
}

}  // namespace autofill
