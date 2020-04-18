// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/passwords/account_select_fill_data.h"

#include "base/strings/string_util.h"
#include "components/autofill/core/common/password_form_fill_data.h"

namespace password_manager {

FillData::FillData() = default;
FillData::~FillData() = default;

FormInfo::FormInfo() = default;
FormInfo::~FormInfo() = default;
FormInfo::FormInfo(const FormInfo&) = default;

Credential::Credential(const base::string16& username,
                       const base::string16& password,
                       const std::string& realm)
    : username(username), password(password), realm(realm) {}
Credential::~Credential() = default;

AccountSelectFillData::AccountSelectFillData() = default;
AccountSelectFillData::~AccountSelectFillData() = default;

void AccountSelectFillData::Add(
    const autofill::PasswordFormFillData& form_data) {
  auto iter_ok = forms_.insert(std::make_pair(form_data.name, FormInfo()));
  FormInfo& form_info = iter_ok.first->second;
  form_info.origin = form_data.origin;
  form_info.action = form_data.action;
  form_info.username_element = form_data.username_field.name;
  form_info.password_element = form_data.password_field.name;

  // Suggested credentials don't depend on a clicked form. It's better to use
  // the latest known credentials, since credentials can be updated between
  // loading of different forms.
  credentials_.clear();
  credentials_.push_back({form_data.username_field.value,
                          form_data.password_field.value,
                          form_data.preferred_realm});

  for (const auto& username_password_and_realm : form_data.additional_logins) {
    const base::string16& username = username_password_and_realm.first;
    const base::string16& password =
        username_password_and_realm.second.password;
    const std::string& realm = username_password_and_realm.second.realm;
    credentials_.push_back({username, password, realm});
  }
}

void AccountSelectFillData::Reset() {
  forms_.clear();
  credentials_.clear();
  last_requested_form_ = nullptr;
}

bool AccountSelectFillData::Empty() const {
  return credentials_.empty();
}

bool AccountSelectFillData::IsSuggestionsAvailable(
    const base::string16& form_name,
    const base::string16& field_identifier,
    bool is_password_field) const {
  return GetFormInfo(form_name, field_identifier, is_password_field) != nullptr;
}

std::vector<UsernameAndRealm> AccountSelectFillData::RetrieveSuggestions(
    const base::string16& form_name,
    const base::string16& field_identifier,
    bool is_password_field) const {
  last_requested_form_ =
      GetFormInfo(form_name, field_identifier, is_password_field);
  DCHECK(last_requested_form_);
  if (is_password_field)
    last_requested_password_field_ = field_identifier;
  else
    last_requested_password_field_.clear();
  std::vector<UsernameAndRealm> result;
  for (const Credential& credential : credentials_)
    result.push_back({credential.username, credential.realm});

  return result;
}

std::unique_ptr<FillData> AccountSelectFillData::GetFillData(
    const base::string16& username) const {
  if (!last_requested_form_) {
    NOTREACHED();
    return nullptr;
  }

  auto it = std::find_if(credentials_.begin(), credentials_.end(),
                         [&username](const auto& credential) {
                           return credential.username == username;
                         });
  if (it == credentials_.end())
    return nullptr;
  const Credential& credential = *it;
  auto result = std::make_unique<FillData>();
  result->origin = last_requested_form_->origin;
  result->action = last_requested_form_->action;
  result->username_element = last_requested_form_->username_element;
  result->username_value = credential.username;
  result->password_element = last_requested_password_field_.empty()
                                 ? last_requested_form_->password_element
                                 : last_requested_password_field_;
  result->password_value = credential.password;
  return result;
}

const FormInfo* AccountSelectFillData::GetFormInfo(
    const base::string16& form_name,
    const base::string16& field_identifier,
    bool is_password_field) const {
  auto it = forms_.find(form_name);
  if (it == forms_.end())
    return nullptr;
  return is_password_field || it->second.username_element == field_identifier
             ? &it->second
             : nullptr;
}

}  // namespace  password_manager
