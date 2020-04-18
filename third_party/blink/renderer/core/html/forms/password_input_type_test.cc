// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <memory>
#include <utility>

#include "third_party/blink/renderer/core/html/forms/password_input_type.h"

#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/modules/insecure_input/insecure_input_service.mojom-blink.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/html/forms/html_input_element.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"

namespace blink {

class MockInsecureInputService : public mojom::blink::InsecureInputService {
 public:
  explicit MockInsecureInputService(LocalFrame& frame) {
    service_manager::InterfaceProvider::TestApi test_api(
        &frame.GetInterfaceProvider());
    test_api.SetBinderForName(
        mojom::blink::InsecureInputService::Name_,
        WTF::BindRepeating(&MockInsecureInputService::BindRequest,
                           WTF::Unretained(this)));
  }

  ~MockInsecureInputService() override = default;

  void BindRequest(mojo::ScopedMessagePipeHandle handle) {
    binding_set_.AddBinding(
        this, mojom::blink::InsecureInputServiceRequest(std::move(handle)));
  }

  unsigned DidEditFieldCalls() const { return num_did_edit_field_calls_; }

  bool PasswordFieldVisibleCalled() const {
    return password_field_visible_called_;
  }

  unsigned NumPasswordFieldsInvisibleCalls() const {
    return num_password_fields_invisible_calls_;
  }

 private:
  // mojom::InsecureInputService
  void PasswordFieldVisibleInInsecureContext() override {
    password_field_visible_called_ = true;
  }

  void AllPasswordFieldsInInsecureContextInvisible() override {
    ++num_password_fields_invisible_calls_;
  }

  void DidEditFieldInInsecureContext() override { ++num_did_edit_field_calls_; }

  mojo::BindingSet<InsecureInputService> binding_set_;

  bool password_field_visible_called_ = false;
  unsigned num_did_edit_field_calls_ = 0;
  unsigned num_password_fields_invisible_calls_ = 0;
};

// Tests that a Mojo message is sent when a visible password field
// appears on the page.
TEST(PasswordInputTypeTest, PasswordVisibilityEvent) {
  std::unique_ptr<DummyPageHolder> page_holder =
      DummyPageHolder::Create(IntSize(2000, 2000));
  MockInsecureInputService mock_service(page_holder->GetFrame());
  page_holder->GetDocument().body()->SetInnerHTMLFromString(
      "<input type='password'>");
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  blink::test::RunPendingTasks();
  EXPECT_TRUE(mock_service.PasswordFieldVisibleCalled());
}

// Tests that a Mojo message is not sent when a visible password field
// appears in a secure context.
TEST(PasswordInputTypeTest, PasswordVisibilityEventInSecureContext) {
  std::unique_ptr<DummyPageHolder> page_holder =
      DummyPageHolder::Create(IntSize(2000, 2000));
  MockInsecureInputService mock_service(page_holder->GetFrame());
  page_holder->GetDocument().SetURL(KURL("https://example.test"));
  page_holder->GetDocument().SetSecurityOrigin(
      SecurityOrigin::Create(KURL("https://example.test")));
  page_holder->GetDocument().SetSecureContextStateForTesting(
      SecureContextState::kSecure);
  page_holder->GetDocument().body()->SetInnerHTMLFromString(
      "<input type='password'>");
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  // No message should have been sent from a secure context.
  blink::test::RunPendingTasks();
  EXPECT_FALSE(mock_service.PasswordFieldVisibleCalled());
}

// Tests that a Mojo message is sent when a previously invisible password field
// becomes visible.
TEST(PasswordInputTypeTest, InvisiblePasswordFieldBecomesVisible) {
  std::unique_ptr<DummyPageHolder> page_holder =
      DummyPageHolder::Create(IntSize(2000, 2000));
  MockInsecureInputService mock_service(page_holder->GetFrame());
  page_holder->GetDocument().body()->SetInnerHTMLFromString(
      "<input type='password' style='display:none;'>");
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  blink::test::RunPendingTasks();
  // The message should not be sent for a hidden password field.
  EXPECT_FALSE(mock_service.PasswordFieldVisibleCalled());

  // Now make the input visible.
  HTMLInputElement* input =
      ToHTMLInputElement(page_holder->GetDocument().body()->firstChild());
  input->setAttribute("style", "", ASSERT_NO_EXCEPTION);
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  blink::test::RunPendingTasks();
  EXPECT_TRUE(mock_service.PasswordFieldVisibleCalled());
}

// Tests that a Mojo message is sent when a previously non-password field
// becomes a password.
TEST(PasswordInputTypeTest, NonPasswordFieldBecomesPassword) {
  std::unique_ptr<DummyPageHolder> page_holder =
      DummyPageHolder::Create(IntSize(2000, 2000));
  MockInsecureInputService mock_service(page_holder->GetFrame());
  page_holder->GetDocument().body()->SetInnerHTMLFromString(
      "<input type='text'>");
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  // The message should not be sent for a non-password field.
  blink::test::RunPendingTasks();
  EXPECT_FALSE(mock_service.PasswordFieldVisibleCalled());

  // Now make the input a password field.
  HTMLInputElement* input =
      ToHTMLInputElement(page_holder->GetDocument().body()->firstChild());
  input->setType("password");
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  blink::test::RunPendingTasks();
  EXPECT_TRUE(mock_service.PasswordFieldVisibleCalled());
}

// Tests that a Mojo message is *not* sent when a previously invisible password
// field becomes a visible non-password field.
TEST(PasswordInputTypeTest,
     InvisiblePasswordFieldBecomesVisibleNonPasswordField) {
  std::unique_ptr<DummyPageHolder> page_holder =
      DummyPageHolder::Create(IntSize(2000, 2000));
  MockInsecureInputService mock_service(page_holder->GetFrame());
  page_holder->GetDocument().body()->SetInnerHTMLFromString(
      "<input type='password' style='display:none;'>");
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  blink::test::RunPendingTasks();
  // The message should not be sent for a hidden password field.
  EXPECT_FALSE(mock_service.PasswordFieldVisibleCalled());

  // Now make the input a visible non-password field.
  HTMLInputElement* input =
      ToHTMLInputElement(page_holder->GetDocument().body()->firstChild());
  input->setType("text");
  input->setAttribute("style", "", ASSERT_NO_EXCEPTION);
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  blink::test::RunPendingTasks();
  EXPECT_FALSE(mock_service.PasswordFieldVisibleCalled());
}

// Tests that a Mojo message is sent when the only visible password
// field becomes invisible.
TEST(PasswordInputTypeTest, VisiblePasswordFieldBecomesInvisible) {
  std::unique_ptr<DummyPageHolder> page_holder =
      DummyPageHolder::Create(IntSize(2000, 2000));
  MockInsecureInputService mock_service(page_holder->GetFrame());
  page_holder->GetDocument().body()->SetInnerHTMLFromString(
      "<input type='password'>");
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  blink::test::RunPendingTasks();
  EXPECT_TRUE(mock_service.PasswordFieldVisibleCalled());
  EXPECT_EQ(0u, mock_service.NumPasswordFieldsInvisibleCalls());

  // Now make the input invisible.
  HTMLInputElement* input =
      ToHTMLInputElement(page_holder->GetDocument().body()->firstChild());
  input->setAttribute("style", "display:none;", ASSERT_NO_EXCEPTION);
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  blink::test::RunPendingTasks();
  EXPECT_EQ(1u, mock_service.NumPasswordFieldsInvisibleCalls());
}

// Tests that a Mojo message is sent when all visible password fields
// become invisible.
TEST(PasswordInputTypeTest, AllVisiblePasswordFieldBecomeInvisible) {
  std::unique_ptr<DummyPageHolder> page_holder =
      DummyPageHolder::Create(IntSize(2000, 2000));
  MockInsecureInputService mock_service(page_holder->GetFrame());
  page_holder->GetDocument().body()->SetInnerHTMLFromString(
      "<input type='password'><input type='password'>");
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  blink::test::RunPendingTasks();
  EXPECT_EQ(0u, mock_service.NumPasswordFieldsInvisibleCalls());

  // Make the first input invisible. There should be no message because
  // there is still a visible input.
  HTMLInputElement* input =
      ToHTMLInputElement(page_holder->GetDocument().body()->firstChild());
  input->setAttribute("style", "display:none;", ASSERT_NO_EXCEPTION);
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  blink::test::RunPendingTasks();
  EXPECT_EQ(0u, mock_service.NumPasswordFieldsInvisibleCalls());

  // When all inputs are invisible, then a message should be sent.
  input = ToHTMLInputElement(page_holder->GetDocument().body()->lastChild());
  input->setAttribute("style", "display:none;", ASSERT_NO_EXCEPTION);
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  blink::test::RunPendingTasks();
  EXPECT_EQ(1u, mock_service.NumPasswordFieldsInvisibleCalls());

  // If the count of visible inputs goes positive again and then back to
  // zero, a message should be sent again.
  input->setAttribute("style", "", ASSERT_NO_EXCEPTION);
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  blink::test::RunPendingTasks();
  EXPECT_EQ(1u, mock_service.NumPasswordFieldsInvisibleCalls());
  input->setAttribute("style", "display:none;", ASSERT_NO_EXCEPTION);
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  blink::test::RunPendingTasks();
  EXPECT_EQ(2u, mock_service.NumPasswordFieldsInvisibleCalls());
}

// Tests that a Mojo message is sent when the containing element of a
// visible password field becomes invisible.
TEST(PasswordInputTypeTest, PasswordFieldContainerBecomesInvisible) {
  std::unique_ptr<DummyPageHolder> page_holder =
      DummyPageHolder::Create(IntSize(2000, 2000));
  MockInsecureInputService mock_service(page_holder->GetFrame());
  page_holder->GetDocument().body()->SetInnerHTMLFromString(
      "<div><input type='password'></div>");
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  blink::test::RunPendingTasks();
  EXPECT_EQ(0u, mock_service.NumPasswordFieldsInvisibleCalls());

  // If the containing div becomes invisible, a message should be sent.
  HTMLElement* div =
      ToHTMLDivElement(page_holder->GetDocument().body()->firstChild());
  div->setAttribute("style", "display:none;", ASSERT_NO_EXCEPTION);
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  blink::test::RunPendingTasks();
  EXPECT_EQ(1u, mock_service.NumPasswordFieldsInvisibleCalls());

  // If the containing div becomes visible and then invisible again, a message
  // should be sent.
  div->setAttribute("style", "", ASSERT_NO_EXCEPTION);
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  blink::test::RunPendingTasks();
  EXPECT_EQ(1u, mock_service.NumPasswordFieldsInvisibleCalls());
  div->setAttribute("style", "display:none;", ASSERT_NO_EXCEPTION);
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  blink::test::RunPendingTasks();
  EXPECT_EQ(2u, mock_service.NumPasswordFieldsInvisibleCalls());
}

// Tests that a Mojo message is sent when all visible password fields
// become non-password fields.
TEST(PasswordInputTypeTest, PasswordFieldsBecomeNonPasswordFields) {
  std::unique_ptr<DummyPageHolder> page_holder =
      DummyPageHolder::Create(IntSize(2000, 2000));
  MockInsecureInputService mock_service(page_holder->GetFrame());
  page_holder->GetDocument().body()->SetInnerHTMLFromString(
      "<input type='password'><input type='password'>");
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  blink::test::RunPendingTasks();
  EXPECT_EQ(0u, mock_service.NumPasswordFieldsInvisibleCalls());

  // Make the first input a non-password input. There should be no
  // message because there is still a visible password input.
  HTMLInputElement* input =
      ToHTMLInputElement(page_holder->GetDocument().body()->firstChild());
  input->setType("text");
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  blink::test::RunPendingTasks();
  EXPECT_EQ(0u, mock_service.NumPasswordFieldsInvisibleCalls());

  // When all inputs are no longer passwords, then a message should be sent.
  input = ToHTMLInputElement(page_holder->GetDocument().body()->lastChild());
  input->setType("text");
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  blink::test::RunPendingTasks();
  EXPECT_EQ(1u, mock_service.NumPasswordFieldsInvisibleCalls());
}

// Tests that only one Mojo message is sent for multiple password
// visibility events within the same task.
TEST(PasswordInputTypeTest, MultipleEventsInSameTask) {
  std::unique_ptr<DummyPageHolder> page_holder =
      DummyPageHolder::Create(IntSize(2000, 2000));
  MockInsecureInputService mock_service(page_holder->GetFrame());
  page_holder->GetDocument().body()->SetInnerHTMLFromString(
      "<input type='password'>");
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  // Make the password field invisible in the same task.
  HTMLInputElement* input =
      ToHTMLInputElement(page_holder->GetDocument().body()->firstChild());
  input->setType("text");
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  blink::test::RunPendingTasks();
  // Only a single Mojo message should have been sent, with the latest state of
  // the page (which is that no password fields are visible).
  EXPECT_EQ(1u, mock_service.NumPasswordFieldsInvisibleCalls());
  EXPECT_FALSE(mock_service.PasswordFieldVisibleCalled());
}

// Tests that a Mojo message is sent when a password field is edited
// on the page.
TEST(PasswordInputTypeTest, DidEditFieldEvent) {
  std::unique_ptr<DummyPageHolder> page_holder =
      DummyPageHolder::Create(IntSize(2000, 2000));
  MockInsecureInputService mock_service(page_holder->GetFrame());
  page_holder->GetDocument().body()->SetInnerHTMLFromString(
      "<input type='password'>");
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  blink::test::RunPendingTasks();
  EXPECT_EQ(0u, mock_service.DidEditFieldCalls());
  // Simulate a text field edit.
  page_holder->GetDocument().MaybeQueueSendDidEditFieldInInsecureContext();
  blink::test::RunPendingTasks();
  EXPECT_EQ(1u, mock_service.DidEditFieldCalls());
  // Ensure additional edits do not trigger additional notifications.
  page_holder->GetDocument().MaybeQueueSendDidEditFieldInInsecureContext();
  blink::test::RunPendingTasks();
  EXPECT_EQ(1u, mock_service.DidEditFieldCalls());
}

// Tests that a Mojo message is not sent when a password field is edited
// in a secure context.
TEST(PasswordInputTypeTest, DidEditFieldEventNotSentFromSecureContext) {
  std::unique_ptr<DummyPageHolder> page_holder =
      DummyPageHolder::Create(IntSize(2000, 2000));
  MockInsecureInputService mock_service(page_holder->GetFrame());
  page_holder->GetDocument().SetURL(KURL("https://example.test"));
  page_holder->GetDocument().SetSecurityOrigin(
      SecurityOrigin::Create(KURL("https://example.test")));
  page_holder->GetDocument().SetSecureContextStateForTesting(
      SecureContextState::kSecure);
  page_holder->GetDocument().body()->SetInnerHTMLFromString(
      "<input type='password'>");
  page_holder->GetDocument().View()->UpdateAllLifecyclePhases();
  // Simulate a text field edit.
  page_holder->GetDocument().MaybeQueueSendDidEditFieldInInsecureContext();
  // No message should have been sent from a secure context.
  blink::test::RunPendingTasks();
  EXPECT_EQ(0u, mock_service.DidEditFieldCalls());
}

}  // namespace blink
