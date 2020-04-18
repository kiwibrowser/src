// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/base/locale_util.h"

#include <utility>
#include <vector>

#include "base/task_scheduler/post_task.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/login/session/user_session_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/ime/chromeos/input_method_manager.h"
#include "ui/base/ime/chromeos/input_method_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/platform_font_linux.h"

namespace chromeos {

namespace {

struct SwitchLanguageData {
  SwitchLanguageData(const std::string& locale,
                     const bool enable_locale_keyboard_layouts,
                     const bool login_layouts_only,
                     const locale_util::SwitchLanguageCallback& callback,
                     Profile* profile)
      : callback(callback),
        result(locale, std::string(), false),
        enable_locale_keyboard_layouts(enable_locale_keyboard_layouts),
        login_layouts_only(login_layouts_only),
        profile(profile) {}

  const locale_util::SwitchLanguageCallback callback;

  locale_util::LanguageSwitchResult result;
  const bool enable_locale_keyboard_layouts;
  const bool login_layouts_only;
  Profile* profile;
};

// Runs on SequencedWorkerPool thread under PostTaskAndReply().
// So data is owned by "Reply" part of PostTaskAndReply() process.
void SwitchLanguageDoReloadLocale(SwitchLanguageData* data) {
  DCHECK(!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  data->result.loaded_locale =
      ui::ResourceBundle::GetSharedInstance().ReloadLocaleResources(
          data->result.requested_locale);

  data->result.success = !data->result.loaded_locale.empty();
}

// Callback after SwitchLanguageDoReloadLocale() back in UI thread.
void FinishSwitchLanguage(std::unique_ptr<SwitchLanguageData> data) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (data->result.success) {
    g_browser_process->SetApplicationLocale(data->result.loaded_locale);

    if (data->enable_locale_keyboard_layouts) {
      input_method::InputMethodManager* manager =
          input_method::InputMethodManager::Get();
      scoped_refptr<input_method::InputMethodManager::State> ime_state =
          UserSessionManager::GetInstance()->GetDefaultIMEState(data->profile);
      if (data->login_layouts_only) {
        // Enable the hardware keyboard layouts and locale-specific layouts
        // suitable for use on the login screen. This will also switch to the
        // first hardware keyboard layout since the input method currently in
        // use may not be supported by the new locale.
        ime_state->EnableLoginLayouts(
            data->result.loaded_locale,
            manager->GetInputMethodUtil()->GetHardwareLoginInputMethodIds());
      } else {
        // Enable all hardware keyboard layouts. This will also switch to the
        // first hardware keyboard layout.
        ime_state->ReplaceEnabledInputMethods(
            manager->GetInputMethodUtil()->GetHardwareInputMethodIds());

        // Enable all locale-specific layouts.
        std::vector<std::string> input_methods;
        manager->GetInputMethodUtil()->GetInputMethodIdsFromLanguageCode(
            data->result.loaded_locale,
            input_method::kKeyboardLayoutsOnly,
            &input_methods);
        for (std::vector<std::string>::const_iterator it =
                input_methods.begin(); it != input_methods.end(); ++it) {
          ime_state->EnableInputMethod(*it);
        }
      }
    }
  }

  // The font clean up of ResourceBundle should be done on UI thread, since the
  // cached fonts are thread unsafe.
  ui::ResourceBundle::GetSharedInstance().ReloadFonts();
  gfx::PlatformFontLinux::ReloadDefaultFont();
  if (!data->callback.is_null())
    data->callback.Run(data->result);
}

}  // namespace

namespace locale_util {

constexpr const char* kAllowedLocalesFallbackLocale = "en-US";

LanguageSwitchResult::LanguageSwitchResult(const std::string& requested_locale,
                                           const std::string& loaded_locale,
                                           bool success)
    : requested_locale(requested_locale),
      loaded_locale(loaded_locale),
      success(success) {
}

void SwitchLanguage(const std::string& locale,
                    const bool enable_locale_keyboard_layouts,
                    const bool login_layouts_only,
                    const SwitchLanguageCallback& callback,
                    Profile* profile) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  std::unique_ptr<SwitchLanguageData> data(
      new SwitchLanguageData(locale, enable_locale_keyboard_layouts,
                             login_layouts_only, callback, profile));
  base::Closure reloader(
      base::Bind(&SwitchLanguageDoReloadLocale, base::Unretained(data.get())));
  base::PostTaskWithTraitsAndReply(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND}, reloader,
      base::Bind(&FinishSwitchLanguage, base::Passed(std::move(data))));
}

bool IsAllowedLocale(const std::string& locale_code, const PrefService* prefs) {
  const base::Value::ListStorage& allowed_locales =
      prefs->GetList(prefs::kAllowedLocales)->GetList();

  // Empty list means all locales are allowed.
  if (allowed_locales.empty())
    return true;

  // Check if locale is in list of allowed locales.
  return base::ContainsValue(allowed_locales, base::Value(locale_code));
}

std::string GetAllowedFallbackLocale(const PrefService* prefs) {
  // Check the user's preferred languages if one of them is allowed.
  std::string preferred_languages_string =
      prefs->GetString(prefs::kLanguagePreferredLanguages);
  std::vector<std::string> preferred_languages =
      base::SplitString(preferred_languages_string, ",", base::TRIM_WHITESPACE,
                        base::SPLIT_WANT_NONEMPTY);
  for (const std::string& language : preferred_languages) {
    if (IsAllowedLocale(language, prefs))
      return language;
  }

  // Check the allowed languages and return the first valid entry.
  const base::Value::ListStorage& allowed_locales =
      prefs->GetList(prefs::kAllowedLocales)->GetList();
  const std::vector<std::string>& available_locales =
      l10n_util::GetAvailableLocales();
  for (const base::Value& value : allowed_locales) {
    const std::string& locale = value.GetString();
    if (base::ContainsValue(available_locales, locale))
      return locale;
  }

  // default fallback
  return kAllowedLocalesFallbackLocale;
}

}  // namespace locale_util
}  // namespace chromeos
