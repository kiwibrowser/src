// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/import/csv_reader.h"

#include <stddef.h>

#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_util.h"
#include "third_party/re2/src/re2/re2.h"

namespace {

// Regular expression that matches and captures the first row in CSV formatted
// data (i.e., until the first newline that is not enclosed in double quotes).
// Will throw away the potential trailing EOL character (which is expected to
// have already been normalized to a single '\n').
const char kFirstRowRE[] =
    // Match and capture sequences of 1.) arbitrary characters inside correctly
    // matched double-quotes, or 2.) characters other than the double quote and
    // EOL. Note that because literal double-quotes are escaped as two double
    // quotes and are always enclosed in double quotes, they do not need special
    // treatment as far as splitting on EOL is concerned. However, this RE will
    // still accept inputs such as: "a"b"c"\n.
    "^((?:\"[^\"]*\"|[^\"\\n])*)"
    // Match and throw away EOL, or match end-of-string.
    "(?:\n|$)";

// Regular expression that matches and captures the value of the first field in
// a CSV formatted row of data. Will throw away the potential trailing comma,
// but not the enclosing double quotes if the value is quoted.
const char kFirstFieldRE[] =
    // Match and capture sequences of 1.) arbitrary characters inside correctly
    // matched double-quotes, or 2.) characters other than the double quote and
    // the field separator comma (,). We do not allow a mix of both kinds so as
    // to reject inputs like: "a"b"c".
    "^((?:\"[^\"]*\")*|[^\",]*)"
    // Match and throw away the field separator, or match end-of-string.
    "(?:,|$)";

// Encapsulates the pre-compiled regular expressions and provides the logic to
// parse fields from a CSV file row by row.
class CSVParser {
 public:
  CSVParser(base::StringPiece csv)
      : remaining_csv_piece_(csv.data(), csv.size()),
        first_row_regex_(kFirstRowRE),
        first_field_regex_(kFirstFieldRE) {}

  // Reads and unescapes values from the next row, and writes them to |fields|.
  // Consumes the EOL terminator. Returns false on syntax error.
  bool ParseNextCSVRow(std::vector<std::string>* fields);

  bool HasMoreRows() const { return !remaining_csv_piece_.empty(); }

 private:
  re2::StringPiece remaining_csv_piece_;

  const RE2 first_row_regex_;
  const RE2 first_field_regex_;

  DISALLOW_COPY_AND_ASSIGN(CSVParser);
};

bool CSVParser::ParseNextCSVRow(std::vector<std::string>* fields) {
  fields->clear();

  re2::StringPiece row;
  if (!RE2::Consume(&remaining_csv_piece_, first_row_regex_, &row))
    return false;

  re2::StringPiece remaining_row_piece(row);
  do {
    re2::StringPiece field;
    if (!RE2::Consume(&remaining_row_piece, first_field_regex_, &field))
      return false;
    if (field.starts_with("\"")) {
      CHECK(field.ends_with("\""));
      CHECK_GE(field.size(), 2u);
      field.remove_prefix(1);
      field.remove_suffix(1);
    }
    std::string field_copy(field.as_string());
    base::ReplaceSubstringsAfterOffset(&field_copy, 0, "\"\"", "\"");
    fields->push_back(field_copy);
  } while (!remaining_row_piece.empty());

  if (row.ends_with(","))
    fields->push_back(std::string());

  return true;
}

}  // namespace

namespace password_manager {

bool ReadCSV(base::StringPiece csv,
             std::vector<std::string>* column_names,
             std::vector<std::map<std::string, std::string>>* records) {
  DCHECK(column_names);
  DCHECK(records);

  column_names->clear();
  records->clear();

  // Normalize EOL sequences so that we uniformly use a single LF character.
  std::string normalized_csv(csv.as_string());
  base::ReplaceSubstringsAfterOffset(&normalized_csv, 0, "\r\n", "\n");

  // Read header row.
  CSVParser parser(normalized_csv);
  if (!parser.ParseNextCSVRow(column_names))
    return false;

  // Reader data records rows.
  std::vector<std::string> fields;
  while (parser.HasMoreRows()) {
    if (!parser.ParseNextCSVRow(&fields))
      return false;

    records->resize(records->size() + 1);
    for (size_t i = 0; i < column_names->size() && i < fields.size(); ++i) {
      records->back()[(*column_names)[i]].swap(fields[i]);
    }
  }

  return true;
}

}  // namespace password_manager
