// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/password_manager/password_manager_interactive_test_base.h"

#include "content/public/test/browser_test_utils.h"
#include "ui/events/keycodes/dom_us_layout_data.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"

PasswordManagerInteractiveTestBase::PasswordManagerInteractiveTestBase() =
    default;

PasswordManagerInteractiveTestBase::~PasswordManagerInteractiveTestBase() =
    default;

void PasswordManagerInteractiveTestBase::FillElementWithValue(
    const std::string& element_id,
    const std::string& value) {
  ASSERT_TRUE(content::ExecuteScript(
      RenderFrameHost(),
      base::StringPrintf("document.getElementById('%s').focus();",
                         element_id.c_str())));
  for (base::char16 character : value) {
    ui::DomKey dom_key = ui::DomKey::FromCharacter(character);
    const ui::PrintableCodeEntry* code_entry = std::find_if(
        std::begin(ui::kPrintableCodeMap), std::end(ui::kPrintableCodeMap),
        [character](const ui::PrintableCodeEntry& entry) {
          return entry.character[0] == character ||
                 entry.character[1] == character;
        });
    ASSERT_TRUE(code_entry != std::end(ui::kPrintableCodeMap));
    bool shift = code_entry->character[1] == character;
    ui::DomCode dom_code = code_entry->dom_code;
    content::SimulateKeyPress(WebContents(), dom_key, dom_code,
                              ui::DomCodeToUsLayoutKeyboardCode(dom_code),
                              false, shift, false, false);
  }
}

void PasswordManagerInteractiveTestBase::VerifyPasswordIsSavedAndFilled(
    const std::string& filename,
    const std::string& username_id,
    const std::string& password_id,
    const std::string& submission_script) {
  EXPECT_FALSE(password_id.empty());

  NavigateToFile(filename);

  NavigationObserver observer(WebContents());
  const char kUsername[] = "user";
  const char kPassword[] = "123";
  if (!username_id.empty()) {
    FillElementWithValue(username_id, kUsername);
    CheckElementValue(username_id, kUsername);
  }
  FillElementWithValue(password_id, kPassword);
  CheckElementValue(password_id, kPassword);
  ASSERT_TRUE(content::ExecuteScript(RenderFrameHost(), submission_script));
  observer.Wait();
  WaitForPasswordStore();

  BubbleObserver(WebContents()).AcceptSavePrompt();

  // Spin the message loop to make sure the password store had a chance to save
  // the password.
  WaitForPasswordStore();
  CheckThatCredentialsStored(username_id.empty() ? "" : kUsername, kPassword);

  NavigateToFile(filename);

  // Let the user interact with the page, so that DOM gets modification events,
  // needed for autofilling fields.
  content::SimulateMouseClickAt(
      WebContents(), 0, blink::WebMouseEvent::Button::kLeft, gfx::Point(1, 1));

  // Wait until that interaction causes the password value to be revealed.
  if (!username_id.empty())
    WaitForElementValue(username_id, kUsername);
  WaitForElementValue(password_id, kPassword);
}
