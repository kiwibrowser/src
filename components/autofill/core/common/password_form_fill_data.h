// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_COMMON_PASSWORD_FORM_FILL_DATA_H_
#define COMPONENTS_AUTOFILL_CORE_COMMON_PASSWORD_FORM_FILL_DATA_H_

#include <map>

#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/password_form.h"

namespace autofill {

// Helper struct for PasswordFormFillData
struct UsernamesCollectionKey {
  UsernamesCollectionKey();
  ~UsernamesCollectionKey();

  // Defined so that this struct can be used as a key in a std::map.
  bool operator<(const UsernamesCollectionKey& other) const;

  base::string16 username;
  base::string16 password;
  std::string realm;
};

struct PasswordAndRealm {
  base::string16 password;
  std::string realm;
};

// Structure used for autofilling password forms. Note that the realms in this
// struct are only set when the password's realm differs from the realm of the
// form that we are filling.
struct PasswordFormFillData {
  typedef std::map<base::string16, PasswordAndRealm> LoginCollection;
  typedef std::map<UsernamesCollectionKey,
                   std::vector<base::string16> > UsernamesCollection;

  // The name of the form.
  base::string16 name;

  // An origin URL consists of the scheme, host, port and path; the rest is
  // stripped.
  GURL origin;

  // The action target of the form; like |origin| URL consists of the scheme,
  // host, port and path; the rest is stripped.
  GURL action;

  // Username and password input fields in the form.
  FormFieldData username_field;
  FormFieldData password_field;

  // The signon realm of the preferred user/pass pair.
  std::string preferred_realm;

  // A list of other matching username->PasswordAndRealm pairs for the form.
  LoginCollection additional_logins;

  // Tells us whether we need to wait for the user to enter a valid username
  // before we autofill the password. By default, this is off unless the
  // PasswordManager determined there is an additional risk associated with this
  // form. This can happen, for example, if action URI's of the observed form
  // and our saved representation don't match up.
  bool wait_for_username;

  // True if this form is a change password form.
  bool is_possible_change_password_form;

  PasswordFormFillData();
  PasswordFormFillData(const PasswordFormFillData& other);
  ~PasswordFormFillData();
};

// Create a FillData structure in preparation for autofilling a form,
// from basic_data identifying which form to fill, and a collection of
// matching stored logins to use as username/password values.
// |preferred_match| should equal (address) one of matches.
// |wait_for_username_before_autofill| is true if we should not autofill
// anything until the user typed in a valid username and blurred the field.
// If |enable_possible_usernames| is true, we will populate possible_usernames
// in |result|.
void InitPasswordFormFillData(
    const PasswordForm& form_on_page,
    const std::map<base::string16, const PasswordForm*>& matches,
    const PasswordForm* const preferred_match,
    bool wait_for_username_before_autofill,
    PasswordFormFillData* result);

// Renderer needs to have only a password that should be autofilled, all other
// passwords might be safety erased.
PasswordFormFillData ClearPasswordValues(const PasswordFormFillData& data);

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_COMMON_PASSWORD_FORM_FILL_DATA_H__
