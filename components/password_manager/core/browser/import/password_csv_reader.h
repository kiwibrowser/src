// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_PASSWORD_CSV_READER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_PASSWORD_CSV_READER_H_

#include <map>
#include <string>
#include <vector>

#include "base/macros.h"
#include "components/password_manager/core/browser/import/password_importer.h"

namespace autofill {
struct PasswordForm;
}

namespace password_manager {

class PasswordCSVReader {
 public:
  PasswordCSVReader();
  ~PasswordCSVReader();

  // Interprets |input| as CSV data with password entries and on success
  // clears |*passwords| and fills them with the content from |input|. The
  // success is indicated by returning SUCCESS. The CSV format requirements are:
  // * The first row is the header, with names of columns.
  // * Three columns are mandatory (others ignored): origin URL, username value,
  //   password value.
  // * The mandatory columns are identified by bearing one of the recognised
  //   names (comparison is case insensitive). Please see the implementation
  //   file to find those out. If multiple columns match a names of a single
  //   mandatory column type, an arbitrary one is used.
  PasswordImporter::Result DeserializePasswords(
      const std::string& input,
      std::vector<autofill::PasswordForm>* passwords);

 private:
  // Converts a parsed CSV line |record| to a |form| if possible, and returns
  // false otherwise.
  bool RecordToPasswordForm(const std::map<std::string, std::string>& record,
                            autofill::PasswordForm* form);

  // Deduced values of the column names for the mandatory columns.
  std::string url_field_name_;
  std::string username_field_name_;
  std::string password_field_name_;

  DISALLOW_COPY_AND_ASSIGN(PasswordCSVReader);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_PASSWORD_CSV_READER_H_
