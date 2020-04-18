// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/password_generation_manager.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/message_loop/message_loop.h"
#include "base/metrics/field_trial.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_field.h"
#include "components/autofill/core/browser/autofill_metrics.h"
#include "components/autofill/core/browser/form_structure.h"
#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/form_field_data.h"
#include "components/autofill/core/common/password_form_generation_data.h"
#include "components/autofill/core/common/signatures_util.h"
#include "components/password_manager/core/browser/password_autofill_manager.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "components/password_manager/core/browser/stub_password_manager_client.h"
#include "components/password_manager/core/browser/stub_password_manager_driver.h"
#include "components/password_manager/core/browser/test_password_store.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "components/variations/entropy_provider.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using autofill::FormStructure;
using base::ASCIIToUTF16;
using testing::_;

namespace password_manager {

namespace {

class TestPasswordManagerDriver : public StubPasswordManagerDriver {
 public:
  explicit TestPasswordManagerDriver(PasswordManagerClient* client)
      : password_manager_(client),
        password_generation_manager_(client, this),
        password_autofill_manager_(this, nullptr, client) {}
  ~TestPasswordManagerDriver() override {}

  // PasswordManagerDriver implementation.
  PasswordGenerationManager* GetPasswordGenerationManager() override {
    return &password_generation_manager_;
  }
  PasswordManager* GetPasswordManager() override { return &password_manager_; }
  PasswordAutofillManager* GetPasswordAutofillManager() override {
    return &password_autofill_manager_;
  }
  void FormsEligibleForGenerationFound(
      const std::vector<autofill::PasswordFormGenerationData>& forms) override {
    found_forms_eligible_for_generation_.insert(
        found_forms_eligible_for_generation_.begin(), forms.begin(),
        forms.end());
  }

  const std::vector<autofill::PasswordFormGenerationData>&
  GetFoundEligibleForGenerationForms() {
    return found_forms_eligible_for_generation_;
  }

  MOCK_METHOD0(AllowToRunFormClassifier, void());
  MOCK_METHOD0(MatchingBlacklistedFormFound, void());

 private:
  PasswordManager password_manager_;
  PasswordGenerationManager password_generation_manager_;
  PasswordAutofillManager password_autofill_manager_;
  std::vector<autofill::PasswordFormGenerationData>
      found_forms_eligible_for_generation_;
};

class MockPasswordManagerClient : public StubPasswordManagerClient {
 public:
  MOCK_CONST_METHOD0(GetPasswordSyncState, PasswordSyncState());
  MOCK_CONST_METHOD0(IsSavingAndFillingEnabledForCurrentPage, bool());
  MOCK_CONST_METHOD0(IsIncognito, bool());

  explicit MockPasswordManagerClient(std::unique_ptr<PrefService> prefs)
      : prefs_(std::move(prefs)),
        store_(new TestPasswordStore),
        driver_(this) {}

  ~MockPasswordManagerClient() override { store_->ShutdownOnUIThread(); }

  PasswordStore* GetPasswordStore() const override { return store_.get(); }
  PrefService* GetPrefs() const override { return prefs_.get(); }

  TestPasswordManagerDriver* test_driver() { return &driver_; }

 private:
  std::unique_ptr<PrefService> prefs_;
  scoped_refptr<TestPasswordStore> store_;
  TestPasswordManagerDriver driver_;
};

}  // anonymous namespace

class PasswordGenerationManagerTest : public testing::Test {
 protected:
  void SetUp() override {
    // Construct a PrefService and register all necessary prefs before handing
    // it off to |client_|, as the initialization flow of |client_| will
    // indirectly cause those prefs to be immediately accessed.
    std::unique_ptr<TestingPrefServiceSimple> prefs(
        new TestingPrefServiceSimple());
    prefs->registry()->RegisterBooleanPref(prefs::kCredentialsEnableService,
                                           true);
    client_.reset(new MockPasswordManagerClient(std::move(prefs)));
  }

  void TearDown() override { client_.reset(); }

  PasswordGenerationManager* GetGenerationManager() {
    return client_->test_driver()->GetPasswordGenerationManager();
  }

  TestPasswordManagerDriver* GetTestDriver() { return client_->test_driver(); }

  bool IsGenerationEnabled() {
    return GetGenerationManager()->IsGenerationEnabled();
  }

  void DetectFormsEligibleForGeneration(
      const std::vector<autofill::FormStructure*>& forms) {
    GetGenerationManager()->DetectFormsEligibleForGeneration(forms);
  }

  base::MessageLoop message_loop_;
  std::unique_ptr<MockPasswordManagerClient> client_;
};

TEST_F(PasswordGenerationManagerTest, IsGenerationEnabled) {
  // Enabling the PasswordManager and password sync should cause generation to
  // be enabled, unless the sync is with a custom passphrase.
  EXPECT_CALL(*client_, IsSavingAndFillingEnabledForCurrentPage())
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(*client_, GetPasswordSyncState())
      .WillRepeatedly(testing::Return(SYNCING_NORMAL_ENCRYPTION));
  EXPECT_TRUE(IsGenerationEnabled());

  EXPECT_CALL(*client_, GetPasswordSyncState())
      .WillRepeatedly(testing::Return(SYNCING_WITH_CUSTOM_PASSPHRASE));
  EXPECT_TRUE(IsGenerationEnabled());

  // Disabling password syncing should cause generation to be disabled.
  EXPECT_CALL(*client_, GetPasswordSyncState())
      .WillRepeatedly(testing::Return(NOT_SYNCING_PASSWORDS));
  EXPECT_FALSE(IsGenerationEnabled());

  // Disabling the PasswordManager should cause generation to be disabled even
  // if syncing is enabled.
  EXPECT_CALL(*client_, IsSavingAndFillingEnabledForCurrentPage())
      .WillRepeatedly(testing::Return(false));
  EXPECT_CALL(*client_, GetPasswordSyncState())
      .WillRepeatedly(testing::Return(SYNCING_NORMAL_ENCRYPTION));
  EXPECT_FALSE(IsGenerationEnabled());

  EXPECT_CALL(*client_, GetPasswordSyncState())
      .WillRepeatedly(testing::Return(SYNCING_WITH_CUSTOM_PASSPHRASE));
  EXPECT_FALSE(IsGenerationEnabled());
}

TEST_F(PasswordGenerationManagerTest, DetectFormsEligibleForGeneration) {
  // Setup so that IsGenerationEnabled() returns true.
  EXPECT_CALL(*client_, IsSavingAndFillingEnabledForCurrentPage())
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(*client_, GetPasswordSyncState())
      .WillRepeatedly(testing::Return(SYNCING_NORMAL_ENCRYPTION));

  autofill::FormData login_form;
  login_form.origin = GURL("http://www.yahoo.com/login/");
  autofill::FormFieldData username;
  username.label = ASCIIToUTF16("username");
  username.name = ASCIIToUTF16("login");
  username.form_control_type = "text";
  login_form.fields.push_back(username);
  autofill::FormFieldData password;
  password.label = ASCIIToUTF16("password");
  password.name = ASCIIToUTF16("password");
  password.form_control_type = "password";
  login_form.fields.push_back(password);
  autofill::FormStructure form1(login_form);
  std::vector<autofill::FormStructure*> forms;
  forms.push_back(&form1);

  autofill::FormData account_creation_form;
  account_creation_form.origin = GURL("http://accounts.yahoo.com/");
  account_creation_form.action = GURL("http://accounts.yahoo.com/signup");
  account_creation_form.name = ASCIIToUTF16("account_creation_form");
  account_creation_form.fields.push_back(username);
  account_creation_form.fields.push_back(password);
  autofill::FormFieldData confirm_password;
  confirm_password.label = ASCIIToUTF16("confirm_password");
  confirm_password.name = ASCIIToUTF16("confirm_password");
  confirm_password.form_control_type = "password";
  account_creation_form.fields.push_back(confirm_password);
  autofill::FormSignature account_creation_form_signature =
      autofill::CalculateFormSignature(account_creation_form);
  autofill::FieldSignature account_creation_field_signature =
      autofill::CalculateFieldSignatureForField(password);
  autofill::FieldSignature confirmation_field_signature =
      autofill::CalculateFieldSignatureForField(confirm_password);
  autofill::FormStructure form2(account_creation_form);
  forms.push_back(&form2);

  autofill::FormData change_password_form;
  change_password_form.origin = GURL("http://accounts.yahoo.com/");
  change_password_form.action = GURL("http://accounts.yahoo.com/change");
  change_password_form.name = ASCIIToUTF16("change_password_form");
  change_password_form.fields.push_back(password);
  change_password_form.fields[0].name = ASCIIToUTF16("new_password");
  change_password_form.fields.push_back(confirm_password);
  autofill::FormStructure form3(change_password_form);
  autofill::FormSignature change_password_form_signature =
      autofill::CalculateFormSignature(change_password_form);
  autofill::FieldSignature change_password_field_signature =
      autofill::CalculateFieldSignatureForField(change_password_form.fields[0]);
  forms.push_back(&form3);

  // Simulate the server response to set the field types.
  // The server response numbers mean:
  // EMAIL_ADDRESS = 9
  // PASSWORD = 75
  // ACCOUNT_CREATION_PASSWORD = 76
  // NEW_PASSWORD = 88
  // CONFIRMATION_PASSWORD = 95
  autofill::AutofillQueryResponseContents response;
  response.add_field()->set_overall_type_prediction(9);
  response.add_field()->set_overall_type_prediction(75);
  response.add_field()->set_overall_type_prediction(9);
  response.add_field()->set_overall_type_prediction(76);
  response.add_field()->set_overall_type_prediction(75);
  response.add_field()->set_overall_type_prediction(88);
  response.add_field()->set_overall_type_prediction(95);

  std::string response_string;
  ASSERT_TRUE(response.SerializeToString(&response_string));
  autofill::FormStructure::ParseQueryResponse(response_string, forms);

  DetectFormsEligibleForGeneration(forms);
  EXPECT_EQ(2u, GetTestDriver()->GetFoundEligibleForGenerationForms().size());
  EXPECT_EQ(
      account_creation_form_signature,
      GetTestDriver()->GetFoundEligibleForGenerationForms()[0].form_signature);
  EXPECT_EQ(
      account_creation_field_signature,
      GetTestDriver()->GetFoundEligibleForGenerationForms()[0].field_signature);
  EXPECT_FALSE(GetTestDriver()
                   ->GetFoundEligibleForGenerationForms()[0]
                   .confirmation_field_signature.has_value());

  EXPECT_EQ(
      change_password_form_signature,
      GetTestDriver()->GetFoundEligibleForGenerationForms()[1].form_signature);
  EXPECT_EQ(
      change_password_field_signature,
      GetTestDriver()->GetFoundEligibleForGenerationForms()[1].field_signature);
  ASSERT_TRUE(GetTestDriver()
                  ->GetFoundEligibleForGenerationForms()[1]
                  .confirmation_field_signature.has_value());
  EXPECT_EQ(confirmation_field_signature,
            GetTestDriver()
                ->GetFoundEligibleForGenerationForms()[1]
                .confirmation_field_signature.value());
}

TEST_F(PasswordGenerationManagerTest, UpdatePasswordSyncStateIncognito) {
  // Disable password manager by going incognito. Even though password
  // syncing is enabled, generation should still
  // be disabled.
  EXPECT_CALL(*client_, IsIncognito()).WillRepeatedly(testing::Return(true));
  PrefService* prefs = client_->GetPrefs();
  prefs->SetBoolean(prefs::kCredentialsEnableService, true);
  EXPECT_CALL(*client_, GetPasswordSyncState())
      .WillRepeatedly(testing::Return(SYNCING_NORMAL_ENCRYPTION));

  EXPECT_FALSE(IsGenerationEnabled());
}

TEST_F(PasswordGenerationManagerTest, CheckIfFormClassifierShouldRun) {
  const bool kFalseTrue[] = {false, true};
  for (bool is_autofill_field_metadata_enabled : kFalseTrue) {
    SCOPED_TRACE(testing::Message() << "is_autofill_field_metadata_enabled="
                                    << is_autofill_field_metadata_enabled);
    std::unique_ptr<base::FieldTrialList> field_trial_list;
    scoped_refptr<base::FieldTrial> field_trial;
    if (is_autofill_field_metadata_enabled) {
      field_trial_list.reset(new base::FieldTrialList(
          std::make_unique<variations::SHA1EntropyProvider>("foo")));
      field_trial = base::FieldTrialList::CreateFieldTrial(
          "AutofillFieldMetadata", "Enabled");
      EXPECT_CALL(*GetTestDriver(), AllowToRunFormClassifier())
          .WillOnce(testing::Return());
    }
    GetGenerationManager()->CheckIfFormClassifierShouldRun();
  }
}

}  // namespace password_manager
