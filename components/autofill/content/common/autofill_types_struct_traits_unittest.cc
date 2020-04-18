// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/content/common/test_autofill_types.mojom.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/form_field_data.h"
#include "components/autofill/core/common/signatures_util.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace autofill {

const std::vector<const char*> kOptions = {"Option1", "Option2", "Option3",
                                           "Option4"};
namespace {

void CreateTestFieldDataPredictions(const std::string& signature,
                                    FormFieldDataPredictions* field_predict) {
  test::CreateTestSelectField("TestLabel", "TestName", "TestValue", kOptions,
                              kOptions, 4, &field_predict->field);
  field_predict->signature = signature;
  field_predict->heuristic_type = "TestSignature";
  field_predict->server_type = "TestServerType";
  field_predict->overall_type = "TestOverallType";
  field_predict->parseable_name = "TestParseableName";
  field_predict->section = "TestSection";
}

void CreateTestPasswordFormFillData(PasswordFormFillData* fill_data) {
  fill_data->name = base::ASCIIToUTF16("TestName");
  fill_data->origin = GURL("https://foo.com/");
  fill_data->action = GURL("https://foo.com/login");
  test::CreateTestSelectField("TestUsernameFieldLabel", "TestUsernameFieldName",
                              "TestUsernameFieldValue", kOptions, kOptions, 4,
                              &fill_data->username_field);
  test::CreateTestSelectField("TestPasswordFieldLabel", "TestPasswordFieldName",
                              "TestPasswordFieldValue", kOptions, kOptions, 4,
                              &fill_data->password_field);
  fill_data->preferred_realm = "https://foo.com/";

  base::string16 name;
  PasswordAndRealm pr;
  name = base::ASCIIToUTF16("Tom");
  pr.password = base::ASCIIToUTF16("Tom_Password");
  pr.realm = "https://foo.com/";
  fill_data->additional_logins[name] = pr;
  name = base::ASCIIToUTF16("Jerry");
  pr.password = base::ASCIIToUTF16("Jerry_Password");
  pr.realm = "https://bar.com/";
  fill_data->additional_logins[name] = pr;

  fill_data->wait_for_username = true;
  fill_data->is_possible_change_password_form = false;
}

void CreateTestPasswordForm(PasswordForm* form) {
  form->scheme = PasswordForm::Scheme::SCHEME_HTML;
  form->signon_realm = "https://foo.com/";
  form->origin = GURL("https://foo.com/");
  form->action = GURL("https://foo.com/login");
  form->affiliated_web_realm = "https://foo.com/";
  form->submit_element = base::ASCIIToUTF16("test_submit");
  form->username_element = base::ASCIIToUTF16("username");
  form->username_marked_by_site = true;
  form->username_value = base::ASCIIToUTF16("test@gmail.com");
  form->other_possible_usernames.push_back(ValueElementPair(
      base::ASCIIToUTF16("Jerry_1"), base::ASCIIToUTF16("id1")));
  form->other_possible_usernames.push_back(ValueElementPair(
      base::ASCIIToUTF16("Jerry_2"), base::ASCIIToUTF16("id2")));
  form->all_possible_passwords.push_back(
      ValueElementPair(base::ASCIIToUTF16("pass1"), base::ASCIIToUTF16("el1")));
  form->all_possible_passwords.push_back(
      ValueElementPair(base::ASCIIToUTF16("pass2"), base::ASCIIToUTF16("el2")));
  form->form_has_autofilled_value = true;
  form->password_element = base::ASCIIToUTF16("password");
  form->password_value = base::ASCIIToUTF16("test");
  form->password_value_is_default = true;
  form->new_password_element = base::ASCIIToUTF16("new_password");
  form->new_password_value = base::ASCIIToUTF16("new_password_value");
  form->new_password_value_is_default = false;
  form->new_password_marked_by_site = false;
  form->new_password_element = base::ASCIIToUTF16("confirmation_password");
  form->preferred = false;
  form->date_created = base::Time::Now();
  form->date_synced = base::Time::Now();
  form->blacklisted_by_user = false;
  form->type = PasswordForm::Type::TYPE_GENERATED;
  form->times_used = 999;
  test::CreateTestAddressFormData(&form->form_data);
  form->generation_upload_status =
      PasswordForm::GenerationUploadStatus::POSITIVE_SIGNAL_SENT;
  form->display_name = base::ASCIIToUTF16("test display name");
  form->icon_url = GURL("https://foo.com/icon.png");
  form->federation_origin = url::Origin::Create(GURL("http://wwww.google.com"));
  form->skip_zero_click = false;
  form->layout = PasswordForm::Layout::LAYOUT_LOGIN_AND_SIGNUP;
  form->was_parsed_using_autofill_predictions = false;
  form->is_public_suffix_match = true;
  form->is_affiliation_based_match = true;
  form->submission_event =
      PasswordForm::SubmissionIndicatorEvent::SAME_DOCUMENT_NAVIGATION;
}

void CreateTestFormsPredictionsMap(FormsPredictionsMap* predictions) {
  FormsPredictionsMap& result_map = *predictions;
  // 1st element.
  FormData form_data;
  test::CreateTestAddressFormData(&form_data);
  ASSERT_TRUE(form_data.fields.size() >= 4);
  result_map[form_data][form_data.fields[0]] =
      PasswordFormFieldPredictionType::PREDICTION_USERNAME;
  result_map[form_data][form_data.fields[1]] =
      PasswordFormFieldPredictionType::PREDICTION_CURRENT_PASSWORD;
  result_map[form_data][form_data.fields[2]] =
      PasswordFormFieldPredictionType::PREDICTION_NEW_PASSWORD;
  result_map[form_data][form_data.fields[3]] =
      PasswordFormFieldPredictionType::PREDICTION_NOT_PASSWORD;

  // 2nd element.
  form_data.fields.clear();
  result_map[form_data] =
      std::map<FormFieldData, PasswordFormFieldPredictionType>();

  // 3rd element.
  FormFieldData field_data;
  test::CreateTestSelectField("TestLabel1", "TestName1", "TestValue1", kOptions,
                              kOptions, 4, &field_data);
  form_data.fields.push_back(field_data);
  test::CreateTestSelectField("TestLabel2", "TestName2", "TestValue2", kOptions,
                              kOptions, 4, &field_data);
  form_data.fields.push_back(field_data);
  result_map[form_data][form_data.fields[0]] =
      PasswordFormFieldPredictionType::PREDICTION_NEW_PASSWORD;
  result_map[form_data][form_data.fields[1]] =
      PasswordFormFieldPredictionType::PREDICTION_CURRENT_PASSWORD;
}

void CheckEqualPasswordFormFillData(const PasswordFormFillData& expected,
                                    const PasswordFormFillData& actual) {
  EXPECT_EQ(expected.name, actual.name);
  EXPECT_EQ(expected.origin, actual.origin);
  EXPECT_EQ(expected.action, actual.action);
  EXPECT_EQ(expected.username_field, actual.username_field);
  EXPECT_EQ(expected.password_field, actual.password_field);
  EXPECT_EQ(expected.preferred_realm, actual.preferred_realm);

  {
    EXPECT_EQ(expected.additional_logins.size(),
              actual.additional_logins.size());
    auto iter1 = expected.additional_logins.begin();
    auto end1 = expected.additional_logins.end();
    auto iter2 = actual.additional_logins.begin();
    auto end2 = actual.additional_logins.end();
    for (; iter1 != end1 && iter2 != end2; ++iter1, ++iter2) {
      EXPECT_EQ(iter1->first, iter2->first);
      EXPECT_EQ(iter1->second.password, iter2->second.password);
      EXPECT_EQ(iter1->second.realm, iter2->second.realm);
    }
    ASSERT_EQ(iter1, end1);
    ASSERT_EQ(iter2, end2);
  }

  EXPECT_EQ(expected.wait_for_username, actual.wait_for_username);
  EXPECT_EQ(expected.is_possible_change_password_form,
            actual.is_possible_change_password_form);
}

void CheckEqualPasswordFormGenerationData(
    const PasswordFormGenerationData& expected,
    const PasswordFormGenerationData& actual) {
  EXPECT_EQ(expected.form_signature, actual.form_signature);
  EXPECT_EQ(expected.field_signature, actual.field_signature);
  ASSERT_EQ(expected.confirmation_field_signature.has_value(),
            actual.confirmation_field_signature.has_value());
  EXPECT_EQ(expected.confirmation_field_signature.value(),
            actual.confirmation_field_signature.value());
}

}  // namespace

class AutofillTypeTraitsTestImpl : public testing::Test,
                                   public mojom::TypeTraitsTest {
 public:
  AutofillTypeTraitsTestImpl() {}

  mojom::TypeTraitsTestPtr GetTypeTraitsTestProxy() {
    mojom::TypeTraitsTestPtr proxy;
    bindings_.AddBinding(this, mojo::MakeRequest(&proxy));
    return proxy;
  }

  // mojom::TypeTraitsTest:
  void PassFormData(const FormData& s, PassFormDataCallback callback) override {
    std::move(callback).Run(s);
  }

  void PassFormFieldData(const FormFieldData& s,
                         PassFormFieldDataCallback callback) override {
    std::move(callback).Run(s);
  }

  void PassFormDataPredictions(
      const FormDataPredictions& s,
      PassFormDataPredictionsCallback callback) override {
    std::move(callback).Run(s);
  }

  void PassFormFieldDataPredictions(
      const FormFieldDataPredictions& s,
      PassFormFieldDataPredictionsCallback callback) override {
    std::move(callback).Run(s);
  }

  void PassPasswordFormFillData(
      const PasswordFormFillData& s,
      PassPasswordFormFillDataCallback callback) override {
    std::move(callback).Run(s);
  }

  void PassPasswordFormGenerationData(
      const PasswordFormGenerationData& s,
      PassPasswordFormGenerationDataCallback callback) override {
    std::move(callback).Run(s);
  }

  void PassPasswordForm(const PasswordForm& s,
                        PassPasswordFormCallback callback) override {
    std::move(callback).Run(s);
  }

  void PassFormsPredictionsMap(
      const FormsPredictionsMap& s,
      PassFormsPredictionsMapCallback callback) override {
    std::move(callback).Run(s);
  }

 private:
  base::MessageLoop loop_;

  mojo::BindingSet<TypeTraitsTest> bindings_;
};

void ExpectFormFieldData(const FormFieldData& expected,
                         const base::Closure& closure,
                         const FormFieldData& passed) {
  EXPECT_EQ(expected, passed);
  closure.Run();
}

void ExpectFormData(const FormData& expected,
                    const base::Closure& closure,
                    const FormData& passed) {
  EXPECT_EQ(expected, passed);
  closure.Run();
}

void ExpectFormFieldDataPredictions(const FormFieldDataPredictions& expected,
                                    const base::Closure& closure,
                                    const FormFieldDataPredictions& passed) {
  EXPECT_EQ(expected, passed);
  closure.Run();
}

void ExpectFormDataPredictions(const FormDataPredictions& expected,
                               const base::Closure& closure,
                               const FormDataPredictions& passed) {
  EXPECT_EQ(expected, passed);
  closure.Run();
}

void ExpectPasswordFormFillData(const PasswordFormFillData& expected,
                                const base::Closure& closure,
                                const PasswordFormFillData& passed) {
  CheckEqualPasswordFormFillData(expected, passed);
  closure.Run();
}

void ExpectPasswordFormGenerationData(
    const PasswordFormGenerationData& expected,
    const base::Closure& closure,
    const PasswordFormGenerationData& passed) {
  CheckEqualPasswordFormGenerationData(expected, passed);
  closure.Run();
}

void ExpectPasswordForm(const PasswordForm& expected,
                        const base::Closure& closure,
                        const PasswordForm& passed) {
  EXPECT_EQ(expected, passed);
  closure.Run();
}

void ExpectFormsPredictionsMap(const FormsPredictionsMap& expected,
                               const base::Closure& closure,
                               const FormsPredictionsMap& passed) {
  EXPECT_EQ(expected, passed);
  closure.Run();
}

TEST_F(AutofillTypeTraitsTestImpl, PassFormFieldData) {
  FormFieldData input;
  test::CreateTestSelectField("TestLabel", "TestName", "TestValue", kOptions,
                              kOptions, 4, &input);
  // Set other attributes to check if they are passed correctly.
  input.id = base::ASCIIToUTF16("id");
  input.autocomplete_attribute = "on";
  input.placeholder = base::ASCIIToUTF16("placeholder");
  input.css_classes = base::ASCIIToUTF16("class1");
  input.max_length = 12345;
  input.is_autofilled = true;
  input.check_status = FormFieldData::CHECKED;
  input.should_autocomplete = true;
  input.role = FormFieldData::ROLE_ATTRIBUTE_PRESENTATION;
  input.text_direction = base::i18n::RIGHT_TO_LEFT;
  input.properties_mask = FieldPropertiesFlags::HAD_FOCUS;

  base::RunLoop loop;
  mojom::TypeTraitsTestPtr proxy = GetTypeTraitsTestProxy();
  proxy->PassFormFieldData(
      input, base::Bind(&ExpectFormFieldData, input, loop.QuitClosure()));
  loop.Run();
}

TEST_F(AutofillTypeTraitsTestImpl, PassFormData) {
  FormData input;
  test::CreateTestAddressFormData(&input);

  base::RunLoop loop;
  mojom::TypeTraitsTestPtr proxy = GetTypeTraitsTestProxy();
  proxy->PassFormData(input,
                      base::Bind(&ExpectFormData, input, loop.QuitClosure()));
  loop.Run();
}

TEST_F(AutofillTypeTraitsTestImpl, PassFormFieldDataPredictions) {
  FormFieldDataPredictions input;
  CreateTestFieldDataPredictions("TestSignature", &input);

  base::RunLoop loop;
  mojom::TypeTraitsTestPtr proxy = GetTypeTraitsTestProxy();
  proxy->PassFormFieldDataPredictions(
      input,
      base::Bind(&ExpectFormFieldDataPredictions, input, loop.QuitClosure()));
  loop.Run();
}

TEST_F(AutofillTypeTraitsTestImpl, PassFormDataPredictions) {
  FormDataPredictions input;
  test::CreateTestAddressFormData(&input.data);
  input.signature = "TestSignature";

  FormFieldDataPredictions field_predict;
  CreateTestFieldDataPredictions("Tom", &field_predict);
  input.fields.push_back(field_predict);
  CreateTestFieldDataPredictions("Jerry", &field_predict);
  input.fields.push_back(field_predict);
  CreateTestFieldDataPredictions("NoOne", &field_predict);
  input.fields.push_back(field_predict);

  base::RunLoop loop;
  mojom::TypeTraitsTestPtr proxy = GetTypeTraitsTestProxy();
  proxy->PassFormDataPredictions(
      input, base::Bind(&ExpectFormDataPredictions, input, loop.QuitClosure()));
  loop.Run();
}

TEST_F(AutofillTypeTraitsTestImpl, PassPasswordFormFillData) {
  PasswordFormFillData input;
  CreateTestPasswordFormFillData(&input);

  base::RunLoop loop;
  mojom::TypeTraitsTestPtr proxy = GetTypeTraitsTestProxy();
  proxy->PassPasswordFormFillData(input, base::Bind(&ExpectPasswordFormFillData,
                                                    input, loop.QuitClosure()));
  loop.Run();
}

TEST_F(AutofillTypeTraitsTestImpl, PassPasswordFormGenerationData) {
  FormData form;
  test::CreateTestAddressFormData(&form);
  FormSignature form_signature = CalculateFormSignature(form);
  FieldSignature field_signature =
      CalculateFieldSignatureForField(form.fields[0]);
  FieldSignature confirmation_field_signature =
      CalculateFieldSignatureForField(form.fields[1]);
  PasswordFormGenerationData input(form_signature, field_signature);
  input.confirmation_field_signature.emplace(confirmation_field_signature);

  base::RunLoop loop;
  mojom::TypeTraitsTestPtr proxy = GetTypeTraitsTestProxy();
  proxy->PassPasswordFormGenerationData(
      input,
      base::Bind(&ExpectPasswordFormGenerationData, input, loop.QuitClosure()));
  loop.Run();
}

TEST_F(AutofillTypeTraitsTestImpl, PassPasswordForm) {
  PasswordForm input;
  CreateTestPasswordForm(&input);

  base::RunLoop loop;
  mojom::TypeTraitsTestPtr proxy = GetTypeTraitsTestProxy();
  proxy->PassPasswordForm(
      input, base::Bind(&ExpectPasswordForm, input, loop.QuitClosure()));
  loop.Run();
}

TEST_F(AutofillTypeTraitsTestImpl, PassFormsPredictionsMap) {
  FormsPredictionsMap input;
  CreateTestFormsPredictionsMap(&input);

  base::RunLoop loop;
  mojom::TypeTraitsTestPtr proxy = GetTypeTraitsTestProxy();
  proxy->PassFormsPredictionsMap(
      input, base::Bind(&ExpectFormsPredictionsMap, input, loop.QuitClosure()));
  loop.Run();
}

}  // namespace autofill
