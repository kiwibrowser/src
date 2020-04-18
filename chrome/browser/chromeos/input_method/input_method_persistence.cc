// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/input_method/input_method_persistence.h"

#include "base/logging.h"
#include "base/sys_info.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/language_preferences.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "ui/base/ime/chromeos/input_method_util.h"

namespace chromeos {
namespace input_method {
namespace {

void PersistSystemInputMethod(const std::string& input_method) {
  if (!g_browser_process || !g_browser_process->local_state())
    return;

  g_browser_process->local_state()->SetString(
        language_prefs::kPreferredKeyboardLayout, input_method);
}

static void SetUserLastInputMethodPreference(const std::string& username,
                                             const std::string& input_method,
                                             PrefService* local_state) {
  if (!username.empty() && !local_state->ReadOnly()) {
    bool update_succeed = false;
    {
      // Updater may have side-effects, therefore we do not replace
      // entry while updater exists.
      DictionaryPrefUpdate updater(local_state, prefs::kUsersLastInputMethod);
      base::DictionaryValue* const users_last_input_methods = updater.Get();
      if (users_last_input_methods) {
        users_last_input_methods->SetKey(username, base::Value(input_method));
        update_succeed = true;
      }
    }
    if (!update_succeed) {
      // Somehow key kUsersLastInputMethod has value of invalid type.
      // Replace and retry.
      local_state->Set(prefs::kUsersLastInputMethod, base::DictionaryValue());

      DictionaryPrefUpdate updater(local_state, prefs::kUsersLastInputMethod);
      base::DictionaryValue* const users_last_input_methods = updater.Get();
      if (users_last_input_methods) {
        users_last_input_methods->SetKey(username, base::Value(input_method));
        update_succeed = true;
      }
    }
    if (!update_succeed) {
      DVLOG(1) << "Failed to replace local_state.kUsersLastInputMethod: '"
               << prefs::kUsersLastInputMethod << "' for '" << username << "'";
    }
  }
}

// Update user Last keyboard layout for login screen
static void SetUserLastInputMethod(
    const std::string& input_method,
    const chromeos::input_method::InputMethodManager* const manager,
    Profile* profile) {
  // Skip if it's not a keyboard layout. Drop input methods including
  // extension ones.
  if (!manager->IsLoginKeyboard(input_method))
    return;

  if (profile == NULL)
    return;

  PrefService* const local_state = g_browser_process->local_state();

  SetUserLastInputMethodPreference(profile->GetProfileUserName(), input_method,
                                   local_state);
}

void PersistUserInputMethod(const std::string& input_method,
                            InputMethodManager* const manager,
                            Profile* profile) {
  PrefService* user_prefs = NULL;
  // Persist the method on a per user basis. Note that the keyboard settings are
  // stored per user desktop and a visiting window will use the same input
  // method as the desktop it is on (and not of the owner of the window).
  if (profile)
    user_prefs = profile->GetPrefs();
  if (!user_prefs)
    return;
  SetUserLastInputMethod(input_method, manager, profile);

  const std::string current_input_method_on_pref =
      user_prefs->GetString(prefs::kLanguageCurrentInputMethod);
  if (current_input_method_on_pref == input_method)
    return;

  user_prefs->SetString(prefs::kLanguagePreviousInputMethod,
                        current_input_method_on_pref);
  user_prefs->SetString(prefs::kLanguageCurrentInputMethod,
                        input_method);
}

}  // namespace

InputMethodPersistence::InputMethodPersistence(
    InputMethodManager* input_method_manager)
    : input_method_manager_(input_method_manager),
      ui_session_(InputMethodManager::STATE_LOGIN_SCREEN) {
  input_method_manager_->AddObserver(this);
}

InputMethodPersistence::~InputMethodPersistence() {
  input_method_manager_->RemoveObserver(this);
}

void InputMethodPersistence::InputMethodChanged(InputMethodManager* manager,
                                                Profile* profile,
                                                bool show_message) {
  DCHECK_EQ(input_method_manager_, manager);
  const std::string current_input_method =
      manager->GetActiveIMEState()->GetCurrentInputMethod().id();
  // Save the new input method id depending on the current browser state.
  switch (ui_session_) {
    case InputMethodManager::STATE_LOGIN_SCREEN:
      if (!manager->IsLoginKeyboard(current_input_method)) {
        DVLOG(1) << "Only keyboard layouts are supported: "
                 << current_input_method;
        return;
      }
      PersistSystemInputMethod(current_input_method);
      return;
    case InputMethodManager::STATE_BROWSER_SCREEN:
      PersistUserInputMethod(current_input_method, manager, profile);
      return;
    case InputMethodManager::STATE_LOCK_SCREEN:
    case InputMethodManager::STATE_SECONDARY_LOGIN_SCREEN:
      // We use a special set of input methods on the screen. Do not update.
      return;
    case InputMethodManager::STATE_TERMINATING:
      return;
  }
  NOTREACHED();
}

void InputMethodPersistence::OnSessionStateChange(
    InputMethodManager::UISessionState new_ui_session) {
  ui_session_ = new_ui_session;
}

void SetUserLastInputMethodPreferenceForTesting(
    const std::string& username,
    const std::string& input_method,
    PrefService* const local_state) {
  SetUserLastInputMethodPreference(username, input_method, local_state);
}

}  // namespace input_method
}  // namespace chromeos
