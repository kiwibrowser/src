// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/common/password_form_fill_data.h"

#include <tuple>

#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/common/form_field_data.h"

namespace autofill {

UsernamesCollectionKey::UsernamesCollectionKey() {}

UsernamesCollectionKey::~UsernamesCollectionKey() {}

bool UsernamesCollectionKey::operator<(
    const UsernamesCollectionKey& other) const {
  return std::tie(username, password, realm) <
         std::tie(other.username, other.password, other.realm);
}

PasswordFormFillData::PasswordFormFillData()
    : wait_for_username(false), is_possible_change_password_form(false) {}

PasswordFormFillData::PasswordFormFillData(const PasswordFormFillData& other) =
    default;

PasswordFormFillData::~PasswordFormFillData() {
}

void InitPasswordFormFillData(
    const PasswordForm& form_on_page,
    const std::map<base::string16, const PasswordForm*>& matches,
    const PasswordForm* const preferred_match,
    bool wait_for_username_before_autofill,
    PasswordFormFillData* result) {
  // Note that many of the |FormFieldData| members are not initialized for
  // |username_field| and |password_field| because they are currently not used
  // by the password autocomplete code.
  FormFieldData username_field;
  username_field.name = form_on_page.username_element;
  username_field.value = preferred_match->username_value;
  FormFieldData password_field;
  password_field.name = form_on_page.password_element;
  password_field.value = preferred_match->password_value;
  password_field.form_control_type = "password";

  // Fill basic form data.
  result->name = form_on_page.form_data.name;
  result->origin = form_on_page.origin;
  result->action = form_on_page.action;
  result->username_field = username_field;
  result->password_field = password_field;
  result->wait_for_username = wait_for_username_before_autofill;
  result->is_possible_change_password_form =
      form_on_page.IsPossibleChangePasswordForm();

  if (preferred_match->is_public_suffix_match ||
      preferred_match->is_affiliation_based_match)
    result->preferred_realm = preferred_match->signon_realm;

  // Copy additional username/value pairs.
  for (const auto& it : matches) {
    if (it.second != preferred_match) {
      PasswordAndRealm value;
      value.password = it.second->password_value;
      if (it.second->is_public_suffix_match ||
          it.second->is_affiliation_based_match)
        value.realm = it.second->signon_realm;
      result->additional_logins[it.first] = value;
    }
  }
}

PasswordFormFillData ClearPasswordValues(const PasswordFormFillData& data) {
  // In case when there is a username on a page (for example in a hidden field),
  // credentials from |additional_logins| could be used for filling on load. So
  // in case of filling on load nor |password_field| nor |additional_logins|
  // can't be cleared
  if (!data.wait_for_username)
    return data;
  PasswordFormFillData result(data);
  result.password_field.value.clear();
  for (auto& credentials : result.additional_logins)
    credentials.second.password.clear();
  return result;
}

}  // namespace autofill
