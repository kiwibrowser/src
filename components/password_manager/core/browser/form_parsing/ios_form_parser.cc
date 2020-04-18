// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/form_parsing/ios_form_parser.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/strings/string_split.h"
#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/password_form.h"

using autofill::FormData;
using autofill::FormFieldData;
using autofill::PasswordForm;

using FieldPointersVector = std::vector<const FormFieldData*>;

namespace password_manager {

namespace {

constexpr char kAutocompleteUsername[] = "username";
constexpr char kAutocompleteCurrentPassword[] = "current-password";
constexpr char kAutocompleteNewPassword[] = "new-password";
constexpr char kAutocompleteCreditCardPrefix[] = "cc-";

// Helper struct that is used to return results from the parsing function.
struct ParseResult {
  const FormFieldData* username_field = nullptr;
  const FormFieldData* password_field = nullptr;
  const FormFieldData* new_password_field = nullptr;
  const FormFieldData* confirmation_password_field = nullptr;

  bool IsEmpty() {
    return password_field == nullptr && new_password_field == nullptr;
  }
};

// Checks in a case-insensitive way if credit card autocomplete attributes for
// the given |element| are present.
bool HasCreditCardAutocompleteAttributes(const FormFieldData& field) {
  std::vector<base::StringPiece> tokens = base::SplitStringPiece(
      field.autocomplete_attribute, base::kWhitespaceASCII,
      base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  return std::find_if(
             tokens.begin(), tokens.end(), [](base::StringPiece token) {
               return base::StartsWith(token, kAutocompleteCreditCardPrefix,
                                       base::CompareCase::INSENSITIVE_ASCII);
             }) != tokens.end();
}

// Checks in a case-insensitive way if the autocomplete attribute for the given
// |element| is present and has the specified |value_in_lowercase|.
bool HasAutocompleteAttributeValue(const FormFieldData& field,
                                   base::StringPiece value_in_lowercase) {
  std::vector<base::StringPiece> tokens = base::SplitStringPiece(
      field.autocomplete_attribute, base::kWhitespaceASCII,
      base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  return std::find_if(tokens.begin(), tokens.end(),
                      [value_in_lowercase](base::StringPiece token) {
                        return base::LowerCaseEqualsASCII(token,
                                                          value_in_lowercase);
                      }) != tokens.end();
}

// Returns text fields from |fields|.
FieldPointersVector GetTextFields(const std::vector<FormFieldData>& fields) {
  FieldPointersVector result;
  result.reserve(fields.size());
  for (const auto& field : fields) {
    if (field.IsTextInputElement())
      result.push_back(&field);
  }
  return result;
}

// Returns fields that do not have credit card related autocomplete attributes.
FieldPointersVector GetNonCreditCardFields(const FieldPointersVector& fields) {
  FieldPointersVector result;
  result.reserve(fields.size());
  for (const auto* field : fields) {
    if (!HasCreditCardAutocompleteAttributes(*field))
      result.push_back(field);
  }
  return result;
}

// Returns true iff there is a password field.
bool HasPasswordField(const FieldPointersVector& fields) {
  for (const FormFieldData* field : fields)
    if (field->form_control_type == "password")
      return true;
  return false;
}

// Returns true iff there is a focusable field.
bool HasFocusableField(const FieldPointersVector& fields) {
  for (const FormFieldData* field : fields) {
    if (field->is_focusable)
      return true;
  }
  return false;
}

// Tries to parse |fields| based on autocomplete attributes.
// Assumption on the usage autocomplete attributes:
// 1. Not more than 1 field with autocomplete=username.
// 2. Not more than 1 field with autocomplete=current-password.
// 3. Not more than 2 fields with autocomplete=new-password.
// In case of violating of any of these assumption, parsing is unsuccessful.
// Returns nullptr if parsing is unsuccessful.
std::unique_ptr<ParseResult> ParseUsingAutocomplete(
    const FieldPointersVector& fields) {
  std::unique_ptr<ParseResult> result = std::make_unique<ParseResult>();
  for (const FormFieldData* field : fields) {
    if (HasAutocompleteAttributeValue(*field, kAutocompleteUsername)) {
      if (result->username_field)
        return nullptr;
      result->username_field = field;
    } else if (HasAutocompleteAttributeValue(*field,
                                             kAutocompleteCurrentPassword)) {
      if (result->password_field)
        return nullptr;
      result->password_field = field;
    } else if (HasAutocompleteAttributeValue(*field,
                                             kAutocompleteNewPassword)) {
      // The first field with autocomplete=new-password is considered to be
      // new_password_field and the second is confirmation_password_field.
      if (!result->new_password_field)
        result->new_password_field = field;
      else if (!result->confirmation_password_field)
        result->confirmation_password_field = field;
      else
        return nullptr;
    }
  }

  return result->IsEmpty() ? nullptr : std::move(result);
}

// Returns the fields of |fields| which are password fields if |is_password| is
// true and text fields otherwise. In addition, it drops non-focusable fields if
// |only_focusable| is true, and empty fields if |only_non_empty| is true.
FieldPointersVector FilterFields(const FieldPointersVector& fields,
                                 bool is_password,
                                 bool only_focusable,
                                 bool only_non_empty) {
  FieldPointersVector result;
  for (const FormFieldData* field : fields) {
    if ((field->form_control_type == "password") != is_password)
      continue;
    if (only_focusable && !field->is_focusable)
      continue;
    if (only_non_empty && field->value.empty())
      continue;
    result.push_back(field);
  }
  return result;
}

// Returns only relevant password fields from |fields|. Namely
// 1. If there is a focusable password field, return only focusable.
// 2. If mode == SAVING return only non-empty fields (for saving empty fields
// are useless).
// Note that focusability is the proxy for visibility.
FieldPointersVector GetRelevantPasswords(const FieldPointersVector& fields,
                                         FormParsingMode mode) {
  const bool is_there_focusable_password_field =
      std::any_of(fields.begin(), fields.end(), [](const auto* field) {
        return field->is_focusable && field->form_control_type == "password";
      });

  return FilterFields(fields, true, is_there_focusable_password_field,
                      mode == FormParsingMode::SAVING);
}

// Detects different password fields from |passwords|.
void LocateSpecificPasswords(const FieldPointersVector& passwords,
                             const FormFieldData** current_password,
                             const FormFieldData** new_password,
                             const FormFieldData** confirmation_password) {
  switch (passwords.size()) {
    case 1:
      *current_password = passwords[0];
      break;
    case 2:
      if (!passwords[0]->value.empty() &&
          passwords[0]->value == passwords[1]->value) {
        // Two identical non-empty passwords: assume we are seeing a new
        // password with a confirmation. This can be either a sign-up form or a
        // password change form that does not ask for the old password.
        *new_password = passwords[0];
        *confirmation_password = passwords[1];
      } else {
        // Assume first is old password, second is new (no choice but to guess).
        // This case also includes empty passwords in order to allow filling of
        // password change forms (that also could autofill for sign up form, but
        // we can't do anything with this using only client side information).
        *current_password = passwords[0];
        *new_password = passwords[1];
      }
      break;
    default:
      // If there are more 3 passwords it is not very clear what this form it
      // is. Consider only the first 3 passwords in such case in hope that it
      // would be useful.
      if (!passwords[0]->value.empty() &&
          passwords[0]->value == passwords[1]->value &&
          passwords[0]->value == passwords[2]->value) {
        // All passwords are the same. For the specifiy assume that the first
        // field is the current password.
        *current_password = passwords[0];
      } else if (passwords[1]->value == passwords[2]->value) {
        // New password is the duplicated one, and comes second; or empty form
        // with at least 3 password fields.
        *current_password = passwords[0];
        *new_password = passwords[1];
        *confirmation_password = passwords[2];
      } else if (passwords[0]->value == passwords[1]->value) {
        // It is strange that the new password comes first, but trust more which
        // fields are duplicated than the ordering of fields. Assume that
        // any password fields after the new password contain sensitive
        // information that isn't actually a password (security hint, SSN, etc.)
        *new_password = passwords[0];
        *confirmation_password = passwords[1];
      } else {
        // Three different passwords, or first and last match with middle
        // different. No idea which is which. Let's save the first password.
        // Password selection in a prompt will allow to correct the choice.
        *current_password = passwords[0];
      }
  }
}

// Tries to find username elements among text fields from |fields| before
// |first_relevant_password|.
// Returns nullptr if the username is not found.
const FormFieldData* FindUsernameFieldBaseHeuristics(
    const FieldPointersVector& fields,
    const FormFieldData* first_relevant_password,
    FormParsingMode mode) {
  DCHECK(first_relevant_password);
  DCHECK(find(fields.begin(), fields.end(), first_relevant_password) !=
         fields.end());

  // Let username_candidates be all non-password fields before
  // |first_relevant_password|.
  auto first_relevant_password_it =
      std::find(fields.begin(), fields.end(), first_relevant_password);
  FieldPointersVector username_candidates(fields.begin(),
                                          first_relevant_password_it);

  // For saving filter out empty fields.
  const bool consider_only_non_empty = mode == FormParsingMode::SAVING;
  username_candidates =
      FilterFields(username_candidates, false /* is_password */,
                   false /* only_focusable */, consider_only_non_empty);

  // If there is a focusable username candidate than username should be
  // focusable.
  if (HasFocusableField(username_candidates))
    username_candidates = FilterFields(
        username_candidates, false /* is_password */, true /* only_focusable */,
        consider_only_non_empty /* only_non_empty */);

  // According to base heuristics, username should be the last from
  // |username_candidates|.
  return username_candidates.empty() ? nullptr : *username_candidates.rbegin();
}

std::unique_ptr<ParseResult> ParseUsingBaseHeuristics(
    const FieldPointersVector& fields,
    FormParsingMode mode) {
  // Try to find password elements (current, new, confirmation).
  FieldPointersVector passwords = GetRelevantPasswords(fields, mode);
  if (passwords.empty())
    return nullptr;

  std::unique_ptr<ParseResult> result = std::make_unique<ParseResult>();
  LocateSpecificPasswords(passwords, &result->password_field,
                          &result->new_password_field,
                          &result->confirmation_password_field);
  if (result->IsEmpty())
    return nullptr;

  // If password elements are found then try to find a username.
  result->username_field =
      FindUsernameFieldBaseHeuristics(fields, passwords[0], mode);
  return result;
}

// Set username and password fields from |parse_result| in |password_form|.
void SetFields(const ParseResult& parse_result, PasswordForm* password_form) {
  if (parse_result.username_field) {
    password_form->username_element = parse_result.username_field->id;
    password_form->username_value = parse_result.username_field->value;
  }

  if (parse_result.password_field) {
    password_form->password_element = parse_result.password_field->id;
    password_form->password_value = parse_result.password_field->value;
  }

  if (parse_result.new_password_field) {
    password_form->new_password_element = parse_result.new_password_field->id;
    password_form->new_password_value = parse_result.new_password_field->value;
  }

  if (parse_result.confirmation_password_field) {
    password_form->confirmation_password_element =
        parse_result.confirmation_password_field->id;
  }
}

}  // namespace

std::unique_ptr<PasswordForm> ParseFormData(const FormData& form_data,
                                            FormParsingMode mode) {
  FieldPointersVector fields = GetTextFields(form_data.fields);
  fields = GetNonCreditCardFields(fields);

  // Skip forms without password fields.
  if (!HasPasswordField(fields))
    return nullptr;

  // Create parse result and set non-field related information.
  std::unique_ptr<PasswordForm> result = std::make_unique<PasswordForm>();
  result->origin = form_data.origin;
  result->signon_realm = form_data.origin.GetOrigin().spec();
  result->action = form_data.action;
  result->form_data = form_data;

  // Try to parse with autocomplete attributes.
  auto autocomplete_parse_result = ParseUsingAutocomplete(fields);
  if (autocomplete_parse_result) {
    SetFields(*autocomplete_parse_result, result.get());
    return result;
  }

  // Try to parse with base heuristic.
  auto base_heuristics_parse_result = ParseUsingBaseHeuristics(fields, mode);

  if (!base_heuristics_parse_result)
    return nullptr;

  SetFields(*base_heuristics_parse_result, result.get());
  return result;
}

}  // namespace password_manager
