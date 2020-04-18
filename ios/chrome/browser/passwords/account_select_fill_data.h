// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PASSWORDS_ACCOUNT_SELECT_FILL_DATA_H_
#define IOS_CHROME_BROWSER_PASSWORDS_ACCOUNT_SELECT_FILL_DATA_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "url/gurl.h"

namespace autofill {
struct PasswordFormFillData;
}

namespace password_manager {

struct UsernameAndRealm {
  base::string16 username;
  std::string realm;
};

// Keeps all required for filling information from Password Form.
struct FormInfo {
  FormInfo();
  ~FormInfo();
  FormInfo(const FormInfo&);
  GURL origin;
  GURL action;
  base::string16 name;
  base::string16 username_element;
  base::string16 password_element;
};

struct Credential {
  Credential(const base::string16& username,
             const base::string16& password,
             const std::string& realm);
  ~Credential();
  base::string16 username;
  base::string16 password;
  std::string realm;
};

// Contains all information whis is required for filling the password form.
struct FillData {
  FillData();
  ~FillData();

  GURL origin;
  GURL action;
  base::string16 username_element;
  base::string16 username_value;
  base::string16 password_element;
  base::string16 password_value;
};

// Handles data and logic for filling on account select. This class stores 2
// types of independent data - forms on the page and credentials saved for the
// current page. Based on the user action (clicks, typing values, choosing
// suggestions) this class decides which suggestions should be shown and which
// credentials should be filled.
class AccountSelectFillData {
 public:
  AccountSelectFillData();
  ~AccountSelectFillData();

  // Adds form structure from |form_data| to internal lists of known forms and
  // overrides known credentials with credentials from |form_data|. So only the
  // credentials from the latest |form_data| will be shown to the user.
  void Add(const autofill::PasswordFormFillData& form_data);
  void Reset();
  bool Empty() const;

  // Returns whether suggestions are available for field with name
  // |field_name| which is in the form with name |form_name|.
  bool IsSuggestionsAvailable(const base::string16& form_name,
                              const base::string16& field_identifier,
                              bool is_password_field) const;

  // Returns suggestions for field with name |field_name| which is in the form
  // with name |form_name|.
  std::vector<UsernameAndRealm> RetrieveSuggestions(
      const base::string16& form_name,
      const base::string16& field_identifier,
      bool is_password_field) const;

  // Returns data for password form filling based on |username| chosen by the
  // user.
  // RetrieveSuggestions should be called before in order to specify on which
  // field the user clicked.
  std::unique_ptr<FillData> GetFillData(const base::string16& username) const;

 private:
  // Keeps data about all known forms. The key is the pair (form_name, username
  // field_name).
  std::map<base::string16, FormInfo> forms_;

  // Keeps all known credentials.
  std::vector<Credential> credentials_;

  // Mutable because it's updated from RetrieveSuggestions, which is logically
  // should be const.
  // Keeps information about last form that was requested in
  // RetrieveSuggestions.
  mutable const FormInfo* last_requested_form_ = nullptr;
  // Keeps id of the last requested field if it was password otherwise the empty
  // string.
  mutable base::string16 last_requested_password_field_;

  // Returns form information from |forms_| that has form name |form_name|.
  // If |is_password_field| == false and |field_identifier| is not equal to
  // form username_element null is returned. If |is_password_field| == true then
  // |field_identifier| is ignored. That corresponds to the logic, that
  // suggestions should be shown on any password fields.
  const FormInfo* GetFormInfo(const base::string16& form_name,
                              const base::string16& field_identifier,
                              bool is_password_field) const;

  DISALLOW_COPY_AND_ASSIGN(AccountSelectFillData);
};

}  // namespace  password_manager

#endif  // IOS_CHROME_BROWSER_PASSWORDS_ACCOUNT_SELECT_FILL_DATA_H_
