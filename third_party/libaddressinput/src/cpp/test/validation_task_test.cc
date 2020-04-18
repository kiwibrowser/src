// Copyright (C) 2014 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "validation_task.h"

#include <libaddressinput/address_data.h>
#include <libaddressinput/address_field.h>
#include <libaddressinput/address_problem.h>
#include <libaddressinput/address_validator.h>
#include <libaddressinput/callback.h>
#include <libaddressinput/supplier.h>

#include <cstddef>
#include <memory>
#include <utility>

#include <gtest/gtest.h>

#include "lookup_key.h"
#include "rule.h"
#include "util/size.h"

namespace i18n {
namespace addressinput {

class ValidationTaskTest : public testing::Test {
 public:
  ValidationTaskTest(const ValidationTaskTest&) = delete;
  ValidationTaskTest& operator=(const ValidationTaskTest&) = delete;

 protected:
  ValidationTaskTest()
      : json_(),
        success_(true),
        address_(),
        allow_postal_(false),
        require_name_(false),
        filter_(),
        problems_(),
        expected_(),
        called_(false),
        validated_(BuildCallback(this, &ValidationTaskTest::Validated)) {
    // Add all problems to the filter except those affected by the metadata
    // in region_data_constants.cc.
    static const AddressField kFields[] = {
      COUNTRY,
      ADMIN_AREA,
      LOCALITY,
      DEPENDENT_LOCALITY,
      SORTING_CODE,
      POSTAL_CODE,
      STREET_ADDRESS,
      ORGANIZATION,
      RECIPIENT
    };

    static const AddressProblem kProblems[] = {
      // UNEXPECTED_FIELD is validated using IsFieldUsed().
      // MISSING_REQUIRED_FIELD is validated using IsFieldRequired().
      UNKNOWN_VALUE,
      INVALID_FORMAT,
      MISMATCHING_VALUE,
      USES_P_O_BOX
    };

    for (size_t i = 0; i < size(kFields); ++i) {
      AddressField field = kFields[i];
      for (size_t j = 0; j < size(kProblems); ++j) {
        AddressProblem problem = kProblems[j];
        filter_.insert(std::make_pair(field, problem));
      }
    }

    filter_.insert(std::make_pair(COUNTRY, UNEXPECTED_FIELD));
    filter_.insert(std::make_pair(COUNTRY, MISSING_REQUIRED_FIELD));
    filter_.insert(std::make_pair(RECIPIENT, UNEXPECTED_FIELD));
    filter_.insert(std::make_pair(RECIPIENT, MISSING_REQUIRED_FIELD));
  }

  void Validate() {
    Rule rule[size(LookupKey::kHierarchy)];

    ValidationTask* task = new ValidationTask(
        address_,
        allow_postal_,
        require_name_,
        &filter_,
        &problems_,
        *validated_);

    Supplier::RuleHierarchy hierarchy;

    for (size_t i = 0;
         i < size(LookupKey::kHierarchy) && json_[i] != nullptr; ++i) {
      ASSERT_TRUE(rule[i].ParseSerializedRule(json_[i]));
      hierarchy.rule[i] = &rule[i];
    }

    (*task->supplied_)(success_, *task->lookup_key_, hierarchy);
  }

  const char* json_[size(LookupKey::kHierarchy)];
  bool success_;
  AddressData address_;
  bool allow_postal_;
  bool require_name_;
  FieldProblemMap filter_;
  FieldProblemMap problems_;
  FieldProblemMap expected_;
  bool called_;

 private:
  void Validated(bool success,
                 const AddressData& address,
                 const FieldProblemMap& problems) {
    ASSERT_EQ(success_, success);
    ASSERT_EQ(&address_, &address);
    ASSERT_EQ(&problems_, &problems);
    called_ = true;
  }

  const std::unique_ptr<const AddressValidator::Callback> validated_;
};

namespace {

TEST_F(ValidationTaskTest, FailureCountryRuleNull) {
  success_ = false;

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, FailureCountryRuleEmpty) {
  json_[0] = "{}";
  success_ = false;

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, SuccessCountryRuleNullNameEmpty) {
  expected_.insert(std::make_pair(COUNTRY, MISSING_REQUIRED_FIELD));

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, SuccessCountryRuleNullNameNotEmpty) {
  address_.region_code = "rrr";

  expected_.insert(std::make_pair(COUNTRY, UNKNOWN_VALUE));

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, SuccessCountryRuleEmptyNameEmpty) {
  json_[0] = "{}";

  expected_.insert(std::make_pair(COUNTRY, MISSING_REQUIRED_FIELD));

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, SuccessCountryRuleEmptyNameNotEmpty) {
  json_[0] = "{}";

  address_.region_code = "rrr";

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, MissingRequiredFieldsUS) {
  json_[0] = "{}";

  address_.region_code = "US";

  filter_.insert(std::make_pair(ADMIN_AREA, MISSING_REQUIRED_FIELD));
  filter_.insert(std::make_pair(LOCALITY, MISSING_REQUIRED_FIELD));
  filter_.insert(std::make_pair(POSTAL_CODE, MISSING_REQUIRED_FIELD));
  filter_.insert(std::make_pair(STREET_ADDRESS, MISSING_REQUIRED_FIELD));
  expected_.insert(std::make_pair(ADMIN_AREA, MISSING_REQUIRED_FIELD));
  expected_.insert(std::make_pair(LOCALITY, MISSING_REQUIRED_FIELD));
  expected_.insert(std::make_pair(POSTAL_CODE, MISSING_REQUIRED_FIELD));
  expected_.insert(std::make_pair(STREET_ADDRESS, MISSING_REQUIRED_FIELD));

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, MissingNoRequiredFieldsUS) {
  json_[0] = "{}";

  address_.region_code = "US";
  address_.administrative_area = "sss";
  address_.locality = "ccc";
  address_.postal_code = "zzz";
  address_.address_line.push_back("aaa");
  address_.organization = "ooo";
  address_.recipient = "nnn";

  filter_.insert(std::make_pair(ADMIN_AREA, MISSING_REQUIRED_FIELD));
  filter_.insert(std::make_pair(LOCALITY, MISSING_REQUIRED_FIELD));
  filter_.insert(std::make_pair(POSTAL_CODE, MISSING_REQUIRED_FIELD));
  filter_.insert(std::make_pair(STREET_ADDRESS, MISSING_REQUIRED_FIELD));
  filter_.insert(std::make_pair(ORGANIZATION, MISSING_REQUIRED_FIELD));

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, UnexpectedFieldUS) {
  json_[0] = "{}";

  address_.region_code = "US";
  address_.dependent_locality = "ddd";

  filter_.insert(std::make_pair(DEPENDENT_LOCALITY, UNEXPECTED_FIELD));
  expected_.insert(std::make_pair(DEPENDENT_LOCALITY, UNEXPECTED_FIELD));

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, MissingRequiredFieldRequireName) {
  json_[0] = "{}";

  address_.region_code = "rrr";

  require_name_ = true;

  expected_.insert(std::make_pair(RECIPIENT, MISSING_REQUIRED_FIELD));

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, UnknownValueRuleNull) {
  json_[0] = R"({"fmt":"%R%S","require":"RS","sub_keys":"aa~bb"})";

  address_.region_code = "rrr";
  address_.administrative_area = "sss";

  expected_.insert(std::make_pair(ADMIN_AREA, UNKNOWN_VALUE));

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, NoUnknownValueRuleNotNull) {
  json_[0] = R"({"fmt":"%R%S","require":"RS","sub_keys":"aa~bb"})";
  json_[1] = "{}";

  address_.region_code = "rrr";
  address_.administrative_area = "sss";

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, PostalCodeUnrecognizedFormatTooShort) {
  json_[0] = R"({"fmt":"%Z","zip":"\\d{3}"})";

  address_.region_code = "rrr";
  address_.postal_code = "12";

  expected_.insert(std::make_pair(POSTAL_CODE, INVALID_FORMAT));

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, PostalCodeUnrecognizedFormatTooLong) {
  json_[0] = R"({"fmt":"%Z","zip":"\\d{3}"})";

  address_.region_code = "rrr";
  address_.postal_code = "1234";

  expected_.insert(std::make_pair(POSTAL_CODE, INVALID_FORMAT));

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, PostalCodeRecognizedFormat) {
  json_[0] = R"({"fmt":"%Z","zip":"\\d{3}"})";

  address_.region_code = "rrr";
  address_.postal_code = "123";

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, PostalCodeMismatchingValue1) {
  json_[0] = R"({"fmt":"%Z","zip":"\\d{3}"})";
  json_[1] = R"({"zip":"1"})";

  address_.region_code = "rrr";
  address_.postal_code = "000";

  expected_.insert(std::make_pair(POSTAL_CODE, MISMATCHING_VALUE));

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, PostalCodeMismatchingValue2) {
  json_[0] = R"({"fmt":"%Z","zip":"\\d{3}"})";
  json_[1] = R"({"zip":"1"})";
  json_[2] = R"({"zip":"12"})";

  address_.region_code = "rrr";
  address_.postal_code = "100";

  expected_.insert(std::make_pair(POSTAL_CODE, MISMATCHING_VALUE));

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, PostalCodeMismatchingValue3) {
  json_[0] = R"({"fmt":"%Z","zip":"\\d{3}"})";
  json_[1] = R"({"zip":"1"})";
  json_[2] = R"({"zip":"12"})";
  json_[3] = R"({"zip":"123"})";

  address_.region_code = "rrr";
  address_.postal_code = "120";

  expected_.insert(std::make_pair(POSTAL_CODE, MISMATCHING_VALUE));

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, PostalCodeMatchingValue) {
  json_[0] = R"({"fmt":"%Z","zip":"\\d{3}"})";
  json_[1] = R"({"zip":"1"})";
  json_[2] = R"({"zip":"12"})";
  json_[3] = R"({"zip":"123"})";

  address_.region_code = "rrr";
  address_.postal_code = "123";

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, PostalCodePrefixMismatchingValue) {
  json_[0] = R"({"fmt":"%Z","zip":"\\d{5}"})";
  json_[1] = R"({"zip":"9[0-5]|96[01]"})";

  address_.region_code = "rrr";
  address_.postal_code = "10960";

  expected_.insert(std::make_pair(POSTAL_CODE, MISMATCHING_VALUE));

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, PostalCodeFilterIgnoresMismatching) {
  json_[0] = R"({"zip":"\\d{3}"})";
  json_[1] = R"({"zip":"1"})";

  address_.region_code = "rrr";
  address_.postal_code = "000";

  filter_.erase(POSTAL_CODE);
  filter_.insert(std::make_pair(POSTAL_CODE, INVALID_FORMAT));

  // (POSTAL_CODE, MISMATCHING_VALUE) should not be reported.

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, UsesPoBoxLanguageUnd) {
  json_[0] = R"({"fmt":"%A"})";

  address_.region_code = "rrr";
  address_.address_line.push_back("aaa");
  address_.address_line.push_back("P.O. Box");
  address_.address_line.push_back("aaa");

  expected_.insert(std::make_pair(STREET_ADDRESS, USES_P_O_BOX));

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, UsesPoBoxLanguageDa) {
  json_[0] = R"({"fmt":"%A","languages":"da"})";

  address_.region_code = "rrr";
  address_.address_line.push_back("aaa");
  address_.address_line.push_back("Postboks");
  address_.address_line.push_back("aaa");

  expected_.insert(std::make_pair(STREET_ADDRESS, USES_P_O_BOX));

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, UsesPoBoxLanguageDaNotMatchDe) {
  json_[0] = R"({"fmt":"%A","languages":"da"})";

  address_.region_code = "rrr";
  address_.address_line.push_back("aaa");
  address_.address_line.push_back("Postfach");
  address_.address_line.push_back("aaa");

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

TEST_F(ValidationTaskTest, UsesPoBoxAllowPostal) {
  json_[0] = R"({"fmt":"%A"})";

  address_.region_code = "rrr";
  address_.address_line.push_back("aaa");
  address_.address_line.push_back("P.O. Box");
  address_.address_line.push_back("aaa");

  allow_postal_ = true;

  ASSERT_NO_FATAL_FAILURE(Validate());
  ASSERT_TRUE(called_);
  EXPECT_EQ(expected_, problems_);
}

}  // namespace
}  // namespace addressinput
}  // namespace i18n
