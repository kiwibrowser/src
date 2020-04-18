// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/translate/core/browser/options_menu_model.h"

#include "base/metrics/histogram.h"
#include "build/build_config.h"
#include "components/strings/grit/components_strings.h"
#include "components/translate/core/browser/translate_driver.h"
#include "components/translate/core/browser/translate_infobar_delegate.h"
#include "ui/base/l10n/l10n_util.h"

namespace translate {

namespace {

const char kAboutGoogleTranslateURL[] =
#if defined(OS_CHROMEOS)
    "https://support.google.com/chromebook/?p=ib_translation_bar";
#else
    "https://support.google.com/chrome/?p=ib_translation_bar";
#endif

}  // namespace

OptionsMenuModel::OptionsMenuModel(TranslateInfoBarDelegate* translate_delegate)
    : ui::SimpleMenuModel(this),
      translate_infobar_delegate_(translate_delegate) {
  // |translate_delegate| must already be owned.
  DCHECK(translate_infobar_delegate_->GetTranslateDriver());

  base::string16 original_language =
      translate_delegate->original_language_name();
  base::string16 target_language = translate_delegate->target_language_name();

  bool autodetermined_source_language =
      (translate_delegate->original_language_code() ==
       translate::kUnknownLanguageCode);

  // Populate the menu.
  // Incognito mode does not get any preferences related items.
  if (!translate_delegate->is_off_the_record()) {
    if (!autodetermined_source_language) {
      AddCheckItem(ALWAYS_TRANSLATE,
          l10n_util::GetStringFUTF16(IDS_TRANSLATE_INFOBAR_OPTIONS_ALWAYS,
                                     original_language, target_language));
      AddCheckItem(NEVER_TRANSLATE_LANGUAGE,
                   l10n_util::GetStringFUTF16(
                       IDS_TRANSLATE_INFOBAR_OPTIONS_NEVER_TRANSLATE_LANG,
                       original_language));
    }
    AddCheckItem(NEVER_TRANSLATE_SITE,
                 l10n_util::GetStringUTF16(
                     IDS_TRANSLATE_INFOBAR_OPTIONS_NEVER_TRANSLATE_SITE));
    AddSeparator(ui::NORMAL_SEPARATOR);
  }
  if (!autodetermined_source_language) {
    AddItem(REPORT_BAD_DETECTION,
        l10n_util::GetStringFUTF16(IDS_TRANSLATE_INFOBAR_OPTIONS_REPORT_ERROR,
                                   original_language));
  }
  AddItemWithStringId(ABOUT_TRANSLATE, IDS_TRANSLATE_INFOBAR_OPTIONS_ABOUT);
}

OptionsMenuModel::~OptionsMenuModel() {
}

bool OptionsMenuModel::IsCommandIdChecked(int command_id) const {
  switch (command_id) {
    case NEVER_TRANSLATE_LANGUAGE:
      return !translate_infobar_delegate_->IsTranslatableLanguageByPrefs();

    case NEVER_TRANSLATE_SITE:
      return translate_infobar_delegate_->IsSiteBlacklisted();

    case ALWAYS_TRANSLATE:
      return translate_infobar_delegate_->ShouldAlwaysTranslate();

    default:
      NOTREACHED() << "Invalid command_id from menu";
      break;
  }
  return false;
}

bool OptionsMenuModel::IsCommandIdEnabled(int command_id) const {
  switch (command_id) {
    case NEVER_TRANSLATE_LANGUAGE:
    case NEVER_TRANSLATE_SITE:
      return !translate_infobar_delegate_->ShouldAlwaysTranslate();

    case ALWAYS_TRANSLATE:
      return (translate_infobar_delegate_->IsTranslatableLanguageByPrefs() &&
          !translate_infobar_delegate_->IsSiteBlacklisted());

    default:
      break;
  }
  return true;
}

void OptionsMenuModel::ExecuteCommand(int command_id, int event_flags) {
  switch (command_id) {
    case NEVER_TRANSLATE_LANGUAGE:
      translate_infobar_delegate_->ToggleTranslatableLanguageByPrefs();
      break;

    case NEVER_TRANSLATE_SITE:
      translate_infobar_delegate_->ToggleSiteBlacklist();
      break;

    case ALWAYS_TRANSLATE:
      translate_infobar_delegate_->ToggleAlwaysTranslate();
      break;

    case REPORT_BAD_DETECTION:
      translate_infobar_delegate_->ReportLanguageDetectionError();
      break;

    case ABOUT_TRANSLATE: {
      TranslateDriver* translate_driver =
          translate_infobar_delegate_->GetTranslateDriver();
      if (translate_driver)
        translate_driver->OpenUrlInNewTab(GURL(kAboutGoogleTranslateURL));
      break;
    }

    default:
      NOTREACHED() << "Invalid command id from menu.";
      break;
  }
}

}  // namespace translate
