// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/export/password_csv_writer.h"

#include <memory>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/core/browser/import/csv_reader.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using autofill::PasswordForm;
using testing::AllOf;
using testing::Contains;

namespace password_manager {

TEST(PasswordCSVWriterTest, SerializePasswords_ZeroPasswords) {
  std::vector<std::unique_ptr<PasswordForm>> passwords;

  std::vector<std::string> column_names;
  std::vector<std::map<std::string, std::string>> records;
  ASSERT_TRUE(ReadCSV(PasswordCSVWriter::SerializePasswords(passwords),
                      &column_names, &records));

  EXPECT_THAT(column_names, AllOf(Contains("url"), Contains("username"),
                                  Contains("password")));
  EXPECT_EQ(0u, records.size());
}

TEST(PasswordCSVWriterTest, SerializePasswords_SinglePassword) {
  std::vector<std::unique_ptr<PasswordForm>> passwords;
  PasswordForm form;
  form.origin = GURL("http://example.com");
  form.username_value = base::UTF8ToUTF16("Someone");
  form.password_value = base::UTF8ToUTF16("Secret");
  passwords.push_back(std::make_unique<PasswordForm>(form));

  std::vector<std::string> column_names;
  std::vector<std::map<std::string, std::string>> records;
  ASSERT_TRUE(ReadCSV(PasswordCSVWriter::SerializePasswords(passwords),
                      &column_names, &records));

  EXPECT_THAT(column_names, AllOf(Contains("url"), Contains("username"),
                                  Contains("password")));
  EXPECT_EQ(1u, records.size());
  EXPECT_EQ(records[0]["url"], "http://example.com/");
  EXPECT_EQ(records[0]["username"], "Someone");
  EXPECT_EQ(records[0]["password"], "Secret");
}

TEST(PasswordCSVWriterTest, SerializePasswords_TwoPasswords) {
  std::vector<std::unique_ptr<PasswordForm>> passwords;
  PasswordForm form;
  form.origin = GURL("http://example.com");
  form.username_value = base::UTF8ToUTF16("Someone");
  form.password_value = base::UTF8ToUTF16("Secret");
  passwords.push_back(std::make_unique<PasswordForm>(form));
  form.origin = GURL("http://other.org");
  form.username_value = base::UTF8ToUTF16("Anyone");
  form.password_value = base::UTF8ToUTF16("None");
  passwords.push_back(std::make_unique<PasswordForm>(form));

  std::vector<std::string> column_names;
  std::vector<std::map<std::string, std::string>> records;
  ASSERT_TRUE(ReadCSV(PasswordCSVWriter::SerializePasswords(passwords),
                      &column_names, &records));

  EXPECT_THAT(column_names, AllOf(Contains("url"), Contains("username"),
                                  Contains("password")));
  EXPECT_EQ(2u, records.size());
  EXPECT_EQ(records[0]["url"], "http://example.com/");
  EXPECT_EQ(records[0]["username"], "Someone");
  EXPECT_EQ(records[0]["password"], "Secret");
  EXPECT_EQ(records[1]["url"], "http://other.org/");
  EXPECT_EQ(records[1]["username"], "Anyone");
  EXPECT_EQ(records[1]["password"], "None");
}

}  // namespace password_manager
