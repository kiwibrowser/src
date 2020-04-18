// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/new_password_form_manager.h"

#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/password_form.h"
#include "components/autofill/core/common/password_form_fill_data.h"
#include "components/password_manager/core/browser/fake_form_fetcher.h"
#include "components/password_manager/core/browser/stub_password_manager_client.h"
#include "components/password_manager/core/browser/stub_password_manager_driver.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using autofill::FormData;
using autofill::FormFieldData;
using autofill::PasswordForm;
using autofill::PasswordFormFillData;
using base::ASCIIToUTF16;
using testing::_;
using testing::SaveArg;

namespace password_manager {

namespace {
class MockPasswordManagerDriver : public StubPasswordManagerDriver {
 public:
  MockPasswordManagerDriver() {}

  ~MockPasswordManagerDriver() override {}

  MOCK_METHOD1(FillPasswordForm, void(const PasswordFormFillData&));
  MOCK_METHOD1(AllowPasswordGenerationForForm, void(const PasswordForm&));
};

}  // namespace

class NewPasswordFormManagerTest : public testing::Test {
 public:
  NewPasswordFormManagerTest() {
    GURL origin = GURL("http://accounts.google.com/a/ServiceLoginAuth");
    GURL action = GURL("http://accounts.google.com/a/ServiceLogin");

    observed_form_.origin = origin;
    observed_form_.action = action;
    observed_form_.name = ASCIIToUTF16("sign-in");
    observed_form_.unique_renderer_id = 1;
    observed_form_.is_form_tag = true;

    FormFieldData field;
    field.name = ASCIIToUTF16("firstname");
    field.form_control_type = "text";
    observed_form_.fields.push_back(field);

    field.name = ASCIIToUTF16("username");
    field.form_control_type = "text";
    observed_form_.fields.push_back(field);

    field.name = ASCIIToUTF16("password");
    field.form_control_type = "password";
    observed_form_.fields.push_back(field);

    saved_match_.origin = origin;
    saved_match_.action = action;
    saved_match_.preferred = true;
    saved_match_.username_value = ASCIIToUTF16("test@gmail.com");
    saved_match_.password_value = ASCIIToUTF16("test1");
  }

 protected:
  FormData observed_form_;
  PasswordForm saved_match_;
  StubPasswordManagerClient client_;
  MockPasswordManagerDriver driver_;
};

TEST_F(NewPasswordFormManagerTest, DoesManage) {
  FakeFormFetcher fetcher;
  fetcher.Fetch();
  NewPasswordFormManager form_manager(&client_, driver_.AsWeakPtr(),
                                      observed_form_, &fetcher);
  EXPECT_TRUE(form_manager.DoesManage(observed_form_, &driver_));
  // Forms on other drivers are not considered managed.
  EXPECT_FALSE(form_manager.DoesManage(observed_form_, nullptr));
  FormData another_form = observed_form_;
  another_form.is_form_tag = false;
  EXPECT_FALSE(form_manager.DoesManage(another_form, &driver_));

  another_form = observed_form_;
  another_form.unique_renderer_id = observed_form_.unique_renderer_id + 1;
  EXPECT_FALSE(form_manager.DoesManage(another_form, &driver_));
}

TEST_F(NewPasswordFormManagerTest, DoesManageNoFormTag) {
  observed_form_.is_form_tag = false;
  FakeFormFetcher fetcher;
  fetcher.Fetch();
  NewPasswordFormManager form_manager(&client_, driver_.AsWeakPtr(),
                                      observed_form_, &fetcher);
  FormData another_form = observed_form_;
  // Simulate that new input was added by JavaScript.
  another_form.fields.push_back(FormFieldData());
  EXPECT_TRUE(form_manager.DoesManage(another_form, &driver_));
  // Forms on other drivers are not considered managed.
  EXPECT_FALSE(form_manager.DoesManage(another_form, nullptr));
}

TEST_F(NewPasswordFormManagerTest, Autofill) {
  FakeFormFetcher fetcher;
  fetcher.Fetch();
  EXPECT_CALL(driver_, AllowPasswordGenerationForForm(_));
  PasswordFormFillData fill_data;
  EXPECT_CALL(driver_, FillPasswordForm(_)).WillOnce(SaveArg<0>(&fill_data));
  NewPasswordFormManager form_manager(&client_, driver_.AsWeakPtr(),
                                      observed_form_, &fetcher);
  fetcher.SetNonFederated({&saved_match_}, 0u);

  EXPECT_EQ(observed_form_.origin, fill_data.origin);
  EXPECT_FALSE(fill_data.wait_for_username);
  EXPECT_EQ(observed_form_.fields[1].name, fill_data.username_field.name);
  EXPECT_EQ(saved_match_.username_value, fill_data.username_field.value);
  EXPECT_EQ(observed_form_.fields[2].name, fill_data.password_field.name);
  EXPECT_EQ(saved_match_.password_value, fill_data.password_field.value);
}

TEST_F(NewPasswordFormManagerTest, SetSubmitted) {
  FakeFormFetcher fetcher;
  fetcher.Fetch();
  NewPasswordFormManager form_manager(&client_, driver_.AsWeakPtr(),
                                      observed_form_, &fetcher);
  EXPECT_FALSE(form_manager.is_submitted());
  EXPECT_TRUE(
      form_manager.SetSubmittedFormIfIsManaged(observed_form_, &driver_));
  EXPECT_TRUE(form_manager.is_submitted());

  FormData another_form = observed_form_;
  another_form.name += ASCIIToUTF16("1");
  // |another_form| is managed because the same |unique_renderer_id| as
  // |observed_form_|.
  EXPECT_TRUE(form_manager.SetSubmittedFormIfIsManaged(another_form, &driver_));
  EXPECT_TRUE(form_manager.is_submitted());

  form_manager.set_not_submitted();
  EXPECT_FALSE(form_manager.is_submitted());

  another_form.unique_renderer_id = observed_form_.unique_renderer_id + 1;
  EXPECT_FALSE(
      form_manager.SetSubmittedFormIfIsManaged(another_form, &driver_));
  EXPECT_FALSE(form_manager.is_submitted());

  // An identical form but in a different frame (represented here by a null
  // driver) is also not considered managed.
  EXPECT_FALSE(
      form_manager.SetSubmittedFormIfIsManaged(observed_form_, nullptr));
  EXPECT_FALSE(form_manager.is_submitted());
}

}  // namespace  password_manager
