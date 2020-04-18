// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/passwords/manage_passwords_state.h"

#include <iterator>
#include <utility>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/password_manager/core/browser/fake_form_fetcher.h"
#include "components/password_manager/core/browser/password_form_manager.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "components/password_manager/core/browser/stub_form_saver.h"
#include "components/password_manager/core/browser/stub_password_manager_client.h"
#include "components/password_manager/core/browser/stub_password_manager_driver.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

using ::testing::_;
using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::Pointee;
using ::testing::UnorderedElementsAre;

namespace {

constexpr char kTestOrigin[] = "http://example.com/";
constexpr char kTestPSLOrigin[] = "http://1.example.com/";

std::vector<const autofill::PasswordForm*> GetRawPointers(
    const std::vector<std::unique_ptr<autofill::PasswordForm>>& forms) {
  std::vector<const autofill::PasswordForm*> result;
  std::transform(forms.begin(), forms.end(), std::back_inserter(result),
                 [](const std::unique_ptr<autofill::PasswordForm>& form) {
                   return form.get();
                 });
  return result;
}

class ManagePasswordsStateTest : public testing::Test {
 public:
  ManagePasswordsStateTest() : password_manager_(&stub_client_) {
    fetcher_.Fetch();
  }

  void SetUp() override {
    test_local_form_.origin = GURL(kTestOrigin);
    test_local_form_.username_value = base::ASCIIToUTF16("username");
    test_local_form_.username_element = base::ASCIIToUTF16("username_element");
    test_local_form_.password_value = base::ASCIIToUTF16("12345");

    test_psl_form_.origin = GURL(kTestPSLOrigin);
    test_psl_form_.username_value = base::ASCIIToUTF16("username_psl");
    test_psl_form_.username_element = base::ASCIIToUTF16("username_element");
    test_psl_form_.password_value = base::ASCIIToUTF16("12345");
    test_psl_form_.is_public_suffix_match = true;

    test_submitted_form_ = test_local_form_;
    test_submitted_form_.username_value = base::ASCIIToUTF16("new one");
    test_submitted_form_.password_value = base::ASCIIToUTF16("asdfjkl;");

    test_local_federated_form_ = test_local_form_;
    test_local_federated_form_.federation_origin =
        url::Origin::Create(GURL("https://idp.com"));
    test_local_federated_form_.password_value.clear();
    test_local_federated_form_.signon_realm =
        "federation://example.com/accounts.com";

    passwords_data_.set_client(&stub_client_);
  }

  autofill::PasswordForm& test_local_form() { return test_local_form_; }
  autofill::PasswordForm& test_psl_form() { return test_psl_form_; }
  autofill::PasswordForm& test_submitted_form() { return test_submitted_form_; }
  autofill::PasswordForm& test_local_federated_form() {
    return test_local_federated_form_;
  }
  std::vector<const autofill::PasswordForm*>& test_stored_forms() {
    return test_stored_forms_;
  }
  ManagePasswordsState& passwords_data() { return passwords_data_; }

  // Returns a PasswordFormManager containing |test_stored_forms_| as the best
  // matches.
  std::unique_ptr<password_manager::PasswordFormManager> CreateFormManager();

  // Returns a PasswordFormManager containing test_local_federated_form() as a
  // stored federated credential.
  std::unique_ptr<password_manager::PasswordFormManager>
  CreateFormManagerWithFederation();

  // Pushes irrelevant updates to |passwords_data_| and checks that they don't
  // affect the state.
  void TestNoisyUpdates();

  // Pushes both relevant and irrelevant updates to |passwords_data_|.
  void TestAllUpdates();

  // Pushes a blacklisted form and checks that it doesn't affect the state.
  void TestBlacklistedUpdates();

  MOCK_METHOD1(CredentialCallback, void(const autofill::PasswordForm*));

 private:
  // Implements both CreateFormManager and CreateFormManagerWithFederation.
  std::unique_ptr<password_manager::PasswordFormManager>
  CreateFormManagerInternal(bool include_federated);

  password_manager::StubPasswordManagerClient stub_client_;
  password_manager::StubPasswordManagerDriver driver_;
  password_manager::PasswordManager password_manager_;
  password_manager::FakeFormFetcher fetcher_;

  ManagePasswordsState passwords_data_;
  autofill::PasswordForm test_local_form_;
  autofill::PasswordForm test_psl_form_;
  autofill::PasswordForm test_submitted_form_;
  autofill::PasswordForm test_local_federated_form_;
  std::vector<const autofill::PasswordForm*> test_stored_forms_;
};

std::unique_ptr<password_manager::PasswordFormManager>
ManagePasswordsStateTest::CreateFormManager() {
  return CreateFormManagerInternal(false);
}

std::unique_ptr<password_manager::PasswordFormManager>
ManagePasswordsStateTest::CreateFormManagerWithFederation() {
  return CreateFormManagerInternal(true);
}

std::unique_ptr<password_manager::PasswordFormManager>
ManagePasswordsStateTest::CreateFormManagerInternal(bool include_federated) {
  std::unique_ptr<password_manager::PasswordFormManager> test_form_manager(
      new password_manager::PasswordFormManager(
          &password_manager_, &stub_client_, driver_.AsWeakPtr(),
          test_local_form(),
          base::WrapUnique(new password_manager::StubFormSaver), &fetcher_));
  test_form_manager->Init(nullptr);
  fetcher_.SetNonFederated(test_stored_forms_, 0u);
  if (include_federated) {
    fetcher_.set_federated({&test_local_federated_form()});
  }
  EXPECT_EQ(include_federated ? 1u : 0u,
            test_form_manager->GetFormFetcher()->GetFederatedMatches().size());
  if (include_federated) {
    EXPECT_EQ(
        test_local_federated_form(),
        *test_form_manager->GetFormFetcher()->GetFederatedMatches().front());
  }
  return test_form_manager;
}

void ManagePasswordsStateTest::TestNoisyUpdates() {
  const std::vector<const autofill::PasswordForm*> forms =
      GetRawPointers(passwords_data_.GetCurrentForms());
  const password_manager::ui::State state = passwords_data_.state();
  const GURL origin = passwords_data_.origin();

  // Push "Add".
  autofill::PasswordForm form;
  form.origin = GURL("http://3rdparty.com");
  form.username_value = base::ASCIIToUTF16("username");
  form.password_value = base::ASCIIToUTF16("12345");
  password_manager::PasswordStoreChange change(
      password_manager::PasswordStoreChange::ADD, form);
  password_manager::PasswordStoreChangeList list(1, change);
  passwords_data().ProcessLoginsChanged(list);
  EXPECT_EQ(forms, GetRawPointers(passwords_data().GetCurrentForms()));
  EXPECT_EQ(state, passwords_data().state());
  EXPECT_EQ(origin, passwords_data().origin());

  // Update the form.
  form.password_value = base::ASCIIToUTF16("password");
  list[0] = password_manager::PasswordStoreChange(
      password_manager::PasswordStoreChange::UPDATE, form);
  passwords_data().ProcessLoginsChanged(list);
  EXPECT_EQ(forms, GetRawPointers(passwords_data().GetCurrentForms()));
  EXPECT_EQ(state, passwords_data().state());
  EXPECT_EQ(origin, passwords_data().origin());

  // Delete the form.
  list[0] = password_manager::PasswordStoreChange(
      password_manager::PasswordStoreChange::REMOVE, form);
  passwords_data().ProcessLoginsChanged(list);
  EXPECT_EQ(forms, GetRawPointers(passwords_data().GetCurrentForms()));
  EXPECT_EQ(state, passwords_data().state());
  EXPECT_EQ(origin, passwords_data().origin());
}

void ManagePasswordsStateTest::TestAllUpdates() {
  const std::vector<const autofill::PasswordForm*> forms =
      GetRawPointers(passwords_data_.GetCurrentForms());
  const password_manager::ui::State state = passwords_data_.state();
  const GURL origin = passwords_data_.origin();
  EXPECT_NE(GURL::EmptyGURL(), origin);

  // Push "Add".
  autofill::PasswordForm form;
  GURL::Replacements replace_path;
  replace_path.SetPathStr("absolutely_different_path");
  form.origin = origin.ReplaceComponents(replace_path);
  form.signon_realm = form.origin.GetOrigin().spec();
  form.username_value = base::ASCIIToUTF16("user15");
  form.password_value = base::ASCIIToUTF16("12345");
  password_manager::PasswordStoreChange change(
      password_manager::PasswordStoreChange::ADD, form);
  password_manager::PasswordStoreChangeList list(1, change);
  passwords_data().ProcessLoginsChanged(list);
  EXPECT_THAT(passwords_data().GetCurrentForms(), Contains(Pointee(form)));
  EXPECT_EQ(state, passwords_data().state());
  EXPECT_EQ(origin, passwords_data().origin());

  // Update the form.
  form.password_value = base::ASCIIToUTF16("password");
  list[0] = password_manager::PasswordStoreChange(
      password_manager::PasswordStoreChange::UPDATE, form);
  passwords_data().ProcessLoginsChanged(list);
  EXPECT_THAT(passwords_data().GetCurrentForms(), Contains(Pointee(form)));
  EXPECT_EQ(state, passwords_data().state());
  EXPECT_EQ(origin, passwords_data().origin());

  // Delete the form.
  list[0] = password_manager::PasswordStoreChange(
      password_manager::PasswordStoreChange::REMOVE, form);
  passwords_data().ProcessLoginsChanged(list);
  EXPECT_EQ(forms, GetRawPointers(passwords_data().GetCurrentForms()));
  EXPECT_EQ(state, passwords_data().state());
  EXPECT_EQ(origin, passwords_data().origin());

  TestNoisyUpdates();
}

void ManagePasswordsStateTest::TestBlacklistedUpdates() {
  const std::vector<const autofill::PasswordForm*> forms =
      GetRawPointers(passwords_data_.GetCurrentForms());
  const password_manager::ui::State state = passwords_data_.state();
  const GURL origin = passwords_data_.origin();
  EXPECT_NE(GURL::EmptyGURL(), origin);

  // Process the blacklisted form.
  autofill::PasswordForm blacklisted;
  blacklisted.blacklisted_by_user = true;
  blacklisted.origin = origin;
  password_manager::PasswordStoreChangeList list;
  list.push_back(password_manager::PasswordStoreChange(
      password_manager::PasswordStoreChange::ADD, blacklisted));
  passwords_data().ProcessLoginsChanged(list);
  EXPECT_EQ(forms, GetRawPointers(passwords_data().GetCurrentForms()));
  EXPECT_EQ(state, passwords_data().state());
  EXPECT_EQ(origin, passwords_data().origin());

  // Delete the blacklisted form.
  list[0] = password_manager::PasswordStoreChange(
      password_manager::PasswordStoreChange::REMOVE, blacklisted);
  passwords_data().ProcessLoginsChanged(list);
  EXPECT_EQ(forms, GetRawPointers(passwords_data().GetCurrentForms()));
  EXPECT_EQ(state, passwords_data().state());
  EXPECT_EQ(origin, passwords_data().origin());
}

TEST_F(ManagePasswordsStateTest, DefaultState) {
  EXPECT_THAT(passwords_data().GetCurrentForms(), IsEmpty());
  EXPECT_EQ(password_manager::ui::INACTIVE_STATE, passwords_data().state());
  EXPECT_EQ(GURL::EmptyGURL(), passwords_data().origin());
  EXPECT_FALSE(passwords_data().form_manager());

  TestNoisyUpdates();
}

TEST_F(ManagePasswordsStateTest, PasswordSubmitted) {
  test_stored_forms().push_back(&test_local_form());
  test_stored_forms().push_back(&test_psl_form());
  std::unique_ptr<password_manager::PasswordFormManager> test_form_manager(
      CreateFormManager());
  test_form_manager->ProvisionallySave(test_submitted_form());
  passwords_data().OnPendingPassword(std::move(test_form_manager));

  EXPECT_THAT(passwords_data().GetCurrentForms(),
              ElementsAre(Pointee(test_local_form())));
  EXPECT_EQ(password_manager::ui::PENDING_PASSWORD_STATE,
            passwords_data().state());
  EXPECT_EQ(test_submitted_form().origin, passwords_data().origin());
  ASSERT_TRUE(passwords_data().form_manager());
  EXPECT_EQ(test_submitted_form(),
            passwords_data().form_manager()->GetPendingCredentials());
  TestAllUpdates();
}

TEST_F(ManagePasswordsStateTest, PasswordSaved) {
  test_stored_forms().push_back(&test_local_form());
  std::unique_ptr<password_manager::PasswordFormManager> test_form_manager(
      CreateFormManager());
  test_form_manager->ProvisionallySave(test_submitted_form());
  passwords_data().OnPendingPassword(std::move(test_form_manager));
  EXPECT_EQ(password_manager::ui::PENDING_PASSWORD_STATE,
            passwords_data().state());

  passwords_data().TransitionToState(password_manager::ui::MANAGE_STATE);
  EXPECT_THAT(passwords_data().GetCurrentForms(),
              ElementsAre(Pointee(test_local_form())));
  EXPECT_EQ(password_manager::ui::MANAGE_STATE,
            passwords_data().state());
  EXPECT_EQ(test_submitted_form().origin, passwords_data().origin());
  TestAllUpdates();
}

TEST_F(ManagePasswordsStateTest, PasswordSubmittedFederationsPresent) {
  std::unique_ptr<password_manager::PasswordFormManager> test_form_manager(
      CreateFormManagerWithFederation());
  test_form_manager->ProvisionallySave(test_submitted_form());
  passwords_data().OnPendingPassword(std::move(test_form_manager));

  EXPECT_THAT(passwords_data().GetCurrentForms(),
              ElementsAre(Pointee(test_local_federated_form())));
}

TEST_F(ManagePasswordsStateTest, OnRequestCredentials) {
  std::vector<std::unique_ptr<autofill::PasswordForm>> local_credentials;
  local_credentials.emplace_back(new autofill::PasswordForm(test_local_form()));
  const GURL origin = test_local_form().origin;
  passwords_data().OnRequestCredentials(std::move(local_credentials), origin);
  passwords_data().set_credentials_callback(base::Bind(
      &ManagePasswordsStateTest::CredentialCallback, base::Unretained(this)));
  EXPECT_THAT(passwords_data().GetCurrentForms(),
              ElementsAre(Pointee(test_local_form())));
  EXPECT_EQ(password_manager::ui::CREDENTIAL_REQUEST_STATE,
            passwords_data().state());
  EXPECT_EQ(origin, passwords_data().origin());
  TestAllUpdates();

  EXPECT_CALL(*this, CredentialCallback(nullptr));
  passwords_data().TransitionToState(password_manager::ui::MANAGE_STATE);
  EXPECT_TRUE(passwords_data().credentials_callback().is_null());
  EXPECT_THAT(passwords_data().GetCurrentForms(),
              ElementsAre(Pointee(test_local_form())));
  EXPECT_EQ(password_manager::ui::MANAGE_STATE, passwords_data().state());
  EXPECT_EQ(origin, passwords_data().origin());
  TestAllUpdates();
}

TEST_F(ManagePasswordsStateTest, AutoSignin) {
  std::vector<std::unique_ptr<autofill::PasswordForm>> local_credentials;
  local_credentials.emplace_back(new autofill::PasswordForm(test_local_form()));
  passwords_data().OnAutoSignin(std::move(local_credentials),
                                test_local_form().origin);
  EXPECT_THAT(passwords_data().GetCurrentForms(),
              ElementsAre(Pointee(test_local_form())));
  EXPECT_EQ(password_manager::ui::AUTO_SIGNIN_STATE, passwords_data().state());
  EXPECT_EQ(test_local_form().origin, passwords_data().origin());
  TestAllUpdates();

  passwords_data().TransitionToState(password_manager::ui::MANAGE_STATE);
  EXPECT_THAT(passwords_data().GetCurrentForms(),
              ElementsAre(Pointee(test_local_form())));
  EXPECT_EQ(password_manager::ui::MANAGE_STATE, passwords_data().state());
  EXPECT_EQ(test_local_form().origin, passwords_data().origin());
  TestAllUpdates();
}

TEST_F(ManagePasswordsStateTest, AutomaticPasswordSave) {
  test_stored_forms().push_back(&test_psl_form());
  std::unique_ptr<password_manager::PasswordFormManager> test_form_manager(
      CreateFormManager());
  test_form_manager->ProvisionallySave(test_submitted_form());

  passwords_data().OnAutomaticPasswordSave(std::move(test_form_manager));
  EXPECT_EQ(password_manager::ui::CONFIRMATION_STATE, passwords_data().state());
  EXPECT_EQ(test_submitted_form().origin, passwords_data().origin());
  ASSERT_TRUE(passwords_data().form_manager());
  EXPECT_EQ(test_submitted_form(),
            passwords_data().form_manager()->GetPendingCredentials());
  TestAllUpdates();

  passwords_data().TransitionToState(password_manager::ui::MANAGE_STATE);
  EXPECT_THAT(passwords_data().GetCurrentForms(),
              ElementsAre(Pointee(test_submitted_form())));
  EXPECT_EQ(password_manager::ui::MANAGE_STATE, passwords_data().state());
  EXPECT_EQ(test_submitted_form().origin, passwords_data().origin());
  TestAllUpdates();
}

TEST_F(ManagePasswordsStateTest, AutomaticPasswordSaveWithFederations) {
  std::unique_ptr<password_manager::PasswordFormManager> test_form_manager(
      CreateFormManagerWithFederation());
  test_form_manager->ProvisionallySave(test_submitted_form());

  passwords_data().OnAutomaticPasswordSave(std::move(test_form_manager));
  EXPECT_THAT(passwords_data().GetCurrentForms(),
              UnorderedElementsAre(Pointee(test_submitted_form()),
                                   Pointee(test_local_federated_form())));
}

TEST_F(ManagePasswordsStateTest, PasswordAutofilled) {
  std::map<base::string16, const autofill::PasswordForm*> password_form_map;
  password_form_map.insert(
      std::make_pair(test_local_form().username_value, &test_local_form()));
  const GURL origin(kTestOrigin);
  passwords_data().OnPasswordAutofilled(password_form_map, origin, nullptr);

  EXPECT_THAT(passwords_data().GetCurrentForms(),
              ElementsAre(Pointee(test_local_form())));
  EXPECT_EQ(password_manager::ui::MANAGE_STATE, passwords_data().state());
  EXPECT_EQ(origin, passwords_data().origin());

  TestAllUpdates();
}

TEST_F(ManagePasswordsStateTest, PasswordAutofillWithSavedFederations) {
  std::map<base::string16, const autofill::PasswordForm*> password_form_map;
  password_form_map.insert(
      std::make_pair(test_local_form().username_value, &test_local_form()));
  const GURL origin(kTestOrigin);
  std::vector<const autofill::PasswordForm*> federated;
  federated.push_back(&test_local_federated_form());
  passwords_data().OnPasswordAutofilled(password_form_map, origin, &federated);

  // |federated| represents the locally saved federations. These are bundled in
  // the "current forms".
  EXPECT_THAT(passwords_data().GetCurrentForms(),
              UnorderedElementsAre(Pointee(test_local_form()),
                                   Pointee(test_local_federated_form())));
  // |federated_credentials_forms()| do not refer to the saved federations.
  EXPECT_EQ(password_manager::ui::MANAGE_STATE, passwords_data().state());
}

TEST_F(ManagePasswordsStateTest, ActiveOnMixedPSLAndNonPSLMatched) {
  std::map<base::string16, const autofill::PasswordForm*> password_form_map;
  password_form_map[test_local_form().username_value] = &test_local_form();
  password_form_map[test_psl_form().username_value] = &test_psl_form();
  const GURL origin(kTestOrigin);
  passwords_data().OnPasswordAutofilled(password_form_map, origin, nullptr);

  EXPECT_THAT(passwords_data().GetCurrentForms(),
              ElementsAre(Pointee(test_local_form())));
  EXPECT_EQ(password_manager::ui::MANAGE_STATE, passwords_data().state());
  EXPECT_EQ(origin, passwords_data().origin());

  TestAllUpdates();
}

TEST_F(ManagePasswordsStateTest, InactiveOnPSLMatched) {
  std::map<base::string16, const autofill::PasswordForm*> password_form_map;
  password_form_map[test_psl_form().username_value] = &test_psl_form();
  passwords_data().OnPasswordAutofilled(password_form_map, GURL(kTestOrigin),
                                        nullptr);

  EXPECT_THAT(passwords_data().GetCurrentForms(), IsEmpty());
  EXPECT_EQ(password_manager::ui::INACTIVE_STATE, passwords_data().state());
  EXPECT_EQ(GURL::EmptyGURL(), passwords_data().origin());
  EXPECT_FALSE(passwords_data().form_manager());
}

TEST_F(ManagePasswordsStateTest, OnInactive) {
  std::unique_ptr<password_manager::PasswordFormManager> test_form_manager(
      CreateFormManager());
  test_form_manager->ProvisionallySave(test_submitted_form());

  passwords_data().OnPendingPassword(std::move(test_form_manager));
  EXPECT_EQ(password_manager::ui::PENDING_PASSWORD_STATE,
            passwords_data().state());
  passwords_data().OnInactive();
  EXPECT_THAT(passwords_data().GetCurrentForms(), IsEmpty());
  EXPECT_EQ(password_manager::ui::INACTIVE_STATE, passwords_data().state());
  EXPECT_EQ(GURL::EmptyGURL(), passwords_data().origin());
  EXPECT_FALSE(passwords_data().form_manager());
  TestNoisyUpdates();
}

TEST_F(ManagePasswordsStateTest, PendingPasswordAddBlacklisted) {
  std::unique_ptr<password_manager::PasswordFormManager> test_form_manager(
      CreateFormManager());
  test_form_manager->ProvisionallySave(test_submitted_form());
  passwords_data().OnPendingPassword(std::move(test_form_manager));
  EXPECT_EQ(password_manager::ui::PENDING_PASSWORD_STATE,
            passwords_data().state());

  TestBlacklistedUpdates();
}

TEST_F(ManagePasswordsStateTest, RequestCredentialsAddBlacklisted) {
  std::vector<std::unique_ptr<autofill::PasswordForm>> local_credentials;
  local_credentials.emplace_back(new autofill::PasswordForm(test_local_form()));
  const GURL origin = test_local_form().origin;
  passwords_data().OnRequestCredentials(std::move(local_credentials), origin);
  passwords_data().set_credentials_callback(base::Bind(
      &ManagePasswordsStateTest::CredentialCallback, base::Unretained(this)));
  EXPECT_EQ(password_manager::ui::CREDENTIAL_REQUEST_STATE,
            passwords_data().state());

  TestBlacklistedUpdates();
}

TEST_F(ManagePasswordsStateTest, AutoSigninAddBlacklisted) {
  std::vector<std::unique_ptr<autofill::PasswordForm>> local_credentials;
  local_credentials.emplace_back(new autofill::PasswordForm(test_local_form()));
  passwords_data().OnAutoSignin(std::move(local_credentials),
                                test_local_form().origin);
  EXPECT_EQ(password_manager::ui::AUTO_SIGNIN_STATE, passwords_data().state());

  TestBlacklistedUpdates();
}

TEST_F(ManagePasswordsStateTest, AutomaticPasswordSaveAddBlacklisted) {
  std::unique_ptr<password_manager::PasswordFormManager> test_form_manager(
      CreateFormManager());
  test_form_manager->ProvisionallySave(test_submitted_form());
  passwords_data().OnAutomaticPasswordSave(std::move(test_form_manager));
  EXPECT_EQ(password_manager::ui::CONFIRMATION_STATE, passwords_data().state());

  TestBlacklistedUpdates();
}

TEST_F(ManagePasswordsStateTest, BackgroundAutofilledAddBlacklisted) {
  std::map<base::string16, const autofill::PasswordForm*> password_form_map;
  password_form_map.insert(
      std::make_pair(test_local_form().username_value, &test_local_form()));
  passwords_data().OnPasswordAutofilled(
      password_form_map, password_form_map.begin()->second->origin, nullptr);
  EXPECT_EQ(password_manager::ui::MANAGE_STATE, passwords_data().state());

  TestBlacklistedUpdates();
}

TEST_F(ManagePasswordsStateTest, PasswordUpdateAddBlacklisted) {
  std::unique_ptr<password_manager::PasswordFormManager> test_form_manager(
      CreateFormManager());
  test_form_manager->ProvisionallySave(test_submitted_form());
  passwords_data().OnUpdatePassword(std::move(test_form_manager));
  EXPECT_EQ(password_manager::ui::PENDING_PASSWORD_UPDATE_STATE,
            passwords_data().state());

  TestBlacklistedUpdates();
}

TEST_F(ManagePasswordsStateTest, PasswordUpdateSubmitted) {
  test_stored_forms().push_back(&test_local_form());
  test_stored_forms().push_back(&test_psl_form());
  std::unique_ptr<password_manager::PasswordFormManager> test_form_manager(
      CreateFormManager());
  test_form_manager->ProvisionallySave(test_submitted_form());
  passwords_data().OnUpdatePassword(std::move(test_form_manager));

  EXPECT_THAT(passwords_data().GetCurrentForms(),
              ElementsAre(Pointee(test_local_form())));
  EXPECT_EQ(password_manager::ui::PENDING_PASSWORD_UPDATE_STATE,
            passwords_data().state());
  EXPECT_EQ(test_submitted_form().origin, passwords_data().origin());
  ASSERT_TRUE(passwords_data().form_manager());
  EXPECT_EQ(test_submitted_form(),
            passwords_data().form_manager()->GetPendingCredentials());
  TestAllUpdates();
}

TEST_F(ManagePasswordsStateTest, AndroidPasswordUpdateSubmitted) {
  autofill::PasswordForm android_form;
  android_form.signon_realm = "android://dHJhc2g=@com.example.android/";
  android_form.origin = GURL(android_form.signon_realm);
  android_form.username_value = test_submitted_form().username_value;
  android_form.password_value = base::ASCIIToUTF16("old pass");
  test_stored_forms().push_back(&android_form);
  std::unique_ptr<password_manager::PasswordFormManager> test_form_manager(
      CreateFormManager());
  test_form_manager->ProvisionallySave(test_submitted_form());
  passwords_data().OnUpdatePassword(std::move(test_form_manager));

  EXPECT_THAT(passwords_data().GetCurrentForms(),
              ElementsAre(Pointee(android_form)));
  EXPECT_EQ(password_manager::ui::PENDING_PASSWORD_UPDATE_STATE,
            passwords_data().state());
  EXPECT_EQ(test_submitted_form().origin, passwords_data().origin());
  ASSERT_TRUE(passwords_data().form_manager());
  android_form.password_value = test_submitted_form().password_value;
  EXPECT_EQ(android_form,
            passwords_data().form_manager()->GetPendingCredentials());
  TestAllUpdates();
}

TEST_F(ManagePasswordsStateTest, PasswordUpdateSubmittedWithFederations) {
  test_stored_forms().push_back(&test_local_form());
  std::unique_ptr<password_manager::PasswordFormManager> test_form_manager(
      CreateFormManagerWithFederation());
  test_form_manager->ProvisionallySave(test_submitted_form());
  passwords_data().OnUpdatePassword(std::move(test_form_manager));

  EXPECT_THAT(passwords_data().GetCurrentForms(),
              UnorderedElementsAre(Pointee(test_local_form()),
                                   Pointee(test_local_federated_form())));
}

TEST_F(ManagePasswordsStateTest, ChooseCredentialLocal) {
  passwords_data().OnRequestCredentials(
      std::vector<std::unique_ptr<autofill::PasswordForm>>(),
      test_local_form().origin);
  passwords_data().set_credentials_callback(base::Bind(
      &ManagePasswordsStateTest::CredentialCallback, base::Unretained(this)));
  EXPECT_CALL(*this, CredentialCallback(&test_local_form()));
  passwords_data().ChooseCredential(&test_local_form());
}

TEST_F(ManagePasswordsStateTest, ChooseCredentialEmpty) {
  passwords_data().OnRequestCredentials(
      std::vector<std::unique_ptr<autofill::PasswordForm>>(),
      test_local_form().origin);
  passwords_data().set_credentials_callback(base::Bind(
      &ManagePasswordsStateTest::CredentialCallback, base::Unretained(this)));
  EXPECT_CALL(*this, CredentialCallback(nullptr));
  passwords_data().ChooseCredential(nullptr);
}

TEST_F(ManagePasswordsStateTest, ChooseCredentialLocalWithNonEmptyFederation) {
  passwords_data().OnRequestCredentials(
      std::vector<std::unique_ptr<autofill::PasswordForm>>(),
      test_local_form().origin);
  passwords_data().set_credentials_callback(base::Bind(
      &ManagePasswordsStateTest::CredentialCallback, base::Unretained(this)));
  EXPECT_CALL(*this, CredentialCallback(&test_local_federated_form()));
  passwords_data().ChooseCredential(&test_local_federated_form());
}

}  // namespace
