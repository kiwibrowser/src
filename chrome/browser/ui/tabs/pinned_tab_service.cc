// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/pinned_tab_service.h"

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/pinned_tab_codec.h"
#include "content/public/browser/notification_service.h"

namespace {

// Returns true if |browser| is the only normal (tabbed) browser for |browser|'s
// profile (across all desktops).
bool IsOnlyNormalBrowser(Browser* browser) {
  for (auto* b : *BrowserList::GetInstance()) {
    if (b != browser && b->is_type_tabbed() &&
        b->profile() == browser->profile()) {
      return false;
    }
  }
  return true;
}

// Returns true if there's at lease one tabbed browser associated with
// |profile|.
bool BrowserListHasNormalBrowser(Profile* profile) {
  for (auto* b : *BrowserList::GetInstance()) {
    if (b->is_type_tabbed() && b->profile() == profile)
      return true;
  }
  return false;
}

}  // namespace

PinnedTabService::PinnedTabService(Profile* profile)
    : profile_(profile),
      save_pinned_tabs_(true),
      has_normal_browser_(false),
      browser_list_observer_(this) {
  registrar_.Add(this, chrome::NOTIFICATION_BROWSER_OPENED,
                 content::NotificationService::AllBrowserContextsAndSources());
  registrar_.Add(this, chrome::NOTIFICATION_CLOSE_ALL_BROWSERS_REQUEST,
                 content::NotificationService::AllSources());
  registrar_.Add(this, chrome::NOTIFICATION_TAB_ADDED,
                 content::NotificationService::AllSources());
  browser_list_observer_.Add(BrowserList::GetInstance());
}

PinnedTabService::~PinnedTabService() {}

void PinnedTabService::Observe(int type,
                               const content::NotificationSource& source,
                               const content::NotificationDetails& details) {
  // Saving of tabs happens when saving is enabled, and when either the user
  // exits the application or closes the last browser window.
  // Saving is disabled when the user exits the application to prevent the
  // pin state of all the open browsers being overwritten by the state of the
  // last browser window to close.
  // Saving is re-enabled when a browser window or tab is opened again.
  // Note, cancelling a shutdown (via onbeforeunload) will not re-enable pinned
  // tab saving immediately, to prevent the following situation:
  //   * two windows are open, one with pinned tabs
  //   * user exits
  //   * pinned tabs are saved
  //   * window with pinned tabs is closed
  //   * other window blocks close with onbeforeunload
  //   * user saves work, etc. then closes the window
  //   * pinned tabs are saved, without the window with the pinned tabs,
  //     over-writing the correct state.
  // Saving is re-enabled if a new tab or window is opened.
  switch (type) {
    case chrome::NOTIFICATION_BROWSER_OPENED: {
      Browser* browser = content::Source<Browser>(source).ptr();
      if (!has_normal_browser_ && browser->is_type_tabbed() &&
          browser->profile() == profile_) {
        has_normal_browser_ = true;
      }
      save_pinned_tabs_ = true;
      break;
    }

    case chrome::NOTIFICATION_TAB_ADDED: {
      save_pinned_tabs_ = true;
      break;
    }

    case chrome::NOTIFICATION_CLOSE_ALL_BROWSERS_REQUEST: {
      if (has_normal_browser_ && save_pinned_tabs_) {
        PinnedTabCodec::WritePinnedTabs(profile_);
        save_pinned_tabs_ = false;
      }
      break;
    }

    default:
      NOTREACHED();
  }
}

void PinnedTabService::OnBrowserClosing(Browser* browser) {
  if (has_normal_browser_ && save_pinned_tabs_ &&
      browser->profile() == profile_ && IsOnlyNormalBrowser(browser)) {
    has_normal_browser_ = false;
    PinnedTabCodec::WritePinnedTabs(profile_);
  }
}

void PinnedTabService::OnBrowserRemoved(Browser* browser) {
  if (!browser->is_type_tabbed() || browser->profile() != profile_)
    return;

  if (save_pinned_tabs_ && has_normal_browser_ &&
      !BrowserListHasNormalBrowser(browser->profile())) {
    // This happens when user closes each tabs manually via the close button on
    // them. In this case OnBrowserClosing() above is not called. This causes
    // pinned tabs to repopen on the next startup. So we should call
    // WritePinnedTab() to clear the data.
    // http://crbug.com/71939
    has_normal_browser_ = false;
    PinnedTabCodec::WritePinnedTabs(profile_);
  }
}
