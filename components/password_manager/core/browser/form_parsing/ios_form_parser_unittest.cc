// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/form_parsing/ios_form_parser.h"

#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/form_field_data.h"
#include "components/autofill/core/common/password_form.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using autofill::FormData;
using autofill::FormFieldData;
using autofill::PasswordForm;
using base::ASCIIToUTF16;
using base::UintToString16;

namespace password_manager {

namespace {

constexpr int kFieldNotFound = -1;
struct ParseResultIndices {
  int username_index;
  int password_index;
  int new_password_index;
  int confirmation_password_index;

  bool IsEmpty() const {
    return username_index == kFieldNotFound &&
           password_index == kFieldNotFound &&
           new_password_index == kFieldNotFound &&
           confirmation_password_index == kFieldNotFound;
  }
};

struct TestFieldData {
  bool is_password;
  bool is_focusable = true;
  bool is_empty = true;
  const char* autocomplete_attribute = nullptr;
  // If |value| != nullptr then |is_empty| is ignored.
  // If |value| == nullptr and |is_empty| == false, then the exact field value
  // is assumed to be not important for a test and it will be set to some unique
  // value.
  const char* value = nullptr;

  const char* form_control_type = nullptr;
};

struct FormParsingTestCase {
  const char* description;
  std::vector<TestFieldData> fields;
  ParseResultIndices fill_result;
  ParseResultIndices save_result;
};

class IOSFormParserTest : public testing::Test {
 public:
  IOSFormParserTest() {}

 protected:
  void CheckTestData(const std::vector<FormParsingTestCase>& test_cases);
};

FormData GetFormData(const FormParsingTestCase& test_form) {
  FormData form_data;
  form_data.action = GURL("http://example1.com");
  form_data.origin = GURL("http://example2.com");
  for (size_t i = 0; i < test_form.fields.size(); ++i) {
    const TestFieldData& field_data = test_form.fields[i];
    FormFieldData field;
    // An exact id is not important, set id such that different fields have
    // different id.
    field.id = ASCIIToUTF16("field_id") + UintToString16(i);
    if (field_data.form_control_type)
      field.form_control_type = field_data.form_control_type;
    else
      field.form_control_type = field_data.is_password ? "password" : "text";
    field.is_focusable = field_data.is_focusable;
    if (field_data.value) {
      field.value = ASCIIToUTF16(field_data.value);
    } else if (!field_data.is_empty) {
      // An exact value is not important, set a value with simple pattern, such
      // that different fields have different values.
      field.value = ASCIIToUTF16("field_value") + UintToString16(i);
    }
    if (field_data.autocomplete_attribute)
      field.autocomplete_attribute = field_data.autocomplete_attribute;
    form_data.fields.push_back(field);
  }
  return form_data;
}

// Check that field |fields[field_index]| has type |element_type| and value
// |value|. |element| is the name of this element in parsing
// ("username_element", "password_element" etc), that is used to show diagnostic
// message.
void CheckField(const std::vector<FormFieldData>& fields,
                int field_index,
                const char* element_type,
                const base::string16& element,
                const base::string16* value) {
  SCOPED_TRACE(testing::Message("CheckField, element_type = ") << element_type);
  base::string16 expected_element;
  base::string16 expected_value;
  if (field_index != kFieldNotFound) {
    const FormFieldData& field = fields[field_index];
    expected_element = field.id;
    expected_value = field.value;
  }
  EXPECT_EQ(expected_element, element);
  if (value)
    EXPECT_EQ(expected_value, *value);
}

void CheckPasswordFormFields(const PasswordForm& password_form,
                             const FormData& form_data,
                             const ParseResultIndices& expected_fields) {
  CheckField(form_data.fields, expected_fields.username_index, "username",
             password_form.username_element, &password_form.username_value);

  CheckField(form_data.fields, expected_fields.password_index, "password",
             password_form.password_element, &password_form.password_value);

  CheckField(form_data.fields, expected_fields.new_password_index,
             "new_password", password_form.new_password_element,
             &password_form.new_password_value);

  CheckField(form_data.fields, expected_fields.confirmation_password_index,
             "confirmation_password",
             password_form.confirmation_password_element, nullptr);
}

void IOSFormParserTest::CheckTestData(
    const std::vector<FormParsingTestCase>& test_cases) {
  for (const FormParsingTestCase& test_case : test_cases) {
    const FormData form_data = GetFormData(test_case);
    for (auto mode : {FormParsingMode::FILLING, FormParsingMode::SAVING}) {
      SCOPED_TRACE(
          testing::Message("Test description: ")
          << test_case.description << ", parsing mode = "
          << (mode == FormParsingMode::FILLING ? "Filling" : "Saving"));
      std::unique_ptr<PasswordForm> parsed_form =
          ParseFormData(form_data, mode);

      const ParseResultIndices& expected_indices =
          mode == FormParsingMode::FILLING ? test_case.fill_result
                                           : test_case.save_result;

      if (expected_indices.IsEmpty() != (parsed_form == nullptr)) {
        if (expected_indices.IsEmpty())
          EXPECT_FALSE(parsed_form) << "Expected no parsed results";
        else
          EXPECT_TRUE(parsed_form)
              << "The form is expected to be parsed successfully";
      } else if (!expected_indices.IsEmpty() && parsed_form) {
        EXPECT_TRUE(form_data.SameFormAs(parsed_form->form_data));
        CheckPasswordFormFields(*parsed_form, form_data, expected_indices);
      } else {
        // Expected and parsed results are empty, everything is ok.
      }
    }
  }
}

TEST_F(IOSFormParserTest, NotPasswordForm) {
  std::vector<FormParsingTestCase> test_data = {
      {
          "No fields",
          {},
          {kFieldNotFound, kFieldNotFound, kFieldNotFound, kFieldNotFound},
          {kFieldNotFound, kFieldNotFound, kFieldNotFound, kFieldNotFound},
      },
      {
          "No password fields",
          {{.is_password = false}, {.is_password = false}},
          {kFieldNotFound, kFieldNotFound, kFieldNotFound, kFieldNotFound},
          {kFieldNotFound, kFieldNotFound, kFieldNotFound, kFieldNotFound},
      },
  };

  CheckTestData(test_data);
}

TEST_F(IOSFormParserTest, SkipNotTextFields) {
  std::vector<FormParsingTestCase> test_data = {
      {
          "Select between username and password fields",
          {{.is_password = false, .is_empty = false},
           {.form_control_type = "select", .is_empty = false},
           {.is_password = true, .is_empty = false}},
          {0, 2, kFieldNotFound, kFieldNotFound},
          {0, 2, kFieldNotFound, kFieldNotFound},
      },
  };

  CheckTestData(test_data);
}

TEST_F(IOSFormParserTest, OnlyPasswordFields) {
  std::vector<FormParsingTestCase> test_data = {
      {
          "1 password field",
          {
              {.is_password = true, .is_focusable = true, .is_empty = false},
          },
          {kFieldNotFound, 0, kFieldNotFound, kFieldNotFound},
          {kFieldNotFound, 0, kFieldNotFound, kFieldNotFound},
      },
      {
          "2 password fields, new and confirmation password",
          {
              {.is_password = true, .is_focusable = true, .value = "pw"},
              {.is_password = true, .is_focusable = true, .value = "pw"},
          },
          {kFieldNotFound, kFieldNotFound, 0, 1},
          {kFieldNotFound, kFieldNotFound, 0, 1},
      },
      {
          "2 password fields, current and new password",
          {
              {.is_password = true, .is_focusable = true, .value = "pw1"},
              {.is_password = true, .is_focusable = true, .value = "pw2"},
          },
          {kFieldNotFound, 0, 1, kFieldNotFound},
          {kFieldNotFound, 0, 1, kFieldNotFound},
      },
      {
          "3 password fields, current, new, confirm password",
          {
              {.is_password = true, .is_focusable = true, .value = "pw1"},
              {.is_password = true, .is_focusable = true, .value = "pw2"},
              {.is_password = true, .is_focusable = true, .value = "pw2"},
          },
          {kFieldNotFound, 0, 1, 2},
          {kFieldNotFound, 0, 1, 2},
      },
      {
          "3 password fields with different values",
          {
              {.is_password = true, .is_focusable = true, .value = "pw1"},
              {.is_password = true, .is_focusable = true, .value = "pw2"},
              {.is_password = true, .is_focusable = true, .value = "pw3"},
          },
          {kFieldNotFound, 0, kFieldNotFound, kFieldNotFound},
          {kFieldNotFound, 0, kFieldNotFound, kFieldNotFound},
      },
      {
          "4 password fields, only the first 3 are considered",
          {
              {.is_password = true, .is_focusable = true, .value = "pw1"},
              {.is_password = true, .is_focusable = true, .value = "pw2"},
              {.is_password = true, .is_focusable = true, .value = "pw2"},
              {.is_password = true, .is_focusable = true, .value = "pw3"},
          },
          {kFieldNotFound, 0, 1, 2},
          {kFieldNotFound, 0, 1, 2},
      },
  };

  CheckTestData(test_data);
}

TEST_F(IOSFormParserTest, TestFocusability) {
  std::vector<FormParsingTestCase> test_data = {
      {
          "non-focusable fields are considered when there are no focusable "
          "fields",
          {
              {.is_password = true, .is_focusable = false, .is_empty = false},
              {.is_password = true, .is_focusable = false, .is_empty = false},
          },
          {kFieldNotFound, 0, 1, kFieldNotFound},
          {kFieldNotFound, 0, 1, kFieldNotFound},
      },
      {
          "non-focusable should be skipped when there are focusable fields",
          {
              {.is_password = true, .is_focusable = false, .is_empty = false},
              {.is_password = true, .is_focusable = true, .is_empty = false},
          },
          {kFieldNotFound, 1, kFieldNotFound, kFieldNotFound},
          {kFieldNotFound, 1, kFieldNotFound, kFieldNotFound},
      },
      {
          "non-focusable text fields before password",
          {
              {.is_password = false, .is_focusable = false, .is_empty = false},
              {.is_password = false, .is_focusable = false, .is_empty = false},
              {.is_password = true, .is_focusable = true, .is_empty = false},
          },
          {1, 2, kFieldNotFound, kFieldNotFound},
          {1, 2, kFieldNotFound, kFieldNotFound},
      },
      {
          "focusable and non-focusable text fields before password",
          {
              {.is_password = false, .is_focusable = true, .is_empty = false},
              {.is_password = false, .is_focusable = false, .is_empty = false},
              {.is_password = true, .is_focusable = true, .is_empty = false},
          },
          {0, 2, kFieldNotFound, kFieldNotFound},
          {0, 2, kFieldNotFound, kFieldNotFound},
      },
  };

  CheckTestData(test_data);
}

TEST_F(IOSFormParserTest, TextAndPasswordFields) {
  std::vector<FormParsingTestCase> test_data = {
      {
          "Simple empty sign-in form",
          {{.is_password = false, .is_focusable = true, .is_empty = true},
           {.is_password = true, .is_focusable = true, .is_empty = true}},
          {0, 1, kFieldNotFound, kFieldNotFound},
          // Form with empty fields on saving does not make any sense, so empty
          // parsing.
          {kFieldNotFound, kFieldNotFound, kFieldNotFound, kFieldNotFound},
      },
      {
          "Simple sign-in form with filled data",
          {{.is_password = false, .is_focusable = true, .is_empty = false},
           {.is_password = true, .is_focusable = true, .is_empty = false}},
          {0, 1, kFieldNotFound, kFieldNotFound},
          {0, 1, kFieldNotFound, kFieldNotFound},
      },
      {
          "Empty sign-in form with an extra text field",
          {{.is_password = false, .is_focusable = true, .is_empty = true},
           {.is_password = false, .is_focusable = true, .is_empty = true},
           {.is_password = true, .is_focusable = true, .is_empty = true}},
          {1, 2, kFieldNotFound, kFieldNotFound},
          {kFieldNotFound, kFieldNotFound, kFieldNotFound, kFieldNotFound},
      },
      {
          "Non-empty sign-in form with an extra text field",
          {{.is_password = false, .is_focusable = true, .is_empty = false},
           {.is_password = false, .is_focusable = true, .is_empty = true},
           {.is_password = true, .is_focusable = true, .is_empty = false}},
          {1, 2, kFieldNotFound, kFieldNotFound},
          {0, 2, kFieldNotFound, kFieldNotFound},
      },
      {
          "Empty sign-in form with an extra invisible text field",
          {{.is_password = false, .is_focusable = true, .is_empty = true},
           {.is_password = false, .is_focusable = false, .is_empty = true},
           {.is_password = true, .is_focusable = true, .is_empty = true}},
          {0, 2, kFieldNotFound, kFieldNotFound},
          {kFieldNotFound, kFieldNotFound, kFieldNotFound, kFieldNotFound},
      },
      {
          "Non-empty sign-in form with an extra invisible text field",
          {{.is_password = false, .is_focusable = true, .is_empty = false},
           {.is_password = false, .is_focusable = false, .is_empty = false},
           {.is_password = true, .is_focusable = true, .is_empty = false}},
          {0, 2, kFieldNotFound, kFieldNotFound},
          {0, 2, kFieldNotFound, kFieldNotFound},
      },
      {
          "Simple empty sign-in form with empty username",
          {{.is_password = false, .is_focusable = true, .is_empty = true},
           {.is_password = true, .is_focusable = true, .is_empty = false}},
          {0, 1, kFieldNotFound, kFieldNotFound},
          // Form with empty username does not make sense, so username field
          // should not be found.
          {kFieldNotFound, 1, kFieldNotFound, kFieldNotFound},
      },
  };

  CheckTestData(test_data);
}

TEST_F(IOSFormParserTest, TestAutocomplete) {
  std::vector<FormParsingTestCase> test_data = {
      {
          "All possible password autocomplete attributes and some fields "
          "without autocomplete",
          {
              {.is_password = false, .autocomplete_attribute = "username"},
              {.is_password = false},
              {.is_password = true},
              {.is_password = true,
               .autocomplete_attribute = "current-password"},
              {.is_password = true, .autocomplete_attribute = "new-password"},
              {.is_password = true},
              {.is_password = true, .autocomplete_attribute = "new-password"},
          },
          {0, 3, 4, 6},
          {0, 3, 4, 6},
      },
      {
          "Non-password autocomplete attributes are skipped ",
          {
              {.is_password = false,
               .is_empty = false,
               .autocomplete_attribute = "email"},
              {
                  .is_password = false, .is_empty = false,
              },
              {
                  .is_password = true, .is_empty = false,
              },
              {.is_password = true,
               .is_empty = false,
               .autocomplete_attribute = "password"},
          },
          {1, 2, 3, kFieldNotFound},
          {1, 2, 3, kFieldNotFound},
      },
      {
          "Multiple autocomplete attributes for the same field",
          {
              {.is_password = false,
               .autocomplete_attribute = "email username"},
              {.is_password = false},
              {.is_password = true},
              {.is_password = true,
               .autocomplete_attribute = "abc current-password xyz"},
          },
          {0, 3, kFieldNotFound, kFieldNotFound},
          {0, 3, kFieldNotFound, kFieldNotFound},
      },

      {
          "Multiple username autocomplete attributes, fallback to base "
          "heuristics",
          {
              {.is_password = false, .autocomplete_attribute = "username"},
              {.is_password = false, .autocomplete_attribute = "username"},
              {.is_password = true},
              {.is_password = true,
               .autocomplete_attribute = "current-password"},
          },
          {1, 2, 3, kFieldNotFound},
          {kFieldNotFound, kFieldNotFound, kFieldNotFound, kFieldNotFound},
      },
  };
  CheckTestData(test_data);
}

TEST_F(IOSFormParserTest, SkippingFieldsWithCreditCardFields) {
  std::vector<FormParsingTestCase> test_data = {
      {
          "Simple form with all fields are credit card related",
          {
              {.is_password = false, .autocomplete_attribute = "cc-name"},
              {.is_password = true, .autocomplete_attribute = "cc-any-string"},
          },
          {kFieldNotFound, kFieldNotFound, kFieldNotFound, kFieldNotFound},
          {kFieldNotFound, kFieldNotFound, kFieldNotFound, kFieldNotFound},
      },
      {
          "Multiple autocomplete attributes for the same field",
          {
              // This field should be skipped.
              {.is_password = false,
               .autocomplete_attribute = "cc-name username"},
              {.is_password = true, .is_empty = false},
          },
          {kFieldNotFound, 1, kFieldNotFound, kFieldNotFound},
          {kFieldNotFound, 1, kFieldNotFound, kFieldNotFound},
      },
  };
  CheckTestData(test_data);
}

}  // namespace

}  // namespace password_manager
