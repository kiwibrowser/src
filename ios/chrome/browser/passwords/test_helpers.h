// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PASSWORDS_TEST_HELPERS_H_
#define IOS_CHROME_BROWSER_PASSWORDS_TEST_HELPERS_H_

#include <string>

namespace autofill {
struct PasswordFormFillData;
}

namespace test_helpers {

// Populates |form_data| with test values.
void SetPasswordFormFillData(autofill::PasswordFormFillData& form_data,
                             const std::string& origin,
                             const std::string& action,
                             const char* username_field,
                             const char* username_value,
                             const char* password_field,
                             const char* password_value,
                             const char* additional_username,
                             const char* additional_password,
                             bool wait_for_username);

}  // namespace  test_helpers

#endif  // IOS_CHROME_BROWSER_PASSWORDS_TEST_HELPERS_H_
