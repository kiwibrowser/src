// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/import/password_csv_reader.h"

#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/core/browser/android_affiliation/affiliation_utils.h"
#include "components/password_manager/core/browser/import/password_importer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace password_manager {

TEST(PasswordCSVReaderTest, DeserializePasswords_ZeroValid) {
  const char kCSVInput[] = "title,WEBSITE,login,password\n";
  std::vector<autofill::PasswordForm> passwords;
  PasswordCSVReader reader;
  EXPECT_EQ(PasswordImporter::SUCCESS,
            reader.DeserializePasswords(kCSVInput, &passwords));
  EXPECT_EQ(0u, passwords.size());
}

TEST(PasswordCSVReaderTest, DeserializePasswords_SingleValid) {
  const char kCSVInput[] =
      "Url,Username,Password\n"
      "https://accounts.google.com/a/LoginAuth,test@gmail.com,test1\n";
  std::vector<autofill::PasswordForm> passwords;
  PasswordCSVReader reader;
  EXPECT_EQ(PasswordImporter::SUCCESS,
            reader.DeserializePasswords(kCSVInput, &passwords));
  EXPECT_EQ(1u, passwords.size());
  GURL expected_origin("https://accounts.google.com/a/LoginAuth");
  EXPECT_EQ(expected_origin, passwords[0].origin);
  EXPECT_EQ(expected_origin.GetOrigin().spec(), passwords[0].signon_realm);
  EXPECT_EQ(base::UTF8ToUTF16("test@gmail.com"), passwords[0].username_value);
  EXPECT_EQ(base::UTF8ToUTF16("test1"), passwords[0].password_value);
}

TEST(PasswordCSVReaderTest, DeserializePasswords_SingleAndroid) {
  constexpr char kCSVInput[] =
      "Url,Username,Password\n"
      "android://hash@com.example.android,test@gmail.com,test1\n";
  std::vector<autofill::PasswordForm> passwords;
  PasswordCSVReader reader;
  EXPECT_EQ(PasswordImporter::SUCCESS,
            reader.DeserializePasswords(kCSVInput, &passwords));
  EXPECT_EQ(1u, passwords.size());
  const GURL expected_origin("android://hash@com.example.android");

  const autofill::PasswordForm& password = passwords.front();
  EXPECT_EQ(expected_origin, password.origin);
  EXPECT_EQ(expected_origin.spec(), password.signon_realm);
  EXPECT_TRUE(IsValidAndroidFacetURI(password.signon_realm));
  EXPECT_EQ(base::UTF8ToUTF16("test@gmail.com"), password.username_value);
  EXPECT_EQ(base::UTF8ToUTF16("test1"), password.password_value);
}

TEST(PasswordCSVReaderTest, DeserializePasswords_TwoValid) {
  const char kCSVInput[] =
      "Url,Username,Password,Someotherfield\n"
      "https://accounts.google.com/a/LoginAuth,test@gmail.com,test1,test2\n"
      "http://example.com/,user,password,pwd\n";
  std::vector<autofill::PasswordForm> passwords;
  PasswordCSVReader reader;
  EXPECT_EQ(PasswordImporter::SUCCESS,
            reader.DeserializePasswords(kCSVInput, &passwords));
  EXPECT_EQ(2u, passwords.size());

  GURL expected_origin("https://accounts.google.com/a/LoginAuth");
  EXPECT_EQ(expected_origin, passwords[0].origin);
  EXPECT_EQ(expected_origin.GetOrigin().spec(), passwords[0].signon_realm);
  EXPECT_EQ(base::UTF8ToUTF16("test@gmail.com"), passwords[0].username_value);
  EXPECT_EQ(base::UTF8ToUTF16("test1"), passwords[0].password_value);

  expected_origin = GURL("http://example.com");
  EXPECT_EQ(expected_origin, passwords[1].origin);
  EXPECT_EQ(expected_origin.GetOrigin().spec(), passwords[1].signon_realm);
  EXPECT_EQ(base::UTF8ToUTF16("user"), passwords[1].username_value);
  EXPECT_EQ(base::UTF8ToUTF16("password"), passwords[1].password_value);
}

TEST(PasswordCSVReaderTest, DeserializePasswords_SyntaxError) {
  const char kCSVInput[] =
      "Url,Username,\"Password\n"
      "https://accounts.google.com/a/LoginAuth,test@gmail.com,test1\n";
  std::vector<autofill::PasswordForm> passwords;
  PasswordCSVReader reader;
  EXPECT_EQ(PasswordImporter::SYNTAX_ERROR,
            reader.DeserializePasswords(kCSVInput, &passwords));
  EXPECT_EQ(0u, passwords.size());
}

TEST(PasswordCSVReaderTest, DeserializePasswords_SemanticError) {
  const char kCSVInput[] =
      "Url,Username,Secret\n"  // Password column is missing.
      "https://accounts.google.com/a/LoginAuth,test@gmail.com,test1\n";
  std::vector<autofill::PasswordForm> passwords;
  PasswordCSVReader reader;
  EXPECT_EQ(PasswordImporter::SEMANTIC_ERROR,
            reader.DeserializePasswords(kCSVInput, &passwords));
  EXPECT_EQ(0u, passwords.size());
}

// If the URL value contains non-ASCII characters, the record should be
// rejected.
TEST(PasswordCSVReaderTest, DeserializePasswords_NonASCIIUrl) {
  const char kCSVInput[] =
      "Url,Username,Password\n"
      "https://aččountš.googľe.čom/,test@gmail.com,test1\n"
      "https://example.com/,user,password\n";
  std::vector<autofill::PasswordForm> passwords;
  PasswordCSVReader reader;
  EXPECT_EQ(PasswordImporter::SUCCESS,
            reader.DeserializePasswords(kCSVInput, &passwords));
  // The first record should be ignored, the second imported.
  EXPECT_EQ(1u, passwords.size());
  GURL expected_origin("https://example.com");
  EXPECT_EQ(expected_origin, passwords[0].origin);
  EXPECT_EQ(expected_origin.GetOrigin().spec(), passwords[0].signon_realm);
  EXPECT_EQ(base::UTF8ToUTF16("user"), passwords[0].username_value);
  EXPECT_EQ(base::UTF8ToUTF16("password"), passwords[0].password_value);
}

// If the URL is invalid, the record should be rejected.
TEST(PasswordCSVReaderTest, DeserializePasswords_InvalidUrl) {
  const char kCSVInput[] =
      ",url,user,password\n"
      ",:,,\n"
      ",https://example.com/,user,password\n";
  std::vector<autofill::PasswordForm> passwords;
  PasswordCSVReader reader;
  EXPECT_EQ(PasswordImporter::SUCCESS,
            reader.DeserializePasswords(kCSVInput, &passwords));
  // The first record should be ignored, the second imported.
  EXPECT_EQ(1u, passwords.size());
  GURL expected_origin("https://example.com");
  EXPECT_EQ(expected_origin, passwords[0].origin);
  EXPECT_EQ(expected_origin.GetOrigin().spec(), passwords[0].signon_realm);
  EXPECT_EQ(base::UTF8ToUTF16("user"), passwords[0].username_value);
  EXPECT_EQ(base::UTF8ToUTF16("password"), passwords[0].password_value);
}

}  // namespace password_manager
