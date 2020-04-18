// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/browser_save_password_progress_logger.h"

#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/field_types.h"
#include "components/autofill/core/browser/form_structure.h"
#include "components/autofill/core/browser/proto/server.pb.h"
#include "components/autofill/core/common/password_form.h"
#include "components/autofill/core/common/save_password_progress_logger.h"
#include "components/password_manager/core/browser/stub_log_manager.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace password_manager {

namespace {

class TestLogger : public BrowserSavePasswordProgressLogger {
 public:
  explicit TestLogger(LogManager* log_manager)
      : BrowserSavePasswordProgressLogger(log_manager) {}

  bool LogsContainSubstring(const std::string& substring) {
    return accumulated_log_.find(substring) != std::string::npos;
  }

  std::string accumulated_log() { return accumulated_log_; }

  void SendLog(const std::string& log) override {
    accumulated_log_.append(log);
  }

 private:
  std::string accumulated_log_;
};

class MockLogManager : public StubLogManager {
 public:
  MOCK_CONST_METHOD1(LogSavePasswordProgress, void(const std::string& text));
};

class BrowserSavePasswordProgressLoggerTest : public testing::Test {
 public:
  BrowserSavePasswordProgressLoggerTest() {
    form_.origin = GURL("http://myform.com/form.html");
    form_.action = GURL("http://m.myform.com/submit.html");
    form_.name = base::UTF8ToUTF16("form_name");

    // Add a password field.
    autofill::FormFieldData field;
    field.name = base::UTF8ToUTF16("password");
    field.form_control_type = "password";
    field.is_focusable = true;
    field.autocomplete_attribute = "new-password";
    form_.fields.push_back(field);

    // Add a text field.
    field.name = base::UTF8ToUTF16("email");
    field.form_control_type = "text";
    field.is_focusable = false;
    field.value = base::UTF8ToUTF16("a@example.com");
    field.autocomplete_attribute.clear();
    form_.fields.push_back(field);
  }

 protected:
  autofill::FormData form_;
};

}  // namespace

TEST_F(BrowserSavePasswordProgressLoggerTest, LogFormSignatures) {
  MockLogManager log_manager;
  TestLogger logger(&log_manager);

  autofill::FormStructure form_structure(form_);

  // Add a vote, a generation event and a client-side classifier outcome to the
  // password field.
  autofill::ServerFieldTypeSet type_set;
  type_set.insert(autofill::NEW_PASSWORD);
  form_structure.field(0)->set_possible_types(type_set);
  form_structure.field(0)->set_generation_type(
      autofill::AutofillUploadContents::Field::
          MANUALLY_TRIGGERED_GENERATION_ON_SIGN_UP_FORM);
  form_structure.field(0)->set_form_classifier_outcome(
      autofill::AutofillUploadContents::Field::GENERATION_ELEMENT);

  // Add a server prediction for the text field.
  form_structure.field(1)->set_server_type(autofill::EMAIL_ADDRESS);

  logger.LogFormStructure(
      autofill::SavePasswordProgressLogger::STRING_FORM_VOTES, form_structure);
  SCOPED_TRACE(testing::Message() << "Log string = ["
                                  << logger.accumulated_log() << "]");
  EXPECT_TRUE(logger.LogsContainSubstring("Form votes: {"));
  EXPECT_TRUE(logger.LogsContainSubstring("Signature of form"));
  EXPECT_TRUE(logger.LogsContainSubstring("Origin: http://myform.com"));
  EXPECT_TRUE(logger.LogsContainSubstring("Form fields:"));
  EXPECT_TRUE(logger.LogsContainSubstring(
      "password: 2051817934, type=password, autocomplete=new-password, VOTE: "
      "NEW_PASSWORD, GENERATION_EVENT: "
      "Manual generation on sign-up, CLIENT_SIDE_CLASSIFIER: Generation "
      "element"));
  EXPECT_TRUE(logger.LogsContainSubstring(
      "email: 420638584, type=text, SERVER_PREDICTION: EMAIL_ADDRESS"));
}

TEST_F(BrowserSavePasswordProgressLoggerTest, LogFormData) {
  MockLogManager log_manager;
  TestLogger logger(&log_manager);
  logger.LogFormData(
      autofill::SavePasswordProgressLogger::STRING_FORM_PARSING_INPUT, form_);
  SCOPED_TRACE(testing::Message()
               << "Log string = [" << logger.accumulated_log() << "]");
  EXPECT_TRUE(logger.LogsContainSubstring("Origin: http://myform.com"));
  EXPECT_TRUE(logger.LogsContainSubstring("Action: http://m.myform.com"));
  EXPECT_TRUE(logger.LogsContainSubstring("Form name: form_name"));
  EXPECT_TRUE(logger.LogsContainSubstring("Form with form tag: true"));
  EXPECT_TRUE(logger.LogsContainSubstring("Form fields:"));
  EXPECT_TRUE(logger.LogsContainSubstring(
      "password: type=password, visible, empty, autocomplete=new-password"));
  EXPECT_TRUE(
      logger.LogsContainSubstring("email: type=text, invisible, non-empty"));
}

}  // namespace password_manager
