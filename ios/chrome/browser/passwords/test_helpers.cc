// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/passwords/test_helpers.h"

#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/common/password_form_fill_data.h"
#include "url/gurl.h"

using autofill::PasswordFormFillData;

namespace test_helpers {

// Populates |form_data| with test values.
void SetPasswordFormFillData(PasswordFormFillData& form_data,
                             const std::string& origin,
                             const std::string& action,
                             const char* username_field,
                             const char* username_value,
                             const char* password_field,
                             const char* password_value,
                             const char* additional_username,
                             const char* additional_password,
                             bool wait_for_username) {
  form_data.origin = GURL(origin);
  form_data.action = GURL(action);
  autofill::FormFieldData username;
  username.name = base::UTF8ToUTF16(username_field);
  username.id = base::UTF8ToUTF16(username_field);
  username.value = base::UTF8ToUTF16(username_value);
  form_data.username_field = username;
  autofill::FormFieldData password;
  password.name = base::UTF8ToUTF16(password_field);
  password.id = base::UTF8ToUTF16(password_field);
  password.value = base::UTF8ToUTF16(password_value);
  form_data.password_field = password;
  if (additional_username) {
    autofill::PasswordAndRealm additional_password_data;
    additional_password_data.password = base::UTF8ToUTF16(additional_password);
    additional_password_data.realm.clear();
    form_data.additional_logins.insert(
        std::pair<base::string16, autofill::PasswordAndRealm>(
            base::UTF8ToUTF16(additional_username), additional_password_data));
  }
  form_data.wait_for_username = wait_for_username;
}

}  // namespace  test_helpers
