// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/infobars/infobar_service.h"

#include "base/command_line.h"
#include "chrome/common/render_messages.h"
#include "components/infobars/core/infobar.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "ui/base/page_transition_types.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(InfoBarService);

// static
infobars::InfoBarDelegate::NavigationDetails
    InfoBarService::NavigationDetailsFromLoadCommittedDetails(
        const content::LoadCommittedDetails& details) {
  infobars::InfoBarDelegate::NavigationDetails navigation_details;
  navigation_details.entry_id = details.entry->GetUniqueID();
  navigation_details.is_navigation_to_different_page =
      details.is_navigation_to_different_page();
  navigation_details.did_replace_entry = details.did_replace_entry;
  const ui::PageTransition transition = details.entry->GetTransitionType();
  navigation_details.is_reload =
      ui::PageTransitionCoreTypeIs(transition, ui::PAGE_TRANSITION_RELOAD);
  navigation_details.is_redirect = ui::PageTransitionIsRedirect(transition);
  return navigation_details;
}

// static
content::WebContents* InfoBarService::WebContentsFromInfoBar(
    infobars::InfoBar* infobar) {
  if (!infobar || !infobar->owner())
    return NULL;
  InfoBarService* infobar_service =
      static_cast<InfoBarService*>(infobar->owner());
  return infobar_service->web_contents();
}

InfoBarService::InfoBarService(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      ignore_next_reload_(false) {
  DCHECK(web_contents);
  // Infobar animations cause viewport resizes. Disable them for automated
  // tests, since they could lead to flakiness.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableAutomation))
    set_animations_enabled(false);
}

InfoBarService::~InfoBarService() {
  ShutDown();
}

int InfoBarService::GetActiveEntryID() {
  content::NavigationEntry* active_entry =
      web_contents()->GetController().GetActiveEntry();
  return active_entry ? active_entry->GetUniqueID() : 0;
}

// InfoBarService::CreateConfirmInfoBar() is implemented in platform-specific
// files.

void InfoBarService::RenderProcessGone(base::TerminationStatus status) {
  RemoveAllInfoBars(true);
}

void InfoBarService::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  ignore_next_reload_ = false;
}

void InfoBarService::NavigationEntryCommitted(
    const content::LoadCommittedDetails& load_details) {
  const bool ignore = ignore_next_reload_ &&
      ui::PageTransitionCoreTypeIs(load_details.entry->GetTransitionType(),
                                   ui::PAGE_TRANSITION_RELOAD);
  ignore_next_reload_ = false;
  if (!ignore)
    OnNavigation(NavigationDetailsFromLoadCommittedDetails(load_details));
}

void InfoBarService::WebContentsDestroyed() {
  // The WebContents is going away; be aggressively paranoid and delete
  // ourselves lest other parts of the system attempt to add infobars or use
  // us otherwise during the destruction.
  web_contents()->RemoveUserData(UserDataKey());
  // That was the equivalent of "delete this". This object is now destroyed;
  // returning from this function is the only safe thing to do.
}

void InfoBarService::OpenURL(const GURL& url,
                             WindowOpenDisposition disposition) {
  // A normal user click on an infobar URL will result in a CURRENT_TAB
  // disposition; turn that into a NEW_FOREGROUND_TAB so that we don't end up
  // smashing the page the user is looking at.
  web_contents()->OpenURL(
      content::OpenURLParams(url, content::Referrer(),
                             (disposition == WindowOpenDisposition::CURRENT_TAB)
                                 ? WindowOpenDisposition::NEW_FOREGROUND_TAB
                                 : disposition,
                             ui::PAGE_TRANSITION_LINK, false));
}
