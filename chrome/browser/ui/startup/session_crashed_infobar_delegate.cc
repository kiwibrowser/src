// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/startup/session_crashed_infobar_delegate.h"

#include "build/build_config.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/sessions/session_restore.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/url_constants.h"
#include "components/infobars/core/infobar.h"
#include "components/strings/grit/components_chromium_strings.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/dom_storage_context.h"
#include "content/public/browser/storage_partition.h"
#include "ui/base/l10n/l10n_util.h"

// static
void SessionCrashedInfoBarDelegate::Create(Browser* browser) {
  // Assume that if the user is launching incognito they were previously running
  // incognito so that we have nothing to restore from.
  // Also, in ChromeBot tests, there might be a race.  This code appears to be
  // called during shutdown when there is no active WebContents.
  Profile* profile = browser->profile();
  content::WebContents* web_contents =
      browser->tab_strip_model()->GetActiveWebContents();
  if (profile->IsOffTheRecord() || !web_contents)
    return;

  InfoBarService* infobar_service =
      InfoBarService::FromWebContents(web_contents);
  infobar_service->AddInfoBar(infobar_service->CreateConfirmInfoBar(
      std::unique_ptr<ConfirmInfoBarDelegate>(
          new SessionCrashedInfoBarDelegate(profile))));
}

SessionCrashedInfoBarDelegate::SessionCrashedInfoBarDelegate(Profile* profile)
    : ConfirmInfoBarDelegate(),
      accepted_(false),
      profile_(profile) {
}

SessionCrashedInfoBarDelegate::~SessionCrashedInfoBarDelegate() {
  // If the info bar wasn't accepted, it was either dismissed or expired. In
  // that case, session restore won't happen.
  if (!accepted_) {
    content::BrowserContext::GetDefaultStoragePartition(profile_)->
        GetDOMStorageContext()->StartScavengingUnusedSessionStorage();
  }
}

infobars::InfoBarDelegate::InfoBarIdentifier
SessionCrashedInfoBarDelegate::GetIdentifier() const {
  return SESSION_CRASHED_INFOBAR_DELEGATE_MAC_IOS;
}

const gfx::VectorIcon& SessionCrashedInfoBarDelegate::GetVectorIcon() const {
  return kSadTabIcon;
}

base::string16 SessionCrashedInfoBarDelegate::GetMessageText() const {
  return l10n_util::GetStringUTF16(IDS_SESSION_CRASHED_VIEW_MESSAGE);
}

int SessionCrashedInfoBarDelegate::GetButtons() const {
  return BUTTON_OK;
}

base::string16 SessionCrashedInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  DCHECK_EQ(BUTTON_OK, button);
  return l10n_util::GetStringUTF16(IDS_SESSION_CRASHED_VIEW_RESTORE_BUTTON);
}

bool SessionCrashedInfoBarDelegate::Accept() {
  Browser* browser = chrome::FindBrowserWithWebContents(
      InfoBarService::WebContentsFromInfoBar(infobar()));
  SessionRestore::RestoreSessionAfterCrash(browser);
  accepted_ = true;
  return true;
}
