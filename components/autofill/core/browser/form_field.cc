// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/form_field.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <utility>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/address_field.h"
#include "components/autofill/core/browser/autofill_field.h"
#include "components/autofill/core/browser/autofill_scanner.h"
#include "components/autofill/core/browser/credit_card_field.h"
#include "components/autofill/core/browser/email_field.h"
#include "components/autofill/core/browser/form_structure.h"
#include "components/autofill/core/browser/name_field.h"
#include "components/autofill/core/browser/phone_field.h"
#include "components/autofill/core/common/autofill_constants.h"
#include "components/autofill/core/common/autofill_regexes.h"
#include "components/autofill/core/common/autofill_util.h"

namespace autofill {

// There's an implicit precedence determined by the values assigned here. Email
// is currently the most important followed by Phone, Address, Credit Card and
// finally Name.
const float FormField::kBaseEmailParserScore = 1.4f;
const float FormField::kBasePhoneParserScore = 1.3f;
const float FormField::kBaseAddressParserScore = 1.2f;
const float FormField::kBaseCreditCardParserScore = 1.1f;
const float FormField::kBaseNameParserScore = 1.0f;

// static
FieldCandidatesMap FormField::ParseFormFields(
    const std::vector<std::unique_ptr<AutofillField>>& fields,
    bool is_form_tag) {
  // Set up a working copy of the fields to be processed.
  std::vector<AutofillField*> processed_fields;
  for (const auto& field : fields) {
    // Ignore checkable fields as they interfere with parsers assuming context.
    // Eg., while parsing address, "Is PO box" checkbox after ADDRESS_LINE1
    // interferes with correctly understanding ADDRESS_LINE2.
    // Ignore fields marked as presentational, unless for 'select' fields (for
    // synthetic fields.)
    if (IsCheckable(field->check_status) ||
        (field->role == FormFieldData::ROLE_ATTRIBUTE_PRESENTATION &&
         field->form_control_type != "select-one")) {
      continue;
    }
    processed_fields.push_back(field.get());
  }

  FieldCandidatesMap field_candidates;

  // Email pass.
  ParseFormFieldsPass(EmailField::Parse, processed_fields, &field_candidates);
  const size_t email_count = field_candidates.size();

  // Phone pass.
  ParseFormFieldsPass(PhoneField::Parse, processed_fields, &field_candidates);

  // Address pass.
  ParseFormFieldsPass(AddressField::Parse, processed_fields, &field_candidates);

  // Credit card pass.
  ParseFormFieldsPass(CreditCardField::Parse, processed_fields,
                      &field_candidates);

  // Name pass.
  ParseFormFieldsPass(NameField::Parse, processed_fields, &field_candidates);

  // Do not autofill a form if there aren't enough fields. Otherwise, it is
  // very easy to have false positives. See http://crbug.com/447332
  // For <form> tags, make an exception for email fields, which are commonly
  // the only recognized field on account registration sites.
  const bool accept_parsing =
      field_candidates.size() >= MinRequiredFieldsForHeuristics() ||
      (is_form_tag && email_count > 0);

  if (!accept_parsing)
    field_candidates.clear();

  return field_candidates;
}

// static
bool FormField::ParseField(AutofillScanner* scanner,
                           const base::string16& pattern,
                           AutofillField** match) {
  return ParseFieldSpecifics(scanner, pattern, MATCH_DEFAULT, match);
}

// static
bool FormField::ParseFieldSpecifics(AutofillScanner* scanner,
                                    const base::string16& pattern,
                                    int match_type,
                                    AutofillField** match) {
  if (scanner->IsEnd())
    return false;

  const AutofillField* field = scanner->Cursor();

  if (!MatchesFormControlType(field->form_control_type, match_type))
    return false;

  return MatchAndAdvance(scanner, pattern, match_type, match);
}

// static
bool FormField::ParseEmptyLabel(AutofillScanner* scanner,
                                AutofillField** match) {
  return ParseFieldSpecifics(scanner,
                             base::ASCIIToUTF16("^$"),
                             MATCH_LABEL | MATCH_ALL_INPUTS,
                             match);
}

// static
void FormField::AddClassification(const AutofillField* field,
                                  ServerFieldType type,
                                  float score,
                                  FieldCandidatesMap* field_candidates) {
  // Several fields are optional.
  if (field == nullptr)
    return;

  FieldCandidates& candidates = (*field_candidates)[field->unique_name()];
  candidates.AddFieldCandidate(type, score);
}

// static.
bool FormField::MatchAndAdvance(AutofillScanner* scanner,
                                const base::string16& pattern,
                                int match_type,
                                AutofillField** match) {
  AutofillField* field = scanner->Cursor();
  if (FormField::Match(field, pattern, match_type)) {
    if (match)
      *match = field;
    scanner->Advance();
    return true;
  }

  return false;
}

// static
bool FormField::Match(const AutofillField* field,
                      const base::string16& pattern,
                      int match_type) {
  if ((match_type & FormField::MATCH_LABEL) &&
      MatchesPattern(field->label, pattern)) {
    return true;
  }

  if ((match_type & FormField::MATCH_NAME) &&
      MatchesPattern(field->parseable_name(), pattern)) {
    return true;
  }

  return false;
}

// static
void FormField::ParseFormFieldsPass(ParseFunction parse,
                                    const std::vector<AutofillField*>& fields,
                                    FieldCandidatesMap* field_candidates) {
  AutofillScanner scanner(fields);
  while (!scanner.IsEnd()) {
    std::unique_ptr<FormField> form_field = parse(&scanner);
    if (form_field == nullptr) {
      scanner.Advance();
    } else {
      // Add entries into |field_candidates| for each field type found in
      // |fields|.
      form_field->AddClassifications(field_candidates);
    }
  }
}

bool FormField::MatchesFormControlType(const std::string& type,
                                       int match_type) {
  if ((match_type & MATCH_TEXT) && type == "text")
    return true;

  if ((match_type & MATCH_EMAIL) && type == "email")
    return true;

  if ((match_type & MATCH_TELEPHONE) && type == "tel")
    return true;

  if ((match_type & MATCH_SELECT) && type == "select-one")
    return true;

  if ((match_type & MATCH_TEXT_AREA) && type == "textarea")
    return true;

  if ((match_type & MATCH_PASSWORD) && type == "password")
    return true;

  if ((match_type & MATCH_NUMBER) && type == "number")
    return true;

  return false;
}

}  // namespace autofill
