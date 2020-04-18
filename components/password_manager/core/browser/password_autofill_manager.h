// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_AUTOFILL_MANAGER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_AUTOFILL_MANAGER_H_

#include <map>

#include "base/callback.h"
#include "base/i18n/rtl.h"
#include "base/macros.h"
#include "components/autofill/core/browser/autofill_client.h"
#include "components/autofill/core/browser/autofill_popup_delegate.h"
#include "components/autofill/core/common/password_form_fill_data.h"
#include "components/password_manager/core/browser/password_manager_metrics_util.h"

namespace gfx {
class RectF;
}

namespace password_manager {

class PasswordManagerClient;
class PasswordManagerDriver;

// This class is responsible for filling password forms.
class PasswordAutofillManager : public autofill::AutofillPopupDelegate {
 public:
  PasswordAutofillManager(PasswordManagerDriver* password_manager_driver,
                          autofill::AutofillClient* autofill_client,
                          PasswordManagerClient* password_client);
  virtual ~PasswordAutofillManager();

  // AutofillPopupDelegate implementation.
  void OnPopupShown() override;
  void OnPopupHidden() override;
  void DidSelectSuggestion(const base::string16& value,
                           int identifier) override;
  void DidAcceptSuggestion(const base::string16& value,
                           int identifier,
                           int position) override;
  bool GetDeletionConfirmationText(const base::string16& value,
                                   int identifier,
                                   base::string16* title,
                                   base::string16* body) override;
  bool RemoveSuggestion(const base::string16& value, int identifier) override;
  void ClearPreviewedForm() override;
  autofill::PopupType GetPopupType() const override;
  autofill::AutofillDriver* GetAutofillDriver() override;
  void RegisterDeletionCallback(base::OnceClosure deletion_callback) override;

  // Invoked when a password mapping is added.
  void OnAddPasswordFormMapping(
      int key,
      const autofill::PasswordFormFillData& fill_data);

  // Handles a request from the renderer to show a popup with the given
  // |suggestions| from the password manager. |options| should be a bitwise mask
  // of autofill::ShowPasswordSuggestionsOptions values.
  void OnShowPasswordSuggestions(int key,
                                 base::i18n::TextDirection text_direction,
                                 const base::string16& typed_username,
                                 int options,
                                 const gfx::RectF& bounds);

  // Handles a request from the renderer to show a popup with an option to check
  // user's saved passwords, used when a password field is not autofilled.
  void OnShowManualFallbackSuggestion(base::i18n::TextDirection text_direction,
                                      const gfx::RectF& bounds);

  // Called when main frame navigates. Not called for in-page navigations.
  void DidNavigateMainFrame();

  // A public version of FillSuggestion(), only for use in tests.
  bool FillSuggestionForTest(int key, const base::string16& username);

  // A public version of PreviewSuggestion(), only for use in tests.
  bool PreviewSuggestionForTest(int key, const base::string16& username);

  // Only use in tests.
  void set_autofill_client(autofill::AutofillClient* autofill_client) {
    autofill_client_ = autofill_client;
  }

 private:
  typedef std::map<int, autofill::PasswordFormFillData> LoginToPasswordInfoMap;

  // Attempts to fill the password associated with user name |username|, and
  // returns true if it was successful.
  bool FillSuggestion(int key, const base::string16& username);

  // Attempts to preview the password associated with user name |username|, and
  // returns true if it was successful.
  bool PreviewSuggestion(int key, const base::string16& username);

  // If |current_username| matches a username for one of the login mappings in
  // |fill_data|, returns true and assigns the password and the original signon
  // realm to |password_and_realm|. Note that if the credential comes from the
  // same realm as the one we're filling to, the |realm| field will be left
  // empty, as this is the behavior of |PasswordFormFillData|.
  // Otherwise, returns false and leaves |password_and_realm| untouched.
  bool GetPasswordAndRealmForUsername(
      const base::string16& current_username,
      const autofill::PasswordFormFillData& fill_data,
      autofill::PasswordAndRealm* password_and_realm);

  // Finds login information for a |node| that was previously filled.
  bool FindLoginInfo(int key, autofill::PasswordFormFillData* found_password);

  // Creates suggestion and records the metrics for the "Form not secure
  // warning".
  autofill::Suggestion CreateFormNotSecureWarning();

  // The logins we have filled so far with their associated info.
  LoginToPasswordInfoMap login_to_password_info_;

  // When the autofill popup should be shown, |form_data_key_| identifies the
  // right password info in |login_to_password_info_|.
  int form_data_key_;

  // The driver that owns |this|.
  PasswordManagerDriver* password_manager_driver_;

  // True if the Form-Not-Secure warning has been shown on the current
  // navigation. Used for metrics.
  bool did_show_form_not_secure_warning_ = false;

  // Context in which the "Show all saved passwords" fallback was shown.
  metrics_util::ShowAllSavedPasswordsContext
      show_all_saved_passwords_shown_context_ =
          metrics_util::SHOW_ALL_SAVED_PASSWORDS_CONTEXT_NONE;

  autofill::AutofillClient* autofill_client_;  // weak

  PasswordManagerClient* password_client_;

  // If not null then it will be called in destructor.
  base::OnceClosure deletion_callback_;

  base::WeakPtrFactory<PasswordAutofillManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PasswordAutofillManager);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_AUTOFILL_MANAGER_H_
