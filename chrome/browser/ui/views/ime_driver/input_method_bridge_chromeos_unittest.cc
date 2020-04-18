// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include <memory>

#include "base/bind.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/views/ime_driver/input_method_bridge_chromeos.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/ime/ime_bridge.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/events/event_utils.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/events/keycodes/dom/dom_key.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"
#include "ui/events/keycodes/keyboard_codes.h"

enum class CompositionEventType {
  SET,
  CONFIRM,
  CLEAR,
  INSERT_TEXT,
  INSERT_CHAR
};

struct CompositionEvent {
  CompositionEventType type;
  base::string16 text_data;
  base::char16 char_data;
};

class TestTextInputClient : public ui::mojom::TextInputClient {
 public:
  explicit TestTextInputClient(ui::mojom::TextInputClientRequest request)
      : binding_(this, std::move(request)) {}

  CompositionEvent WaitUntilCompositionEvent() {
    if (!receieved_event_.has_value()) {
      run_loop_ = std::make_unique<base::RunLoop>();
      run_loop_->Run();
      run_loop_.reset();
    }
    CompositionEvent result = receieved_event_.value();
    receieved_event_.reset();
    return result;
  }

 private:
  void SetCompositionText(const ui::CompositionText& composition) override {
    CompositionEvent ev = {CompositionEventType::SET, composition.text, 0};
    receieved_event_ = ev;
    if (run_loop_)
      run_loop_->Quit();
  }
  void ConfirmCompositionText() override {
    CompositionEvent ev = {CompositionEventType::CONFIRM, base::string16(), 0};
    receieved_event_ = ev;
    if (run_loop_)
      run_loop_->Quit();
  }
  void ClearCompositionText() override {
    CompositionEvent ev = {CompositionEventType::CLEAR, base::string16(), 0};
    receieved_event_ = ev;
    if (run_loop_)
      run_loop_->Quit();
  }
  void InsertText(const base::string16& text) override {
    CompositionEvent ev = {CompositionEventType::INSERT_TEXT, text, 0};
    receieved_event_ = ev;
    if (run_loop_)
      run_loop_->Quit();
  }
  void InsertChar(std::unique_ptr<ui::Event> event) override {
    ASSERT_TRUE(event->IsKeyEvent());
    CompositionEvent ev = {CompositionEventType::INSERT_CHAR, base::string16(),
                           event->AsKeyEvent()->GetCharacter()};
    receieved_event_ = ev;
    if (run_loop_)
      run_loop_->Quit();
  }
  void DispatchKeyEventPostIME(
      std::unique_ptr<ui::Event> event,
      DispatchKeyEventPostIMECallback callback) override {
    std::move(callback).Run(false);
  }

  mojo::Binding<ui::mojom::TextInputClient> binding_;
  std::unique_ptr<base::RunLoop> run_loop_;
  base::Optional<CompositionEvent> receieved_event_;

  DISALLOW_COPY_AND_ASSIGN(TestTextInputClient);
};

class InputMethodBridgeChromeOSTest : public testing::Test {
 public:
  InputMethodBridgeChromeOSTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP) {}
  ~InputMethodBridgeChromeOSTest() override {}

  void SetUp() override {
    ui::IMEBridge::Initialize();

    ui::mojom::TextInputClientPtr client_ptr;
    client_ = std::make_unique<TestTextInputClient>(MakeRequest(&client_ptr));
    input_method_ = std::make_unique<InputMethodBridge>(
        std::make_unique<RemoteTextInputClient>(
            std::move(client_ptr), ui::TEXT_INPUT_TYPE_TEXT,
            ui::TEXT_INPUT_MODE_DEFAULT, base::i18n::LEFT_TO_RIGHT, 0,
            gfx::Rect()));
  }

  bool ProcessKeyEvent(std::unique_ptr<ui::Event> event) {
    handled_.reset();

    input_method_->ProcessKeyEvent(
        std::move(event),
        base::Bind(&InputMethodBridgeChromeOSTest::ProcessKeyEventCallback,
                   base::Unretained(this)));

    if (!handled_.has_value()) {
      run_loop_ = std::make_unique<base::RunLoop>();
      run_loop_->Run();
      run_loop_.reset();
    }

    return handled_.value();
  }

  std::unique_ptr<ui::Event> UnicodeKeyPress(ui::KeyboardCode vkey,
                                             ui::DomCode code,
                                             int flags,
                                             base::char16 character) const {
    return std::make_unique<ui::KeyEvent>(ui::ET_KEY_PRESSED, vkey, code, flags,
                                          ui::DomKey::FromCharacter(character),
                                          ui::EventTimeForNow());
  }

 protected:
  void ProcessKeyEventCallback(bool handled) {
    handled_ = handled;
    if (run_loop_)
      run_loop_->Quit();
  }

  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<TestTextInputClient> client_;
  std::unique_ptr<InputMethodBridge> input_method_;
  std::unique_ptr<base::RunLoop> run_loop_;
  base::Optional<bool> handled_;

  DISALLOW_COPY_AND_ASSIGN(InputMethodBridgeChromeOSTest);
};

// Tests if hexadecimal composition provided by ui::CharacterComposer works
// correctly. ui::CharacterComposer is tried if no input method extensions
// have been registered yet.
TEST_F(InputMethodBridgeChromeOSTest, HexadecimalComposition) {
  struct {
    ui::KeyboardCode vkey;
    ui::DomCode code;
    int flags;
    base::char16 character;
    std::string composition_text;
  } kTestSequence[] = {
      {ui::VKEY_U, ui::DomCode::US_U, ui::EF_SHIFT_DOWN | ui::EF_CONTROL_DOWN,
       'U', "u"},
      {ui::VKEY_3, ui::DomCode::DIGIT3, 0, '3', "u3"},
      {ui::VKEY_0, ui::DomCode::DIGIT0, 0, '0', "u30"},
      {ui::VKEY_4, ui::DomCode::DIGIT4, 0, '4', "u304"},
      {ui::VKEY_2, ui::DomCode::DIGIT2, 0, '2', "u3042"},
  };

  // Send the Ctrl-Shift-U,3,4,0,2 sequence.
  for (size_t i = 0; i < arraysize(kTestSequence); i++) {
    EXPECT_TRUE(ProcessKeyEvent(
        UnicodeKeyPress(kTestSequence[i].vkey, kTestSequence[i].code,
                        kTestSequence[i].flags, kTestSequence[i].character)));
    CompositionEvent ev = client_->WaitUntilCompositionEvent();
    EXPECT_EQ(CompositionEventType::SET, ev.type);
    EXPECT_EQ(base::UTF8ToUTF16(kTestSequence[i].composition_text),
              ev.text_data);
  }

  // Press the return key and verify that the composition text was converted
  // to the desired text.
  EXPECT_TRUE(ProcessKeyEvent(
      UnicodeKeyPress(ui::VKEY_RETURN, ui::DomCode::ENTER, 0, '\r')));
  CompositionEvent ev = client_->WaitUntilCompositionEvent();
  EXPECT_EQ(CompositionEventType::INSERT_TEXT, ev.type);
  EXPECT_EQ(base::string16(1, 0x3042), ev.text_data);
}

// Test that Ctrl-C, Ctrl-X, and Ctrl-V are not handled.
TEST_F(InputMethodBridgeChromeOSTest, ClipboardAccelerators) {
  EXPECT_FALSE(ProcessKeyEvent(UnicodeKeyPress(ui::VKEY_C, ui::DomCode::US_C,
                                               ui::EF_CONTROL_DOWN, 'C')));
  EXPECT_FALSE(ProcessKeyEvent(UnicodeKeyPress(ui::VKEY_X, ui::DomCode::US_X,
                                               ui::EF_CONTROL_DOWN, 'X')));
  EXPECT_FALSE(ProcessKeyEvent(UnicodeKeyPress(ui::VKEY_V, ui::DomCode::US_V,
                                               ui::EF_CONTROL_DOWN, 'V')));
}
