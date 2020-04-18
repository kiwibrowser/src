// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include <memory>

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/renderer/autofill/fake_content_password_manager_driver.h"
#include "chrome/renderer/autofill/fake_password_manager_client.h"
#include "chrome/renderer/autofill/password_generation_test_utils.h"
#include "chrome/test/base/chrome_render_view_test.h"
#include "components/autofill/content/renderer/autofill_agent.h"
#include "components/autofill/content/renderer/form_autofill_util.h"
#include "components/autofill/content/renderer/test_password_generation_agent.h"
#include "components/autofill/core/common/autofill_switches.h"
#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/password_generation_util.h"
#include "components/password_manager/core/common/password_manager_features.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_widget.h"
#include "ui/events/keycodes/keyboard_codes.h"

using blink::WebDocument;
using blink::WebElement;
using blink::WebInputElement;
using blink::WebNode;
using blink::WebString;

namespace autofill {

class PasswordGenerationAgentTest : public ChromeRenderViewTest {
 public:
  PasswordGenerationAgentTest() {}

  void RegisterMainFrameRemoteInterfaces() override {
    // We only use the fake driver for main frame
    // because our test cases only involve the main frame.
    service_manager::InterfaceProvider* remote_interfaces =
        view_->GetMainRenderFrame()->GetRemoteInterfaces();
    service_manager::InterfaceProvider::TestApi test_api(remote_interfaces);
    test_api.SetBinderForName(
        mojom::PasswordManagerDriver::Name_,
        base::Bind(&PasswordGenerationAgentTest::BindPasswordManagerDriver,
                   base::Unretained(this)));

    // Because the test cases only involve the main frame in this test,
    // the fake password client is only used for the main frame.
    blink::AssociatedInterfaceProvider* remote_associated_interfaces =
        view_->GetMainRenderFrame()->GetRemoteAssociatedInterfaces();
    remote_associated_interfaces->OverrideBinderForTesting(
        mojom::PasswordManagerClient::Name_,
        base::Bind(&PasswordGenerationAgentTest::BindPasswordManagerClient,
                   base::Unretained(this)));
  }

  void TearDown() override {
    LoadHTML("");
    ChromeRenderViewTest::TearDown();
  }

  void LoadHTMLWithUserGesture(const char* html) {
    LoadHTML(html);

    // Enable show-ime event when element is focused by indicating that a user
    // gesture has been processed since load.
    EXPECT_TRUE(SimulateElementClick("dummy"));
  }

  void FocusField(const char* element_id) {
    WebDocument document = GetMainFrame()->GetDocument();
    blink::WebElement element =
        document.GetElementById(blink::WebString::FromUTF8(element_id));
    ASSERT_FALSE(element.IsNull());
    ExecuteJavaScriptForTests(
        base::StringPrintf("document.getElementById('%s').focus();",
                           element_id).c_str());
  }

  void ExpectGenerationAvailable(const char* element_id,
                                 bool available) {
    FocusField(element_id);
    base::RunLoop().RunUntilIdle();
    ASSERT_EQ(available, GetCalledShowPasswordGenerationPopup());
    fake_pw_client_.reset_called_show_pw_generation_popup();
  }

  void AllowToRunFormClassifier() {
    password_generation_->AllowToRunFormClassifier();
  }

  void ExpectFormClassifierVoteReceived(
      bool received,
      const base::string16& expected_generation_element) {
    base::RunLoop().RunUntilIdle();
    if (received) {
      ASSERT_TRUE(fake_driver_.called_save_generation_field());
      EXPECT_EQ(expected_generation_element,
                fake_driver_.save_generation_field());
    } else {
      ASSERT_FALSE(fake_driver_.called_save_generation_field());
    }

    fake_driver_.reset_save_generation_field();
  }

  bool GetCalledShowPasswordGenerationPopup() {
    fake_pw_client_.Flush();
    return fake_pw_client_.called_show_pw_generation_popup();
  }

  void SelectGenerationFallbackInContextMenu(const char* element_id) {
    SimulateElementRightClick(element_id);
    password_generation_->UserSelectedManualGenerationOption();
  }

  void BindPasswordManagerDriver(mojo::ScopedMessagePipeHandle handle) {
    fake_driver_.BindRequest(
        mojom::PasswordManagerDriverRequest(std::move(handle)));
  }

  void BindPasswordManagerClient(mojo::ScopedInterfaceEndpointHandle handle) {
    fake_pw_client_.BindRequest(
        mojom::PasswordManagerClientAssociatedRequest(std::move(handle)));
  }

  void EnableManualGenerationFallback() {
    scoped_feature_list_.InitAndEnableFeature(
        password_manager::features::kEnableManualFallbacksGeneration);
  }

  FakeContentPasswordManagerDriver fake_driver_;
  FakePasswordManagerClient fake_pw_client_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(PasswordGenerationAgentTest);
};

class PasswordGenerationAgentTestForHtmlAnnotation
    : public PasswordGenerationAgentTest {
 public:
  PasswordGenerationAgentTestForHtmlAnnotation() {}

  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kShowAutofillSignatures);
    PasswordGenerationAgentTest::SetUp();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(PasswordGenerationAgentTestForHtmlAnnotation);
};

const char kSigninFormHTML[] =
    "<FORM name = 'blah' action = 'http://www.random.com/'> "
    "  <INPUT type = 'text' id = 'username'/> "
    "  <INPUT type = 'password' id = 'password'/> "
    "  <INPUT type = 'button' id = 'dummy'/> "
    "  <INPUT type = 'submit' value = 'LOGIN' />"
    "</FORM>";

const char kAccountCreationFormHTML[] =
    "<FORM id = 'blah' action = 'http://www.random.com/pa/th?q=1&p=3#first'> "
    "  <INPUT type = 'text' id = 'username'/> "
    "  <INPUT type = 'password' id = 'first_password' size = 5/>"
    "  <INPUT type = 'password' id = 'second_password' size = 5/> "
    "  <INPUT type = 'text' id = 'address'/> "
    "  <INPUT type = 'button' id = 'dummy'/> "
    "  <INPUT type = 'submit' value = 'LOGIN' />"
    "</FORM>";

const char kAccountCreationNoForm[] =
    "<INPUT type = 'text' id = 'username'/> "
    "<INPUT type = 'password' id = 'first_password' size = 5/>"
    "<INPUT type = 'password' id = 'second_password' size = 5/> "
    "<INPUT type = 'text' id = 'address'/> "
    "<INPUT type = 'button' id = 'dummy'/> "
    "<INPUT type = 'submit' value = 'LOGIN' />";

const char kDisabledElementAccountCreationFormHTML[] =
    "<FORM name = 'blah' action = 'http://www.random.com/'> "
    "  <INPUT type = 'text' id = 'username'/> "
    "  <INPUT type = 'password' id = 'first_password' "
    "         autocomplete = 'off' size = 5/>"
    "  <INPUT type = 'password' id = 'second_password' size = 5/> "
    "  <INPUT type = 'text' id = 'address'/> "
    "  <INPUT type = 'text' id = 'disabled' disabled/> "
    "  <INPUT type = 'button' id = 'dummy'/> "
    "  <INPUT type = 'submit' value = 'LOGIN' />"
    "</FORM>";

const char kHiddenPasswordAccountCreationFormHTML[] =
    "<FORM name = 'blah' action = 'http://www.random.com/'> "
    "  <INPUT type = 'text' id = 'username'/> "
    "  <INPUT type = 'password' id = 'first_password'/> "
    "  <INPUT type = 'password' id = 'second_password' style='display:none'/> "
    "  <INPUT type = 'button' id = 'dummy'/> "
    "  <INPUT type = 'submit' value = 'LOGIN' />"
    "</FORM>";

const char kInvalidActionAccountCreationFormHTML[] =
    "<FORM name = 'blah' action = 'invalid'> "
    "  <INPUT type = 'text' id = 'username'/> "
    "  <INPUT type = 'password' id = 'first_password'/> "
    "  <INPUT type = 'password' id = 'second_password'/> "
    "  <INPUT type = 'button' id = 'dummy'/> "
    "  <INPUT type = 'submit' value = 'LOGIN' />"
    "</FORM>";

const char kMultipleAccountCreationFormHTML[] =
    "<FORM name = 'login' action = 'http://www.random.com/'> "
    "  <INPUT type = 'text' id = 'random'/> "
    "  <INPUT type = 'text' id = 'username'/> "
    "  <INPUT type = 'password' id = 'password'/> "
    "  <INPUT type = 'button' id = 'dummy'/> "
    "  <INPUT type = 'submit' value = 'LOGIN' />"
    "</FORM>"
    "<FORM name = 'signup' action = 'http://www.random.com/signup'> "
    "  <INPUT type = 'text' id = 'username'/> "
    "  <INPUT type = 'password' id = 'first_password' "
    "         autocomplete = 'off' size = 5/>"
    "  <INPUT type = 'password' id = 'second_password' size = 5/> "
    "  <INPUT type = 'text' id = 'address'/> "
    "  <INPUT type = 'submit' value = 'LOGIN' />"
    "</FORM>";

const char kBothAutocompleteAttributesFormHTML[] =
    "<FORM name = 'blah' action = 'http://www.random.com/'> "
    "  <INPUT type = 'text' autocomplete='username' id = 'username'/> "
    "  <INPUT type = 'password' id = 'first_password' "
    "         autocomplete = 'new-password' size = 5/>"
    "  <INPUT type = 'password' id = 'second_password' size = 5/> "
    "  <INPUT type = 'button' id = 'dummy'/> "
    "  <INPUT type = 'submit' value = 'LOGIN' />"
    "</FORM>";

const char kUsernameAutocompleteAttributeFormHTML[] =
    "<FORM name = 'blah' action = 'http://www.random.com/'> "
    "  <INPUT type = 'text' autocomplete='username' id = 'username'/> "
    "  <INPUT type = 'password' id = 'first_password' size = 5/>"
    "  <INPUT type = 'password' id = 'second_password' size = 5/> "
    "  <INPUT type = 'button' id = 'dummy'/> "
    "  <INPUT type = 'submit' value = 'LOGIN' />"
    "</FORM>";

const char kNewPasswordAutocompleteAttributeFormHTML[] =
    "<FORM name = 'blah' action = 'http://www.random.com/'> "
    "  <INPUT type = 'text' id = 'username'/> "
    "  <INPUT type = 'password' id = 'first_password' "
    "         autocomplete='new-password' size = 5/>"
    "  <INPUT type = 'password' id = 'second_password' size = 5/> "
    "  <INPUT type = 'button' id = 'dummy'/> "
    "  <INPUT type = 'submit' value = 'LOGIN' />"
    "</FORM>";

const char kCurrentAndNewPasswordAutocompleteAttributeFormHTML[] =
    "<FORM name = 'blah' action = 'http://www.random.com/'> "
    "  <INPUT type = 'password' id = 'old_password' "
    "         autocomplete='current-password'/>"
    "  <INPUT type = 'password' id = 'new_password' "
    "         autocomplete='new-password'/>"
    "  <INPUT type = 'password' id = 'confirm_password' "
    "         autocomplete='new-password'/>"
    "  <INPUT type = 'button' id = 'dummy'/> "
    "  <INPUT type = 'submit' value = 'LOGIN' />"
    "</FORM>";

const char kPasswordChangeFormHTML[] =
    "<FORM name = 'ChangeWithUsernameForm' action = 'http://www.bidule.com'> "
    "  <INPUT type = 'text' id = 'username'/> "
    "  <INPUT type = 'password' id = 'password'/> "
    "  <INPUT type = 'password' id = 'newpassword'/> "
    "  <INPUT type = 'password' id = 'confirmpassword'/> "
    "  <INPUT type = 'button' id = 'dummy'/> "
    "  <INPUT type = 'submit' value = 'Login'/> "
    "</FORM>";

const char kPasswordFormAndSpanHTML[] =
    "<FORM name = 'blah' action = 'http://www.random.com/pa/th?q=1&p=3#first'>"
    "  <INPUT type = 'text' id = 'username'/> "
    "  <INPUT type = 'password' id = 'password'/> "
    "  <INPUT type = 'button' id = 'dummy'/> "
    "</FORM>"
    "<SPAN id='span'>Text to click on</SPAN>";

TEST_F(PasswordGenerationAgentTest, DetectionTest) {
  // Don't shown the icon for non account creation forms.
  LoadHTMLWithUserGesture(kSigninFormHTML);
  ExpectGenerationAvailable("password", false);

  // We don't show the decoration yet because the feature isn't enabled.
  LoadHTMLWithUserGesture(kAccountCreationFormHTML);
  ExpectGenerationAvailable("first_password", false);

  // Pretend like We have received message indicating site is not blacklisted,
  // and we have received message indicating the form is classified as
  // ACCOUNT_CREATION_FORM form Autofill server. We should show the icon.
  LoadHTMLWithUserGesture(kAccountCreationFormHTML);
  SetNotBlacklistedMessage(password_generation_, kAccountCreationFormHTML);
  SetAccountCreationFormsDetectedMessage(password_generation_,
                                         GetMainFrame()->GetDocument(), 0, 1);
  ExpectGenerationAvailable("first_password", true);

  // Hidden fields are not treated differently.
  LoadHTMLWithUserGesture(kHiddenPasswordAccountCreationFormHTML);
  SetNotBlacklistedMessage(password_generation_,
                           kHiddenPasswordAccountCreationFormHTML);
  SetAccountCreationFormsDetectedMessage(password_generation_,
                                         GetMainFrame()->GetDocument(), 0, 1);
  ExpectGenerationAvailable("first_password", true);

  // This doesn't trigger because the form action is invalid.
  LoadHTMLWithUserGesture(kInvalidActionAccountCreationFormHTML);
  SetNotBlacklistedMessage(password_generation_,
                           kInvalidActionAccountCreationFormHTML);
  SetAccountCreationFormsDetectedMessage(password_generation_,
                                         GetMainFrame()->GetDocument(), 0, 1);
  ExpectGenerationAvailable("first_password", false);
}

TEST_F(PasswordGenerationAgentTest, DetectionTestNoForm) {
  LoadHTMLWithUserGesture(kAccountCreationNoForm);
  SetNotBlacklistedMessage(password_generation_, kAccountCreationNoForm);
  std::vector<blink::WebElement> fieldsets;
  std::vector<blink::WebFormControlElement> control_elements =
      form_util::GetUnownedFormFieldElements(
          GetMainFrame()->GetDocument().All(), &fieldsets);
  autofill::FormData form_data;
  UnownedPasswordFormElementsAndFieldSetsToFormData(
      fieldsets, control_elements, nullptr, GetMainFrame()->GetDocument(),
      nullptr /* field_value_and_properties_map */, form_util::EXTRACT_NONE,
      &form_data, nullptr /* FormFieldData */);
  std::vector<autofill::PasswordFormGenerationData> forms;
  forms.push_back(autofill::PasswordFormGenerationData{
      CalculateFormSignature(form_data),
      CalculateFieldSignatureForField(form_data.fields[1])});
  password_generation_->FoundFormsEligibleForGeneration(forms);

  ExpectGenerationAvailable("first_password", true);
  ExpectGenerationAvailable("second_password", false);
}

TEST_F(PasswordGenerationAgentTest, FillTest) {
  // Add event listeners for password fields.
  std::vector<base::string16> variables_to_check;
  std::string events_registration_script =
      CreateScriptToRegisterListeners("first_password", &variables_to_check) +
      CreateScriptToRegisterListeners("second_password", &variables_to_check);

  // Make sure that we are enabled before loading HTML.
  std::string html =
      std::string(kAccountCreationFormHTML) + events_registration_script;
  // Begin with no gesture and therefore no focused element.
  LoadHTML(html.c_str());
  WebDocument document = GetMainFrame()->GetDocument();
  ASSERT_TRUE(document.FocusedElement().IsNull());
  SetNotBlacklistedMessage(password_generation_, html.c_str());
  SetAccountCreationFormsDetectedMessage(password_generation_, document, 0, 1);

  WebElement element =
      document.GetElementById(WebString::FromUTF8("first_password"));
  ASSERT_FALSE(element.IsNull());
  WebInputElement first_password_element = element.To<WebInputElement>();
  element = document.GetElementById(WebString::FromUTF8("second_password"));
  ASSERT_FALSE(element.IsNull());
  WebInputElement second_password_element = element.To<WebInputElement>();

  // Both password fields should be empty.
  EXPECT_TRUE(first_password_element.Value().IsNull());
  EXPECT_TRUE(second_password_element.Value().IsNull());

  base::string16 password = base::ASCIIToUTF16("random_password");
  password_generation_->GeneratedPasswordAccepted(password);

  // Password fields are filled out and set as being autofilled.
  EXPECT_EQ(password, first_password_element.Value().Utf16());
  EXPECT_EQ(password, second_password_element.Value().Utf16());
  EXPECT_TRUE(first_password_element.IsAutofilled());
  EXPECT_TRUE(second_password_element.IsAutofilled());

  // Make sure all events are called.
  for (const base::string16& variable : variables_to_check) {
    int value;
    EXPECT_TRUE(ExecuteJavaScriptAndReturnIntValue(variable, &value));
    EXPECT_EQ(1, value) << variable;
  }

  // Focus moved to the next input field.
  // TODO(zysxqn): Change this back to the address element once Bug 90224
  // https://bugs.webkit.org/show_bug.cgi?id=90224 has been fixed.
  element = document.GetElementById(WebString::FromUTF8("first_password"));
  ASSERT_FALSE(element.IsNull());
  EXPECT_EQ(element, document.FocusedElement());
}

TEST_F(PasswordGenerationAgentTest, EditingTest) {
  LoadHTMLWithUserGesture(kAccountCreationFormHTML);
  SetNotBlacklistedMessage(password_generation_, kAccountCreationFormHTML);
  SetAccountCreationFormsDetectedMessage(password_generation_,
                                         GetMainFrame()->GetDocument(), 0, 1);

  WebDocument document = GetMainFrame()->GetDocument();
  WebElement element =
      document.GetElementById(WebString::FromUTF8("first_password"));
  ASSERT_FALSE(element.IsNull());
  WebInputElement first_password_element = element.To<WebInputElement>();
  element = document.GetElementById(WebString::FromUTF8("second_password"));
  ASSERT_FALSE(element.IsNull());
  WebInputElement second_password_element = element.To<WebInputElement>();

  base::string16 password = base::ASCIIToUTF16("random_password");
  password_generation_->GeneratedPasswordAccepted(password);

  // Passwords start out the same.
  EXPECT_EQ(password, first_password_element.Value().Utf16());
  EXPECT_EQ(password, second_password_element.Value().Utf16());

  // After editing the first field they are still the same.
  std::string edited_password_ascii = "edited_password";
  SimulateUserInputChangeForElement(&first_password_element,
                                    edited_password_ascii);
  base::string16 edited_password = base::ASCIIToUTF16(edited_password_ascii);
  EXPECT_EQ(edited_password, first_password_element.Value().Utf16());
  EXPECT_EQ(edited_password, second_password_element.Value().Utf16());

  fake_driver_.reset_called_password_no_longer_generated();

  // Verify that password mirroring works correctly even when the password
  // is deleted.
  SimulateUserInputChangeForElement(&first_password_element, std::string());
  EXPECT_EQ(base::string16(), first_password_element.Value().Utf16());
  EXPECT_EQ(base::string16(), second_password_element.Value().Utf16());

  // Should have notified the browser that the password is no longer generated
  // and trigger generation again.
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(fake_driver_.called_password_no_longer_generated());
  EXPECT_TRUE(GetCalledShowPasswordGenerationPopup());
}

TEST_F(PasswordGenerationAgentTest, BlacklistedTest) {
  // Did not receive not blacklisted message. Don't show password generation
  // icon.
  LoadHTMLWithUserGesture(kAccountCreationFormHTML);
  SetAccountCreationFormsDetectedMessage(password_generation_,
                                         GetMainFrame()->GetDocument(), 0, 1);
  ExpectGenerationAvailable("first_password", false);

  // Receive one not blacklisted message for non account creation form. Don't
  // show password generation icon.
  LoadHTMLWithUserGesture(kAccountCreationFormHTML);
  SetNotBlacklistedMessage(password_generation_, kSigninFormHTML);
  SetAccountCreationFormsDetectedMessage(password_generation_,
                                         GetMainFrame()->GetDocument(), 0, 1);
  ExpectGenerationAvailable("first_password", false);

  // Receive one not blacklisted message for account creation form. Show
  // password generation icon.
  LoadHTMLWithUserGesture(kAccountCreationFormHTML);
  SetNotBlacklistedMessage(password_generation_, kAccountCreationFormHTML);
  SetAccountCreationFormsDetectedMessage(password_generation_,
                                         GetMainFrame()->GetDocument(), 0, 1);
  ExpectGenerationAvailable("first_password", true);

  // Receive two not blacklisted messages, one is for account creation form and
  // the other is not. Show password generation icon.
  LoadHTMLWithUserGesture(kAccountCreationFormHTML);
  SetNotBlacklistedMessage(password_generation_, kAccountCreationFormHTML);
  SetNotBlacklistedMessage(password_generation_, kSigninFormHTML);
  SetAccountCreationFormsDetectedMessage(password_generation_,
                                         GetMainFrame()->GetDocument(), 0, 1);
  ExpectGenerationAvailable("first_password", true);
}

TEST_F(PasswordGenerationAgentTest, AccountCreationFormsDetectedTest) {
  // Did not receive account creation forms detected message. Don't show
  // password generation icon.
  LoadHTMLWithUserGesture(kAccountCreationFormHTML);
  SetNotBlacklistedMessage(password_generation_, kAccountCreationFormHTML);
  ExpectGenerationAvailable("first_password", false);

  // Receive the account creation forms detected message. Show password
  // generation icon.
  LoadHTMLWithUserGesture(kAccountCreationFormHTML);
  SetNotBlacklistedMessage(password_generation_, kAccountCreationFormHTML);
  SetAccountCreationFormsDetectedMessage(password_generation_,
                                         GetMainFrame()->GetDocument(), 0, 1);
  ExpectGenerationAvailable("first_password", true);
}

TEST_F(PasswordGenerationAgentTest, MaximumOfferSize) {
  base::HistogramTester histogram_tester;

  LoadHTMLWithUserGesture(kAccountCreationFormHTML);
  SetNotBlacklistedMessage(password_generation_, kAccountCreationFormHTML);
  SetAccountCreationFormsDetectedMessage(password_generation_,
                                         GetMainFrame()->GetDocument(), 0, 1);
  ExpectGenerationAvailable("first_password", true);

  WebDocument document = GetMainFrame()->GetDocument();
  WebElement element =
      document.GetElementById(WebString::FromUTF8("first_password"));
  ASSERT_FALSE(element.IsNull());
  WebInputElement first_password_element = element.To<WebInputElement>();

  // Make a password just under maximum offer size.
  SimulateUserInputChangeForElement(
      &first_password_element,
      std::string(password_generation_->kMaximumOfferSize - 1, 'a'));
  // There should now be a message to show the UI.
  EXPECT_TRUE(GetCalledShowPasswordGenerationPopup());
  fake_pw_client_.reset_called_show_pw_generation_popup();

  fake_pw_client_.reset_called_hide_pw_generation_popup();
  // Simulate a user typing a password just over maximum offer size.
  SimulateUserTypingASCIICharacter('a', false);
  SimulateUserTypingASCIICharacter('a', true);
  // There should now be a message to hide the UI.
  fake_pw_client_.Flush();
  EXPECT_TRUE(fake_pw_client_.called_hide_pw_generation_popup());
  fake_pw_client_.reset_called_show_pw_generation_popup();

  // Simulate the user deleting characters. The generation popup should be shown
  // again.
  SimulateUserTypingASCIICharacter(ui::VKEY_BACK, true);
  // There should now be a message to show the UI.
  EXPECT_TRUE(GetCalledShowPasswordGenerationPopup());
  fake_pw_client_.reset_called_show_pw_generation_popup();

  // Change focus. Bubble should be hidden, but that is handled by AutofilAgent,
  // so no messages are sent.
  ExecuteJavaScriptForTests("document.getElementById('username').focus();");
  EXPECT_FALSE(GetCalledShowPasswordGenerationPopup());
  fake_pw_client_.reset_called_show_pw_generation_popup();

  // Focusing the password field will bring up the generation UI again.
  ExecuteJavaScriptForTests(
      "document.getElementById('first_password').focus();");
  EXPECT_TRUE(GetCalledShowPasswordGenerationPopup());
  fake_pw_client_.reset_called_show_pw_generation_popup();

  // Loading a different page triggers UMA stat upload. Verify that only one
  // display event is sent even though
  LoadHTMLWithUserGesture(kSigninFormHTML);

  histogram_tester.ExpectBucketCount(
      "PasswordGeneration.Event",
      autofill::password_generation::GENERATION_POPUP_SHOWN,
      1);
}

TEST_F(PasswordGenerationAgentTest, DynamicFormTest) {
  LoadHTMLWithUserGesture(kSigninFormHTML);
  SetNotBlacklistedMessage(password_generation_, kSigninFormHTML);

  ExecuteJavaScriptForTests(
      "var form = document.createElement('form');"
      "form.action='http://www.random.com';"
      "var username = document.createElement('input');"
      "username.type = 'text';"
      "username.id = 'dynamic_username';"
      "var first_password = document.createElement('input');"
      "first_password.type = 'password';"
      "first_password.id = 'first_password';"
      "first_password.name = 'first_password';"
      "var second_password = document.createElement('input');"
      "second_password.type = 'password';"
      "second_password.id = 'second_password';"
      "second_password.name = 'second_password';"
      "form.appendChild(username);"
      "form.appendChild(first_password);"
      "form.appendChild(second_password);"
      "document.body.appendChild(form);");
  WaitForAutofillDidAssociateFormControl();

  // This needs to come after the DOM has been modified.
  SetAccountCreationFormsDetectedMessage(password_generation_,
                                         GetMainFrame()->GetDocument(), 1, 1);

  // TODO(gcasto): I'm slightly worried about flakes in this test where
  // didAssociateFormControls() isn't called. If this turns out to be a problem
  // adding a call to OnDynamicFormsSeen(GetMainFrame()) will fix it, though
  // it will weaken the test.
  ExpectGenerationAvailable("first_password", true);
}

TEST_F(PasswordGenerationAgentTest, MultiplePasswordFormsTest) {
  // If two forms on the page looks like possible account creation forms, make
  // sure to trigger on the one that is specified from Autofill.
  LoadHTMLWithUserGesture(kMultipleAccountCreationFormHTML);
  SetNotBlacklistedMessage(password_generation_,
                           kMultipleAccountCreationFormHTML);

  // Should trigger on the second form.
  SetAccountCreationFormsDetectedMessage(password_generation_,
                                         GetMainFrame()->GetDocument(), 1, 1);

  ExpectGenerationAvailable("password", false);
  ExpectGenerationAvailable("first_password", true);
}

TEST_F(PasswordGenerationAgentTest, MessagesAfterAccountSignupFormFound) {
  LoadHTMLWithUserGesture(kAccountCreationFormHTML);
  SetNotBlacklistedMessage(password_generation_, kAccountCreationFormHTML);
  SetAccountCreationFormsDetectedMessage(password_generation_,
                                         GetMainFrame()->GetDocument(), 0, 1);

  // Generation should be enabled.
  ExpectGenerationAvailable("first_password", true);

  // Extra not blacklisted messages can be sent. Make sure that they are handled
  // correctly (generation should still be available).
  SetNotBlacklistedMessage(password_generation_, kAccountCreationFormHTML);

  // Need to focus another field first for verification to work.
  ExpectGenerationAvailable("second_password", false);
  ExpectGenerationAvailable("first_password", true);
}

// Losing focus should not trigger a password generation popup.
TEST_F(PasswordGenerationAgentTest, BlurTest) {
  LoadHTMLWithUserGesture(kDisabledElementAccountCreationFormHTML);
  SetNotBlacklistedMessage(password_generation_,
                           kDisabledElementAccountCreationFormHTML);
  SetAccountCreationFormsDetectedMessage(password_generation_,
                                         GetMainFrame()->GetDocument(), 0, 1);

  // Focus on the first password field: password generation popup should show
  // up.
  ExpectGenerationAvailable("first_password", true);

  fake_pw_client_.reset_called_generation_available_for_form();
  // Remove focus from everywhere by clicking an unfocusable element: password
  // generation popup should not show up.
  EXPECT_TRUE(SimulateElementClick("disabled"));
  fake_pw_client_.Flush();
  EXPECT_FALSE(fake_pw_client_.called_generation_available_for_form());
  EXPECT_FALSE(GetCalledShowPasswordGenerationPopup());
}

TEST_F(PasswordGenerationAgentTest, AutocompleteAttributesTest) {
  // Verify that autocomplete attributes can override Autofill to enable
  // generation
  LoadHTMLWithUserGesture(kBothAutocompleteAttributesFormHTML);
  SetNotBlacklistedMessage(password_generation_,
                           kBothAutocompleteAttributesFormHTML);
  ExpectGenerationAvailable("first_password", true);

  // Only username autocomplete attribute enabled doesn't trigger generation.
  LoadHTMLWithUserGesture(kUsernameAutocompleteAttributeFormHTML);
  SetNotBlacklistedMessage(password_generation_,
                           kUsernameAutocompleteAttributeFormHTML);
  ExpectGenerationAvailable("first_password", false);

  // Only new-password autocomplete attribute enabled does trigger generation.
  LoadHTMLWithUserGesture(kNewPasswordAutocompleteAttributeFormHTML);
  SetNotBlacklistedMessage(password_generation_,
                           kNewPasswordAutocompleteAttributeFormHTML);
  ExpectGenerationAvailable("first_password", true);

  // Generation is triggered if the form has only password fields.
  LoadHTMLWithUserGesture(kCurrentAndNewPasswordAutocompleteAttributeFormHTML);
  SetNotBlacklistedMessage(password_generation_,
                           kCurrentAndNewPasswordAutocompleteAttributeFormHTML);
  ExpectGenerationAvailable("old_password", false);
  ExpectGenerationAvailable("new_password", true);
  ExpectGenerationAvailable("confirm_password", false);
}

TEST_F(PasswordGenerationAgentTest, ChangePasswordFormDetectionTest) {
  // Verify that generation is shown on correct field after message receiving.
  LoadHTMLWithUserGesture(kPasswordChangeFormHTML);
  SetNotBlacklistedMessage(password_generation_, kPasswordChangeFormHTML);
  ExpectGenerationAvailable("password", false);
  ExpectGenerationAvailable("newpassword", false);
  ExpectGenerationAvailable("confirmpassword", false);

  SetAccountCreationFormsDetectedMessage(password_generation_,
                                         GetMainFrame()->GetDocument(), 0, 2);
  ExpectGenerationAvailable("password", false);
  ExpectGenerationAvailable("newpassword", true);
  ExpectGenerationAvailable("confirmpassword", false);
}

TEST_F(PasswordGenerationAgentTest, ManualGenerationInFormTest) {
  LoadHTMLWithUserGesture(kAccountCreationFormHTML);
  SelectGenerationFallbackInContextMenu("first_password");
  ExpectGenerationAvailable("first_password", true);
  ExpectGenerationAvailable("second_password", false);
}

TEST_F(PasswordGenerationAgentTest, ManualGenerationNoFormTest) {
  LoadHTMLWithUserGesture(kAccountCreationNoForm);
  SelectGenerationFallbackInContextMenu("first_password");
  ExpectGenerationAvailable("first_password", true);
  ExpectGenerationAvailable("second_password", false);
}

TEST_F(PasswordGenerationAgentTest, ManualGenerationChangeFocusTest) {
  // This test simulates focus change after user triggered password generation.
  // PasswordGenerationAgent should save last focused password element and
  // generate password, even if focused element has changed.
  LoadHTMLWithUserGesture(kAccountCreationFormHTML);
  FocusField("first_password");
  SelectGenerationFallbackInContextMenu("username" /* current focus */);
  ExpectGenerationAvailable("first_password", true);
  ExpectGenerationAvailable("second_password", false);
}

TEST_F(PasswordGenerationAgentTest, PresavingGeneratedPassword) {
  const struct {
    const char* form;
    const char* generation_element;
  } kTestCases[] = {{kAccountCreationFormHTML, "first_password"},
                    {kAccountCreationNoForm, "first_password"},
                    {kPasswordChangeFormHTML, "newpassword"}};
  for (auto& test_case : kTestCases) {
    SCOPED_TRACE(testing::Message("form: ") << test_case.form);
    LoadHTMLWithUserGesture(test_case.form);
    // To be able to work with input elements outside <form>'s, use manual
    // generation.
    SelectGenerationFallbackInContextMenu(test_case.generation_element);
    ExpectGenerationAvailable(test_case.generation_element, true);

    base::string16 password = base::ASCIIToUTF16("random_password");
    password_generation_->GeneratedPasswordAccepted(password);
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(fake_driver_.called_presave_generated_password());
    fake_driver_.reset_called_presave_generated_password();

    FocusField(test_case.generation_element);
    SimulateUserTypingASCIICharacter('a', true);
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(fake_driver_.called_presave_generated_password());
    fake_driver_.reset_called_presave_generated_password();

    for (size_t i = 0; i < password.length(); ++i)
      SimulateUserTypingASCIICharacter(ui::VKEY_BACK, false);
    SimulateUserTypingASCIICharacter(ui::VKEY_BACK, true);
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(fake_driver_.called_password_no_longer_generated());
    fake_driver_.reset_called_password_no_longer_generated();
  }
}

TEST_F(PasswordGenerationAgentTest, FallbackForSaving) {
  LoadHTMLWithUserGesture(kAccountCreationFormHTML);
  SelectGenerationFallbackInContextMenu("first_password");
  ExpectGenerationAvailable("first_password", true);
  EXPECT_EQ(0, fake_driver_.called_show_manual_fallback_for_saving_count());
  password_generation_->GeneratedPasswordAccepted(
      base::ASCIIToUTF16("random_password"));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(fake_driver_.called_presave_generated_password());
  // Two fallback requests are expected because generation changes either new
  // password and confirmation fields.
  EXPECT_EQ(2, fake_driver_.called_show_manual_fallback_for_saving_count());
  // Make sure that generation event was propagated to the browser before the
  // fallback showing. Otherwise, the fallback for saving provides a save bubble
  // instead of a confirmation bubble.
  EXPECT_TRUE(
      fake_driver_.last_fallback_for_saving_was_for_generated_password());
}

TEST_F(PasswordGenerationAgentTest, FormClassifierVotesSignupForm) {
  AllowToRunFormClassifier();
  LoadHTMLWithUserGesture(kAccountCreationFormHTML);
  ExpectFormClassifierVoteReceived(true /* vote is expected */,
                                   base::ASCIIToUTF16("first_password"));
}

TEST_F(PasswordGenerationAgentTest, FormClassifierVotesSigninForm) {
  AllowToRunFormClassifier();
  LoadHTMLWithUserGesture(kSigninFormHTML);
  ExpectFormClassifierVoteReceived(true /* vote is expected */,
                                   base::string16());
}

TEST_F(PasswordGenerationAgentTest, FormClassifierDisabled) {
  LoadHTMLWithUserGesture(kSigninFormHTML);
  ExpectFormClassifierVoteReceived(false /* vote is not expected */,
                                   base::string16());
}

TEST_F(PasswordGenerationAgentTest, ConfirmationFieldVoteFromServer) {
  LoadHTMLWithUserGesture(kPasswordChangeFormHTML);
  SetNotBlacklistedMessage(password_generation_, kPasswordChangeFormHTML);

  WebDocument document = GetMainFrame()->GetDocument();
  blink::WebVector<blink::WebFormElement> web_forms;
  document.Forms(web_forms);
  autofill::FormData form_data;
  WebFormElementToFormData(web_forms[0], blink::WebFormControlElement(),
                           nullptr, form_util::EXTRACT_NONE, &form_data,
                           nullptr /* FormFieldData */);

  std::vector<autofill::PasswordFormGenerationData> forms;
  autofill::PasswordFormGenerationData generation_data(
      CalculateFormSignature(form_data),
      CalculateFieldSignatureForField(form_data.fields[1]));
  generation_data.confirmation_field_signature.emplace(
      CalculateFieldSignatureForField(form_data.fields[3]));
  forms.push_back(generation_data);
  password_generation_->FoundFormsEligibleForGeneration(forms);

  WebElement element =
      document.GetElementById(WebString::FromUTF16(form_data.fields[1].name));
  ASSERT_FALSE(element.IsNull());
  WebInputElement generation_element = element.To<WebInputElement>();
  element =
      document.GetElementById(WebString::FromUTF16(form_data.fields[2].name));
  ASSERT_FALSE(element.IsNull());
  WebInputElement ignored_password_element = element.To<WebInputElement>();
  element =
      document.GetElementById(WebString::FromUTF16(form_data.fields[3].name));
  ASSERT_FALSE(element.IsNull());
  WebInputElement confirmation_password_element = element.To<WebInputElement>();

  base::string16 password = base::ASCIIToUTF16("random_password");
  password_generation_->GeneratedPasswordAccepted(password);
  EXPECT_EQ(password, generation_element.Value().Utf16());
  // Check that the generated password was copied according to the server's
  // response.
  EXPECT_EQ(base::string16(), ignored_password_element.Value().Utf16());
  EXPECT_EQ(password, confirmation_password_element.Value().Utf16());
}

TEST_F(PasswordGenerationAgentTest, RevealPassword) {
  // Checks that revealed password is masked when the field lost focus.
  // Test cases: user click on another input field and on non-focusable element.
  LoadHTMLWithUserGesture(kPasswordFormAndSpanHTML);
  SetNotBlacklistedMessage(password_generation_, kPasswordFormAndSpanHTML);
  SetAccountCreationFormsDetectedMessage(password_generation_,
                                         GetMainFrame()->GetDocument(), 0, 1);
  const char* kGenerationElementId = "password";
  const char* kSpanId = "span";
  const char* kTextFieldId = "username";

  ExpectGenerationAvailable(kGenerationElementId, true);
  password_generation_->GeneratedPasswordAccepted(base::ASCIIToUTF16("pwd"));

  const bool kFalseTrue[] = {false, true};
  for (bool clickOnInputField : kFalseTrue) {
    SCOPED_TRACE(testing::Message("clickOnInputField = ") << clickOnInputField);
    // Click on the generation field to reveal the password value.
    FocusField(kGenerationElementId);

    WebDocument document = GetMainFrame()->GetDocument();
    blink::WebElement element = document.GetElementById(
        blink::WebString::FromUTF8(kGenerationElementId));
    ASSERT_FALSE(element.IsNull());
    blink::WebInputElement input = element.To<WebInputElement>();
    EXPECT_TRUE(input.ShouldRevealPassword());

    // Click on another HTML element.
    const char* const click_target_name =
        clickOnInputField ? kTextFieldId : kSpanId;
    EXPECT_TRUE(SimulateElementClick(click_target_name));
    EXPECT_FALSE(input.ShouldRevealPassword());
  }
}

TEST_F(PasswordGenerationAgentTest, JavascriptClearedTheField) {
  LoadHTMLWithUserGesture(kAccountCreationFormHTML);
  SetNotBlacklistedMessage(password_generation_, kAccountCreationFormHTML);
  SetAccountCreationFormsDetectedMessage(password_generation_,
                                         GetMainFrame()->GetDocument(), 0, 1);

  const char kGenerationElementId[] = "first_password";
  ExpectGenerationAvailable(kGenerationElementId, true);
  password_generation_->GeneratedPasswordAccepted(base::ASCIIToUTF16("pwd"));
  ExecuteJavaScriptForTests(
      "document.getElementById('first_password').value = '';");
  FocusField(kGenerationElementId);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(fake_driver_.called_password_no_longer_generated());
}

TEST_F(PasswordGenerationAgentTest, GenerationFallbackTest) {
  LoadHTMLWithUserGesture(kAccountCreationFormHTML);
  WebDocument document = GetMainFrame()->GetDocument();
  WebElement element =
      document.GetElementById(WebString::FromUTF8("first_password"));
  ASSERT_FALSE(element.IsNull());
  WebInputElement first_password_element = element.To<WebInputElement>();
  EXPECT_TRUE(first_password_element.Value().IsNull());
  SelectGenerationFallbackInContextMenu("first_password");
  EXPECT_TRUE(first_password_element.Value().IsNull());
}

TEST_F(PasswordGenerationAgentTest, GenerationFallback_NoFocusedElement) {
  // Checks the fallback doesn't cause a crash just in case no password element
  // had focus so far.
  LoadHTMLWithUserGesture(kAccountCreationFormHTML);
  password_generation_->UserSelectedManualGenerationOption();
}

TEST_F(PasswordGenerationAgentTest, AutofillToGenerationField) {
  LoadHTMLWithUserGesture(kAccountCreationFormHTML);
  SetNotBlacklistedMessage(password_generation_, kAccountCreationFormHTML);
  SetAccountCreationFormsDetectedMessage(password_generation_,
                                         GetMainFrame()->GetDocument(), 0, 1);
  ExpectGenerationAvailable("first_password", true);

  WebDocument document = GetMainFrame()->GetDocument();
  WebElement element =
      document.GetElementById(WebString::FromUTF8("first_password"));
  ASSERT_FALSE(element.IsNull());
  const WebInputElement input_element = element.To<WebInputElement>();
  // Since password isn't generated (just suitable field was detected),
  // |OnFieldAutofilled| wouldn't trigger any actions.
  password_generation_->OnFieldAutofilled(input_element);
  EXPECT_FALSE(fake_driver_.called_password_no_longer_generated());
}

TEST_F(PasswordGenerationAgentTestForHtmlAnnotation, AnnotateForm) {
  LoadHTMLWithUserGesture(kAccountCreationFormHTML);
  SetNotBlacklistedMessage(password_generation_, kAccountCreationFormHTML);
  SetAccountCreationFormsDetectedMessage(password_generation_,
                                         GetMainFrame()->GetDocument(), 0, 1);
  ExpectGenerationAvailable("first_password", true);
  WebDocument document = GetMainFrame()->GetDocument();

  // Check the form signature is set.
  blink::WebElement form_element =
      document.GetElementById(blink::WebString::FromUTF8("blah"));
  ASSERT_FALSE(form_element.IsNull());
  blink::WebString form_signature =
      form_element.GetAttribute(blink::WebString::FromUTF8("form_signature"));
  ASSERT_FALSE(form_signature.IsNull());
  EXPECT_EQ("3524919054660658462", form_signature.Ascii());

  // Check field signatures are set.
  blink::WebElement username_element =
      document.GetElementById(blink::WebString::FromUTF8("username"));
  ASSERT_FALSE(username_element.IsNull());
  blink::WebString username_signature = username_element.GetAttribute(
      blink::WebString::FromUTF8("field_signature"));
  ASSERT_FALSE(username_signature.IsNull());
  EXPECT_EQ("239111655", username_signature.Ascii());

  blink::WebElement password_element =
      document.GetElementById(blink::WebString::FromUTF8("first_password"));
  ASSERT_FALSE(password_element.IsNull());
  blink::WebString password_signature = password_element.GetAttribute(
      blink::WebString::FromUTF8("field_signature"));
  ASSERT_FALSE(password_signature.IsNull());
  EXPECT_EQ("3933215845", password_signature.Ascii());

  // Check the generation element is marked.
  blink::WebString generation_mark = password_element.GetAttribute(
      blink::WebString::FromUTF8("password_creation_field"));
  ASSERT_FALSE(generation_mark.IsNull());
  EXPECT_EQ("1", generation_mark.Utf8());
}

TEST_F(PasswordGenerationAgentTestForHtmlAnnotation, AnnotateUnownedFields) {
  LoadHTMLWithUserGesture(kAccountCreationNoForm);
  WebDocument document = GetMainFrame()->GetDocument();

  // Check field signatures are set.
  blink::WebElement username_element =
      document.GetElementById(blink::WebString::FromUTF8("username"));
  ASSERT_FALSE(username_element.IsNull());
  blink::WebString username_signature = username_element.GetAttribute(
      blink::WebString::FromUTF8("field_signature"));
  ASSERT_FALSE(username_signature.IsNull());
  EXPECT_EQ("239111655", username_signature.Ascii());

  blink::WebElement password_element =
      document.GetElementById(blink::WebString::FromUTF8("first_password"));
  ASSERT_FALSE(password_element.IsNull());
  blink::WebString password_signature = password_element.GetAttribute(
      blink::WebString::FromUTF8("field_signature"));
  ASSERT_FALSE(password_signature.IsNull());
  EXPECT_EQ("3933215845", password_signature.Ascii());
}

}  // namespace autofill
