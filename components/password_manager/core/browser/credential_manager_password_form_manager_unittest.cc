// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/credential_manager_password_form_manager.h"

#include <memory>

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/core/browser/fake_form_fetcher.h"
#include "components/password_manager/core/browser/stub_form_saver.h"
#include "components/password_manager/core/browser/stub_password_manager_client.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using autofill::PasswordForm;
using ::testing::Invoke;

namespace password_manager {

namespace {

class MockDelegate : public CredentialManagerPasswordFormManagerDelegate {
 public:
  MOCK_METHOD0(OnProvisionalSaveComplete, void());
};

}  // namespace

class CredentialManagerPasswordFormManagerTest : public testing::Test {
 public:
  CredentialManagerPasswordFormManagerTest() = default;

 protected:
  // Necessary for callbacks, and for TestAutofillDriver.
  base::MessageLoop message_loop_;

  StubPasswordManagerClient client_;

  DISALLOW_COPY_AND_ASSIGN(CredentialManagerPasswordFormManagerTest);
};

// Test that aborting early does not cause use after free.
TEST_F(CredentialManagerPasswordFormManagerTest, AbortEarly) {
  PasswordForm observed_form;
  MockDelegate delegate;
  auto form_manager = std::make_unique<CredentialManagerPasswordFormManager>(
      &client_, observed_form, std::make_unique<PasswordForm>(observed_form),
      &delegate, std::make_unique<StubFormSaver>(),
      std::make_unique<FakeFormFetcher>());
  form_manager->Init(nullptr);

  auto deleter = [&form_manager]() { form_manager.reset(); };

  // Simulate that the PasswordStore responded to the FormFetcher. As a result,
  // |form_manager| should call the delegate's OnProvisionalSaveComplete, which
  // in turn should delete |form_fetcher|.
  EXPECT_CALL(delegate, OnProvisionalSaveComplete()).WillOnce(Invoke(deleter));
  static_cast<FakeFormFetcher*>(form_manager->GetFormFetcher())
      ->SetNonFederated(std::vector<const PasswordForm*>(), 0u);
  // Check that |form_manager| was not deleted yet; doing so would have caused
  // use after free during SetNonFederated.
  EXPECT_TRUE(form_manager);

  base::RunLoop().RunUntilIdle();

  // Ultimately, |form_fetcher| should have been deleted. It just should happen
  // after it finishes executing.
  EXPECT_FALSE(form_manager);
}

// Ensure that GetCredentialSource is actually overriden and returns the proper
// value.
TEST_F(CredentialManagerPasswordFormManagerTest, GetCredentialSource) {
  PasswordForm observed_form;
  MockDelegate delegate;
  auto form_manager = std::make_unique<CredentialManagerPasswordFormManager>(
      &client_, observed_form, std::make_unique<PasswordForm>(observed_form),
      &delegate, std::make_unique<StubFormSaver>(),
      std::make_unique<FakeFormFetcher>());
  form_manager->Init(nullptr);
  ASSERT_EQ(metrics_util::CredentialSourceType::kCredentialManagementAPI,
            form_manager->GetCredentialSource());
}

}  // namespace password_manager
