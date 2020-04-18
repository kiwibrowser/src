// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_EXPORT_CSV_READER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_EXPORT_CSV_READER_H_

#include <map>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/strings/string_piece.h"

namespace password_manager {

// Reads tabular data from CSV (Comma Separated Values) format as defined in RFC
// 4180, with the following limitations/relaxations:
//   * The input should be UTF-8 encoded. No code points should be escaped.
//   * The first line must be a header that contains the column names.
//   * Records may be separated by either LF or CRLF sequences. Each CRLF will
//     be converted to LF characters inside quotes.
//   * Inconsistent number of fields within records is handled gracefully. Extra
//     fields are ignored. Missing fields will have no corresponding key-value
//     pair in the record.
//   * Repeated columns of the same name are not supported (the last value will
//     be preserved).
//
// The CSV representation of the data will be read from |csv|. The first line of
// the file should be a header to extract |column_names| from. From each
// subsequent line, the extracted values are put into a map mapping column names
// to the value of the corresponding field, and inserted into |records|. Both
// |column_names| and |records| will be overwritten. Returns false if parsing
// failed due to a syntax error.
bool ReadCSV(base::StringPiece csv,
             std::vector<std::string>* column_names,
             std::vector<std::map<std::string, std::string>>* records);

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_EXPORT_CSV_READER_H_
