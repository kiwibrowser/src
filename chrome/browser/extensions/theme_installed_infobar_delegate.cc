// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/theme_installed_infobar_delegate.h"

#include <stddef.h>
#include <string>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/grit/generated_resources.h"
#include "components/infobars/core/infobar.h"
#include "content/public/browser/notification_source.h"
#include "extensions/browser/extension_system.h"
#include "ui/base/l10n/l10n_util.h"

// static
void ThemeInstalledInfoBarDelegate::Create(InfoBarService* infobar_service,
                                           ExtensionService* extension_service,
                                           ThemeService* theme_service,
                                           const std::string& theme_name,
                                           const std::string& theme_id,
                                           const std::string& previous_theme_id,
                                           bool previous_using_system_theme) {
  // Create the new infobar.
  std::unique_ptr<infobars::InfoBar> new_infobar(
      infobar_service->CreateConfirmInfoBar(
          std::unique_ptr<ConfirmInfoBarDelegate>(
              new ThemeInstalledInfoBarDelegate(
                  extension_service, theme_service, theme_name, theme_id,
                  previous_theme_id, previous_using_system_theme))));

  // If there's a previous theme infobar, just replace that instead of adding a
  // new one.
  for (size_t i = 0; i < infobar_service->infobar_count(); ++i) {
    infobars::InfoBar* old_infobar = infobar_service->infobar_at(i);
    ThemeInstalledInfoBarDelegate* theme_infobar =
        old_infobar->delegate()->AsThemePreviewInfobarDelegate();
    if (theme_infobar) {
      // If the user installed the same theme twice, ignore the second install
      // and keep the first install info bar, so that they can easily undo to
      // get back the previous theme.
      if (theme_infobar->theme_id_ != theme_id) {
        infobar_service->ReplaceInfoBar(old_infobar, std::move(new_infobar));
        theme_service->OnInfobarDisplayed();
      }
      return;
    }
  }

  // No previous theme infobar, so add this.
  infobar_service->AddInfoBar(std::move(new_infobar));
  theme_service->OnInfobarDisplayed();
}

ThemeInstalledInfoBarDelegate::ThemeInstalledInfoBarDelegate(
    ExtensionService* extension_service,
    ThemeService* theme_service,
    const std::string& theme_name,
    const std::string& theme_id,
    const std::string& previous_theme_id,
    bool previous_using_system_theme)
    : ConfirmInfoBarDelegate(),
      extension_service_(extension_service),
      theme_service_(theme_service),
      theme_name_(theme_name),
      theme_id_(theme_id),
      previous_theme_id_(previous_theme_id),
      previous_using_system_theme_(previous_using_system_theme) {
  registrar_.Add(this, chrome::NOTIFICATION_BROWSER_THEME_CHANGED,
                 content::Source<ThemeService>(theme_service_));
}

ThemeInstalledInfoBarDelegate::~ThemeInstalledInfoBarDelegate() {
  // We don't want any notifications while we're running our destructor.
  registrar_.RemoveAll();

  theme_service_->OnInfobarDestroyed();
}

infobars::InfoBarDelegate::InfoBarIdentifier
ThemeInstalledInfoBarDelegate::GetIdentifier() const {
  return THEME_INSTALLED_INFOBAR_DELEGATE;
}

const gfx::VectorIcon& ThemeInstalledInfoBarDelegate::GetVectorIcon() const {
  return kPaintbrushIcon;
}

ThemeInstalledInfoBarDelegate*
    ThemeInstalledInfoBarDelegate::AsThemePreviewInfobarDelegate() {
  return this;
}

base::string16 ThemeInstalledInfoBarDelegate::GetMessageText() const {
  return l10n_util::GetStringFUTF16(IDS_THEME_INSTALL_INFOBAR_LABEL,
                                    base::UTF8ToUTF16(theme_name_));
}

int ThemeInstalledInfoBarDelegate::GetButtons() const {
  return BUTTON_CANCEL;
}

base::string16 ThemeInstalledInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  DCHECK_EQ(BUTTON_CANCEL, button);
  return l10n_util::GetStringUTF16(IDS_THEME_INSTALL_INFOBAR_UNDO_BUTTON);
}

bool ThemeInstalledInfoBarDelegate::Cancel() {
  if (!previous_theme_id_.empty()) {
    const extensions::Extension* previous_theme =
        extension_service_->GetExtensionById(previous_theme_id_, true);
    if (previous_theme) {
      theme_service_->RevertToTheme(previous_theme);
      // TODO(estade): while we're waiting to close, it would be nice to
      // indicate that the theme is busy reverting.
      return false;  // The theme change will close us.
    }
  }

  if (previous_using_system_theme_)
    theme_service_->UseSystemTheme();
  else
    theme_service_->UseDefaultTheme();
  return false;  // The theme change will close us.
}

void ThemeInstalledInfoBarDelegate::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_BROWSER_THEME_CHANGED, type);
  // If the new theme is different from what this info bar is associated with,
  // close this info bar since it is no longer relevant.
  if (theme_id_ != theme_service_->GetThemeID())
    infobar()->RemoveSelf();
}
