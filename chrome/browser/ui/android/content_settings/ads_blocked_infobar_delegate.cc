// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/content_settings/ads_blocked_infobar_delegate.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/android/android_theme_resources.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/subresource_filter/chrome_subresource_filter_client.h"
#include "chrome/browser/ui/android/infobars/ads_blocked_infobar.h"
#include "chrome/grit/generated_resources.h"
#include "components/infobars/core/infobar.h"
#include "components/strings/grit/components_strings.h"
#include "components/subresource_filter/core/browser/subresource_filter_constants.h"
#include "components/subresource_filter/core/browser/subresource_filter_features.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"

// static
void AdsBlockedInfobarDelegate::Create(InfoBarService* infobar_service) {
  infobar_service->AddInfoBar(std::make_unique<AdsBlockedInfoBar>(
      base::WrapUnique(new AdsBlockedInfobarDelegate())));
}

AdsBlockedInfobarDelegate::AdsBlockedInfobarDelegate()
    : ConfirmInfoBarDelegate() {}

AdsBlockedInfobarDelegate::~AdsBlockedInfobarDelegate() {}

base::string16 AdsBlockedInfobarDelegate::GetExplanationText() const {
  return l10n_util::GetStringUTF16(IDS_BLOCKED_ADS_PROMPT_EXPLANATION);
}

// The experimental UI includes the permission and its associated UI. Without it
// we can't say "Always". Allowing ads will only be scoped to the tab.
base::string16 AdsBlockedInfobarDelegate::GetToggleText() const {
  return base::FeatureList::IsEnabled(
             subresource_filter::kSafeBrowsingSubresourceFilterExperimentalUI)
             ? l10n_util::GetStringUTF16(IDS_ALWAYS_ALLOW_ADS)
             : l10n_util::GetStringUTF16(IDS_ALLOW_ADS);
}

infobars::InfoBarDelegate::InfoBarIdentifier
AdsBlockedInfobarDelegate::GetIdentifier() const {
  return ADS_BLOCKED_INFOBAR_DELEGATE_ANDROID;
}

int AdsBlockedInfobarDelegate::GetIconId() const {
  return IDR_ANDROID_INFOBAR_BLOCKED_POPUPS;
}

base::string16 AdsBlockedInfobarDelegate::GetMessageText() const {
  return l10n_util::GetStringUTF16(IDS_BLOCKED_ADS_INFOBAR_MESSAGE);
}

int AdsBlockedInfobarDelegate::GetButtons() const {
  return BUTTON_OK | BUTTON_CANCEL;
}

base::string16 AdsBlockedInfobarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  if (button == BUTTON_OK)
    return l10n_util::GetStringUTF16(IDS_OK);
  return l10n_util::GetStringUTF16(IDS_APP_MENU_RELOAD);
}

bool AdsBlockedInfobarDelegate::Cancel() {
  content::WebContents* web_contents =
      InfoBarService::WebContentsFromInfoBar(infobar());
  ChromeSubresourceFilterClient::FromWebContents(web_contents)
      ->OnReloadRequested();
  return true;
}

GURL AdsBlockedInfobarDelegate::GetLinkURL() const {
  DCHECK(infobar_expanded_);
  return GURL(subresource_filter::kLearnMoreLink);
}

bool AdsBlockedInfobarDelegate::LinkClicked(WindowOpenDisposition disposition) {
  if (infobar_expanded_) {
    DCHECK_EQ(disposition, WindowOpenDisposition::NEW_FOREGROUND_TAB);
    infobar()->owner()->OpenURL(GetLinkURL(),
                                WindowOpenDisposition::NEW_FOREGROUND_TAB);
    ChromeSubresourceFilterClient::LogAction(kActionClickedLearnMore);
  } else {
    ChromeSubresourceFilterClient::LogAction(kActionDetailsShown);
    infobar_expanded_ = true;
  }
  // Returning false keeps the infobar up, which is the behavior we want in all
  // cases. If the user is navigating via a new foreground tab we do not want
  // the infobar going away on the tab (and therefore invoking our smart-hiding
  // logic).
  return false;
}
