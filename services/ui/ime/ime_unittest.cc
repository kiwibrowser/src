// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "base/bind.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_test.h"
#include "services/ui/ime/test_ime_driver/public/mojom/constants.mojom.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "services/ui/public/interfaces/ime/ime.mojom.h"
#include "ui/events/event.h"

class TestTextInputClient : public ui::mojom::TextInputClient {
 public:
  explicit TestTextInputClient(ui::mojom::TextInputClientRequest request)
      : binding_(this, std::move(request)) {}

  std::unique_ptr<ui::Event> WaitUntilInsertChar() {
    if (!receieved_event_) {
      run_loop_.reset(new base::RunLoop);
      run_loop_->Run();
      run_loop_.reset();
    }

    return std::move(receieved_event_);
  }

 private:
  void SetCompositionText(const ui::CompositionText& composition) override {}
  void ConfirmCompositionText() override {}
  void ClearCompositionText() override {}
  void InsertText(const base::string16& text) override {}
  void InsertChar(std::unique_ptr<ui::Event> event) override {
    receieved_event_ = std::move(event);
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
  std::unique_ptr<ui::Event> receieved_event_;

  DISALLOW_COPY_AND_ASSIGN(TestTextInputClient);
};

class IMEAppTest : public service_manager::test::ServiceTest {
 public:
  IMEAppTest() : ServiceTest("ime_unittests") {}
  ~IMEAppTest() override {}

  // service_manager::test::ServiceTest:
  void SetUp() override {
    ServiceTest::SetUp();
    // test_ime_driver will register itself as the current IMEDriver.
    connector()->StartService(test_ime_driver::mojom::kServiceName);
    connector()->BindInterface(ui::mojom::kServiceName, &ime_driver_);
  }

  bool ProcessKeyEvent(ui::mojom::InputMethodPtr* input_method,
                       std::unique_ptr<ui::Event> event) {
    (*input_method)
        ->ProcessKeyEvent(std::move(event),
                          base::Bind(&IMEAppTest::ProcessKeyEventCallback,
                                     base::Unretained(this)));

    run_loop_.reset(new base::RunLoop);
    run_loop_->Run();
    run_loop_.reset();

    return handled_;
  }

 protected:
  void ProcessKeyEventCallback(bool handled) {
    handled_ = handled;
    run_loop_->Quit();
  }

  ui::mojom::IMEDriverPtr ime_driver_;
  std::unique_ptr<base::RunLoop> run_loop_;
  bool handled_;

  DISALLOW_COPY_AND_ASSIGN(IMEAppTest);
};

// Tests sending a KeyEvent to the IMEDriver through the Mus IMEDriver.
TEST_F(IMEAppTest, ProcessKeyEvent) {
  ui::mojom::InputMethodPtr input_method;
  ui::mojom::StartSessionDetailsPtr details =
      ui::mojom::StartSessionDetails::New();
  TestTextInputClient client(MakeRequest(&details->client));
  details->input_method_request = MakeRequest(&input_method);
  ime_driver_->StartSession(std::move(details));

  // Send character key event.
  ui::KeyEvent char_event('A', ui::VKEY_A, 0);
  EXPECT_TRUE(ProcessKeyEvent(&input_method, ui::Event::Clone(char_event)));

  std::unique_ptr<ui::Event> received_event = client.WaitUntilInsertChar();
  ASSERT_TRUE(received_event && received_event->IsKeyEvent());

  ui::KeyEvent* received_key_event = received_event->AsKeyEvent();
  EXPECT_EQ(ui::ET_KEY_PRESSED, received_key_event->type());
  EXPECT_TRUE(received_key_event->is_char());
  EXPECT_EQ(char_event.GetCharacter(), received_key_event->GetCharacter());

  // Send non-character key event.
  ui::KeyEvent nonchar_event(ui::ET_KEY_PRESSED, ui::VKEY_LEFT, 0);
  EXPECT_FALSE(ProcessKeyEvent(&input_method, ui::Event::Clone(nonchar_event)));
}
