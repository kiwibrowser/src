// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/password_manager/generated_password_saved_infobar_delegate_android.h"

#include <stddef.h>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/android/android_theme_resources.h"
#include "chrome/browser/android/preferences/preferences_launcher.h"
#include "chrome/grit/generated_resources.h"
#include "components/infobars/core/infobar_delegate.h"
#include "components/password_manager/core/browser/password_bubble_experiment.h"
#include "components/password_manager/core/browser/password_manager_constants.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

GeneratedPasswordSavedInfoBarDelegateAndroid::
    ~GeneratedPasswordSavedInfoBarDelegateAndroid() {}

void GeneratedPasswordSavedInfoBarDelegateAndroid::OnInlineLinkClicked() {
  chrome::android::PreferencesLauncher::ShowPasswordSettings();
}

GeneratedPasswordSavedInfoBarDelegateAndroid::
    GeneratedPasswordSavedInfoBarDelegateAndroid()
    : button_label_(l10n_util::GetStringUTF16(IDS_OK)) {
  base::string16 link = l10n_util::GetStringUTF16(IDS_MANAGE_PASSWORDS_LINK);

  size_t offset = 0;
  message_text_ = l10n_util::GetStringFUTF16(
      IDS_MANAGE_PASSWORDS_CONFIRM_GENERATED_TEXT_INFOBAR, link, &offset);
  inline_link_range_ = gfx::Range(offset, offset + link.length());
}

infobars::InfoBarDelegate::InfoBarIdentifier
GeneratedPasswordSavedInfoBarDelegateAndroid::GetIdentifier() const {
  return GENERATED_PASSWORD_SAVED_INFOBAR_DELEGATE_ANDROID;
}

int GeneratedPasswordSavedInfoBarDelegateAndroid::GetIconId() const {
  return IDR_ANDROID_INFOBAR_SAVE_PASSWORD;
}
