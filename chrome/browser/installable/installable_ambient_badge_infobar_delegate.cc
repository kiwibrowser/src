// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/installable/installable_ambient_badge_infobar_delegate.h"

#include <memory>

#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/ui/android/infobars/installable_ambient_badge_infobar.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

InstallableAmbientBadgeInfoBarDelegate::
    ~InstallableAmbientBadgeInfoBarDelegate() {}

// static
void InstallableAmbientBadgeInfoBarDelegate::Create(
    content::WebContents* web_contents,
    base::WeakPtr<Client> weak_client,
    const base::string16& app_name,
    const SkBitmap& primary_icon,
    const GURL& start_url) {
  InfoBarService::FromWebContents(web_contents)
      ->AddInfoBar(std::make_unique<InstallableAmbientBadgeInfoBar>(
          std::unique_ptr<InstallableAmbientBadgeInfoBarDelegate>(
              new InstallableAmbientBadgeInfoBarDelegate(
                  weak_client, app_name, primary_icon, start_url))));
}

void InstallableAmbientBadgeInfoBarDelegate::AddToHomescreen() {
  if (!weak_client_.get())
    return;

  weak_client_->AddToHomescreenFromBadge();
}

const base::string16 InstallableAmbientBadgeInfoBarDelegate::GetMessageText()
    const {
  return l10n_util::GetStringFUTF16(IDS_AMBIENT_BADGE_INSTALL, app_name_);
}

const SkBitmap& InstallableAmbientBadgeInfoBarDelegate::GetPrimaryIcon() const {
  return primary_icon_;
}

InstallableAmbientBadgeInfoBarDelegate::InstallableAmbientBadgeInfoBarDelegate(
    base::WeakPtr<Client> weak_client,
    const base::string16& app_name,
    const SkBitmap& primary_icon,
    const GURL& start_url)
    : infobars::InfoBarDelegate(),
      weak_client_(weak_client),
      app_name_(app_name),
      primary_icon_(primary_icon),
      start_url_(start_url) {}

infobars::InfoBarDelegate::InfoBarIdentifier
InstallableAmbientBadgeInfoBarDelegate::GetIdentifier() const {
  return INSTALLABLE_AMBIENT_BADGE_INFOBAR_DELEGATE;
}

void InstallableAmbientBadgeInfoBarDelegate::InfoBarDismissed() {
  if (!weak_client_.get())
    return;

  weak_client_->BadgeDismissed();
}
