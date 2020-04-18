// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/win/tsf_input_scope.h"

#include <InputScope.h>
#include <stddef.h>

#include "testing/gtest/include/gtest/gtest.h"

namespace ui {
namespace {

struct GetInputScopesTestCase {
  TextInputType input_type;
  TextInputMode input_mode;
  size_t expected_size;
  InputScope expected_input_scopes[2];
};

// Google Test pretty-printer.
void PrintTo(const GetInputScopesTestCase& data, std::ostream* os) {
  *os << " input_type: " << testing::PrintToString(data.input_type)
      << "; input_mode: " << testing::PrintToString(data.input_mode);
}

class TSFInputScopeTest
    : public testing::TestWithParam<GetInputScopesTestCase> {
};

const GetInputScopesTestCase kGetInputScopesTestCases[] = {
    // Test cases of TextInputType.
    {TEXT_INPUT_TYPE_NONE, TEXT_INPUT_MODE_DEFAULT, 1, {IS_DEFAULT}},
    {TEXT_INPUT_TYPE_TEXT, TEXT_INPUT_MODE_DEFAULT, 1, {IS_DEFAULT}},
    {TEXT_INPUT_TYPE_PASSWORD, TEXT_INPUT_MODE_DEFAULT, 1, {IS_PASSWORD}},
    {TEXT_INPUT_TYPE_SEARCH, TEXT_INPUT_MODE_DEFAULT, 1, {IS_SEARCH}},
    {TEXT_INPUT_TYPE_EMAIL,
     TEXT_INPUT_MODE_DEFAULT,
     1,
     {IS_EMAIL_SMTPEMAILADDRESS}},
    {TEXT_INPUT_TYPE_NUMBER, TEXT_INPUT_MODE_DEFAULT, 1, {IS_NUMBER}},
    {TEXT_INPUT_TYPE_TELEPHONE,
     TEXT_INPUT_MODE_DEFAULT,
     1,
     {IS_TELEPHONE_FULLTELEPHONENUMBER}},
    {TEXT_INPUT_TYPE_URL, TEXT_INPUT_MODE_DEFAULT, 1, {IS_URL}},
    {TEXT_INPUT_TYPE_DATE, TEXT_INPUT_MODE_DEFAULT, 1, {IS_DEFAULT}},
    {TEXT_INPUT_TYPE_DATE_TIME, TEXT_INPUT_MODE_DEFAULT, 1, {IS_DEFAULT}},
    {TEXT_INPUT_TYPE_DATE_TIME_LOCAL, TEXT_INPUT_MODE_DEFAULT, 1, {IS_DEFAULT}},
    {TEXT_INPUT_TYPE_MONTH, TEXT_INPUT_MODE_DEFAULT, 1, {IS_DEFAULT}},
    {TEXT_INPUT_TYPE_TIME, TEXT_INPUT_MODE_DEFAULT, 1, {IS_DEFAULT}},
    {TEXT_INPUT_TYPE_WEEK, TEXT_INPUT_MODE_DEFAULT, 1, {IS_DEFAULT}},
    {TEXT_INPUT_TYPE_TEXT_AREA, TEXT_INPUT_MODE_DEFAULT, 1, {IS_DEFAULT}},
    {TEXT_INPUT_TYPE_CONTENT_EDITABLE,
     TEXT_INPUT_MODE_DEFAULT,
     1,
     {IS_DEFAULT}},
    {TEXT_INPUT_TYPE_DATE_TIME_FIELD, TEXT_INPUT_MODE_DEFAULT, 1, {IS_DEFAULT}},
    // Test cases of TextInputMode.
    {TEXT_INPUT_TYPE_NONE, TEXT_INPUT_MODE_DEFAULT, 1, {IS_DEFAULT}},
    {TEXT_INPUT_TYPE_NONE, TEXT_INPUT_MODE_TEXT, 1, {IS_DEFAULT}},
    {TEXT_INPUT_TYPE_NONE, TEXT_INPUT_MODE_NUMERIC, 1, {IS_DIGITS}},
    {TEXT_INPUT_TYPE_NONE, TEXT_INPUT_MODE_DECIMAL, 1, {IS_NUMBER}},
    {TEXT_INPUT_TYPE_NONE,
     TEXT_INPUT_MODE_TEL,
     1,
     {IS_TELEPHONE_FULLTELEPHONENUMBER}},
    {TEXT_INPUT_TYPE_NONE,
     TEXT_INPUT_MODE_EMAIL,
     1,
     {IS_EMAIL_SMTPEMAILADDRESS}},
    {TEXT_INPUT_TYPE_NONE, TEXT_INPUT_MODE_URL, 1, {IS_URL}},
    {TEXT_INPUT_TYPE_NONE, TEXT_INPUT_MODE_SEARCH, 1, {IS_SEARCH}},
    // Mixed test cases.
    {TEXT_INPUT_TYPE_EMAIL,
     TEXT_INPUT_MODE_EMAIL,
     1,
     {IS_EMAIL_SMTPEMAILADDRESS}},
    {TEXT_INPUT_TYPE_NUMBER,
     TEXT_INPUT_MODE_NUMERIC,
     2,
     {IS_NUMBER, IS_DIGITS}},
    {TEXT_INPUT_TYPE_TELEPHONE,
     TEXT_INPUT_MODE_TEL,
     1,
     {IS_TELEPHONE_FULLTELEPHONENUMBER}},
    {TEXT_INPUT_TYPE_URL, TEXT_INPUT_MODE_URL, 1, {IS_URL}},
};

TEST_P(TSFInputScopeTest, GetInputScopes) {
  const GetInputScopesTestCase& test_case = GetParam();

  std::vector<InputScope> input_scopes = tsf_inputscope::GetInputScopes(
      test_case.input_type, test_case.input_mode);

  EXPECT_EQ(test_case.expected_size, input_scopes.size());
  for (size_t i = 0; i < test_case.expected_size; ++i)
    EXPECT_EQ(test_case.expected_input_scopes[i], input_scopes[i]);
}

INSTANTIATE_TEST_CASE_P(,
                        TSFInputScopeTest,
                        ::testing::ValuesIn(kGetInputScopesTestCases));

}  // namespace
}  // namespace ui
