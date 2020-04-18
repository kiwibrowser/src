// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/import/password_csv_reader.h"

#include <set>

#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/core/browser/android_affiliation/affiliation_utils.h"
#include "components/password_manager/core/browser/import/csv_reader.h"

using autofill::PasswordForm;

namespace password_manager {

namespace {

// Used for sets with case insensitive comparison of string keys.
struct CaseInsensitiveComparison {
  bool operator()(const std::string& lhs, const std::string& rhs) const {
    return base::CompareCaseInsensitiveASCII(lhs, rhs) == -1;
  }
};

// All the three following arrays need to be null-terminated.
// Recognised column names for origin URL.
const char* const url_names[] = {"url", "website", "origin", "hostname",
                                 nullptr};

// Recognised column names for username value.
const char* const username_names[] = {"username", "user", "login", "account",
                                      nullptr};

// Recognised column names for password value.
const char* const password_names[] = {"password", nullptr};

// If |real_names| contain a string equal to some of the |possible_names|,
// returns an arbitrary such member of |possible_names|. Otherwise returns null.
// |possible_names| is expected to be a null-terminated array.
std::string GetIntersectingName(
    const std::set<std::string, CaseInsensitiveComparison>& real_names,
    const char* const possible_names[]) {
  for (; *possible_names; ++possible_names) {
    auto match = real_names.find(*possible_names);
    if (match != real_names.end())
      return *match;
  }
  return std::string();
}

}  // namespace

PasswordCSVReader::PasswordCSVReader() = default;

PasswordCSVReader::~PasswordCSVReader() = default;

PasswordImporter::Result PasswordCSVReader::DeserializePasswords(
    const std::string& input,
    std::vector<PasswordForm>* passwords) {
  std::vector<std::string> header;
  std::vector<std::map<std::string, std::string>> records;
  if (!ReadCSV(input, &header, &records))
    return PasswordImporter::SYNTAX_ERROR;

  // Put the names into a set with case insensitive comparison.
  std::set<std::string, CaseInsensitiveComparison> lowercase_column_names;
  for (const auto& name : header) {
    lowercase_column_names.insert(name);
  }
  url_field_name_ = GetIntersectingName(lowercase_column_names, url_names);
  username_field_name_ =
      GetIntersectingName(lowercase_column_names, username_names);
  password_field_name_ =
      GetIntersectingName(lowercase_column_names, password_names);
  if (url_field_name_.empty() || username_field_name_.empty() ||
      password_field_name_.empty()) {
    return PasswordImporter::SEMANTIC_ERROR;
  }

  passwords->clear();
  passwords->reserve(records.size());

  for (const auto& record : records) {
    PasswordForm form;
    if (RecordToPasswordForm(record, &form))
      passwords->push_back(form);
  }
  return PasswordImporter::SUCCESS;
}

bool PasswordCSVReader::RecordToPasswordForm(
    const std::map<std::string, std::string>& record,
    PasswordForm* form) {
  GURL origin;
  auto origin_in_record = record.find(url_field_name_);

  if (origin_in_record == record.end() ||
      !base::IsStringASCII(origin_in_record->second)) {
    return false;
  }
  origin = GURL(origin_in_record->second);
  if (!origin.is_valid())
    return false;

  base::string16 username_value;
  auto username_in_record = record.find(username_field_name_);
  if (username_in_record == record.end())
    return false;
  username_value = base::UTF8ToUTF16(username_in_record->second);

  base::string16 password_value;
  auto password_in_record = record.find(password_field_name_);
  if (password_in_record == record.end())
    return false;
  password_value = base::UTF8ToUTF16(password_in_record->second);

  form->origin.Swap(&origin);
  // |GURL::GetOrigin| returns an empty GURL for Android credentials due to the
  // non-standard scheme ("android://"). Hence the following explicit check is
  // necessary to set |signon_realm| correctly for both regular and Android
  // credentials.
  form->signon_realm = IsValidAndroidFacetURI(form->origin.spec())
                           ? form->origin.spec()
                           : form->origin.GetOrigin().spec();
  form->username_value.swap(username_value);
  form->password_value.swap(password_value);
  return true;
}

}  // namespace password_manager
