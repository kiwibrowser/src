// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/password_manager/chrome_password_manager_client.h"
#include "chrome/browser/password_manager/password_manager_test_base.h"
#include "chrome/browser/password_manager/password_store_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/autofill/password_generation_popup_observer.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/common/autofill_switches.h"
#include "components/password_manager/core/browser/password_generation_manager.h"
#include "components/password_manager/core/browser/test_password_store.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace {

class TestPopupObserver : public autofill::PasswordGenerationPopupObserver {
 public:
  TestPopupObserver()
      : popup_showing_(false),
        password_visible_(false) {}
  virtual ~TestPopupObserver() {}

  void OnPopupShown(bool password_visible) override {
    popup_showing_ = true;
    password_visible_ = password_visible;
  }

  void OnPopupHidden() override { popup_showing_ = false; }

  bool popup_showing() { return popup_showing_; }
  bool password_visible() { return password_visible_; }

 private:
  bool popup_showing_;
  bool password_visible_;
};

}  // namespace

class PasswordGenerationInteractiveTest :
    public PasswordManagerBrowserTestBase {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    PasswordManagerBrowserTestBase::SetUpCommandLine(command_line);

    // Make sure the feature is enabled.
    command_line->AppendSwitch(autofill::switches::kEnablePasswordGeneration);

    // Don't require ping from autofill or blacklist checking.
    command_line->AppendSwitch(
        autofill::switches::kLocalHeuristicsOnlyForPasswordGeneration);
  }

  void SetUpOnMainThread() override {
    PasswordManagerBrowserTestBase::SetUpOnMainThread();

    // Disable Autofill requesting access to AddressBook data. This will cause
    // the tests to hang on Mac.
    autofill::test::DisableSystemServices(browser()->profile()->GetPrefs());

    // Set observer for popup.
    ChromePasswordManagerClient* client =
        ChromePasswordManagerClient::FromWebContents(WebContents());
    client->SetTestObserver(&observer_);

    NavigateToFile("/password/signup_form.html");
  }

  void TearDownOnMainThread() override {
    PasswordManagerBrowserTestBase::TearDownOnMainThread();

    // Clean up UI.
    ChromePasswordManagerClient* client =
        ChromePasswordManagerClient::FromWebContents(WebContents());
    client->HidePasswordGenerationPopup();

    autofill::test::ReenableSystemServices();
  }

  std::string GetFieldValue(const std::string& field_id) {
    std::string value;
    EXPECT_TRUE(content::ExecuteScriptAndExtractString(
        RenderViewHost(),
        "window.domAutomationController.send("
        "    document.getElementById('" + field_id + "').value);",
        &value));
    return value;
  }

  std::string GetFocusedElement() {
    std::string focused_element;
    EXPECT_TRUE(content::ExecuteScriptAndExtractString(
        RenderViewHost(),
        "window.domAutomationController.send("
        "    document.activeElement.id)",
        &focused_element));
    return focused_element;
  }

  void FocusPasswordField() {
    ASSERT_TRUE(content::ExecuteScript(
        RenderViewHost(),
        "document.getElementById('password_field').focus()"));
  }

  void SendKeyToPopup(ui::KeyboardCode key) {
    content::NativeWebKeyboardEvent event(
        blink::WebKeyboardEvent::kRawKeyDown,
        blink::WebInputEvent::kNoModifiers,
        blink::WebInputEvent::GetStaticTimeStampForTests());
    event.windows_key_code = key;
    RenderViewHost()->GetWidget()->ForwardKeyboardEvent(event);
  }

  bool GenerationPopupShowing() {
    return observer_.popup_showing() && observer_.password_visible();
  }

  bool EditingPopupShowing() {
    return observer_.popup_showing() && !observer_.password_visible();
  }

 private:
  TestPopupObserver observer_;
};

IN_PROC_BROWSER_TEST_F(PasswordGenerationInteractiveTest,
                       PopupShownAndPasswordSelected) {
  FocusPasswordField();
  EXPECT_TRUE(GenerationPopupShowing());
  SendKeyToPopup(ui::VKEY_DOWN);
  SendKeyToPopup(ui::VKEY_RETURN);

  // Selecting the password should fill the field and move focus to the
  // submit button.
  EXPECT_FALSE(GetFieldValue("password_field").empty());
  EXPECT_FALSE(GenerationPopupShowing());
  EXPECT_FALSE(EditingPopupShowing());
  EXPECT_EQ("input_submit_button", GetFocusedElement());

  // Re-focusing the password field should show the editing popup.
  FocusPasswordField();
  EXPECT_TRUE(EditingPopupShowing());
}

IN_PROC_BROWSER_TEST_F(PasswordGenerationInteractiveTest,
                       PopupShownAndDismissed) {
  FocusPasswordField();
  EXPECT_TRUE(GenerationPopupShowing());

  SendKeyToPopup(ui::VKEY_ESCAPE);

  // Popup is dismissed.
  EXPECT_FALSE(GenerationPopupShowing());
}

IN_PROC_BROWSER_TEST_F(PasswordGenerationInteractiveTest,
                       PopupShownAndDismissedByScrolling) {
  FocusPasswordField();
  EXPECT_TRUE(GenerationPopupShowing());

  ASSERT_TRUE(content::ExecuteScript(RenderViewHost(),
                                     "window.scrollTo(100, 0);"));

  EXPECT_FALSE(GenerationPopupShowing());
}

IN_PROC_BROWSER_TEST_F(PasswordGenerationInteractiveTest,
                       GenerationTriggeredInIFrame) {
  NavigateToFile("/password/framed_signup_form.html");

  // Execute the script in the context of the iframe so that it kinda receives a
  // user gesture.
  std::vector<content::RenderFrameHost*> frames = WebContents()->GetAllFrames();
  ASSERT_EQ(2u, frames.size());
  ASSERT_TRUE(frames[0] == RenderFrameHost());

  std::string focus_script =
      "document.getElementById('password_field').focus();";

  ASSERT_TRUE(content::ExecuteScript(frames[1], focus_script));
  EXPECT_TRUE(GenerationPopupShowing());
}

// https://crbug.com/791389
IN_PROC_BROWSER_TEST_F(PasswordGenerationInteractiveTest,
                       DISABLED_AutoSavingGeneratedPassword) {
  scoped_refptr<password_manager::TestPasswordStore> password_store =
      static_cast<password_manager::TestPasswordStore*>(
          PasswordStoreFactory::GetForProfile(
              browser()->profile(), ServiceAccessType::IMPLICIT_ACCESS).get());

  FocusPasswordField();
  EXPECT_TRUE(GenerationPopupShowing());
  SendKeyToPopup(ui::VKEY_DOWN);
  SendKeyToPopup(ui::VKEY_RETURN);

  // Change username.
  std::string focus("document.getElementById('username_field').focus();");
  ASSERT_TRUE(content::ExecuteScript(WebContents(), focus));
  content::SimulateKeyPress(WebContents(), ui::DomKey::FromCharacter('U'),
                            ui::DomCode::US_U, ui::VKEY_U, false, false, false,
                            false);
  content::SimulateKeyPress(WebContents(), ui::DomKey::FromCharacter('N'),
                            ui::DomCode::US_N, ui::VKEY_N, false, false, false,
                            false);

  // Submit form.
  NavigationObserver observer(WebContents());
  std::string submit_script =
      "document.getElementById('input_submit_button').click()";
  ASSERT_TRUE(content::ExecuteScript(RenderViewHost(), submit_script));
  observer.Wait();

  WaitForPasswordStore();
  EXPECT_FALSE(password_store->IsEmpty());

  // Make sure the username is correct.
  password_manager::TestPasswordStore::PasswordMap stored_passwords =
      password_store->stored_passwords();
  EXPECT_EQ(1u, stored_passwords.size());
  EXPECT_EQ(1u, stored_passwords.begin()->second.size());
  EXPECT_EQ(base::UTF8ToUTF16("UN"),
            (stored_passwords.begin()->second)[0].username_value);
}
