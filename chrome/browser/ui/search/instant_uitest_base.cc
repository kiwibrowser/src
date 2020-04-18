// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/search/instant_uitest_base.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "chrome/browser/ui/search/search_tab_helper.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/omnibox/browser/omnibox_edit_model.h"
#include "components/omnibox/browser/omnibox_view.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"

InstantUITestBase::InstantUITestBase() = default;

InstantUITestBase::~InstantUITestBase() = default;

OmniboxView* InstantUITestBase::omnibox() {
  return instant_browser()->window()->GetLocationBar()->GetOmniboxView();
}

void InstantUITestBase::FocusOmnibox() {
  // If the omnibox already has focus, just notify SearchTabHelper.
  if (omnibox()->model()->has_focus()) {
    content::WebContents* active_tab =
        instant_browser()->tab_strip_model()->GetActiveWebContents();
    SearchTabHelper::FromWebContents(active_tab)
        ->OmniboxFocusChanged(OMNIBOX_FOCUS_VISIBLE,
                              OMNIBOX_FOCUS_CHANGE_EXPLICIT);
  } else {
    instant_browser()->window()->GetLocationBar()->FocusLocation(false);
  }
}

void InstantUITestBase::SetOmniboxText(const std::string& text) {
  FocusOmnibox();
  omnibox()->SetUserText(base::UTF8ToUTF16(text));
}

void InstantUITestBase::PressEnterAndWaitForNavigation() {
  content::WindowedNotificationObserver nav_observer(
      content::NOTIFICATION_NAV_ENTRY_COMMITTED,
      content::NotificationService::AllSources());
  instant_browser()->window()->GetLocationBar()->AcceptInput();
  nav_observer.Wait();
}

void InstantUITestBase::PressEnterAndWaitForFrameLoad() {
  content::WindowedNotificationObserver nav_observer(
      content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME,
      content::NotificationService::AllSources());
  instant_browser()->window()->GetLocationBar()->AcceptInput();
  nav_observer.Wait();
}

std::string InstantUITestBase::GetOmniboxText() {
  return base::UTF16ToUTF8(omnibox()->GetText());
}
