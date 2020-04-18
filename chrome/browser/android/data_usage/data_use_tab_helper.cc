// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/data_usage/data_use_tab_helper.h"

#include "base/logging.h"
#include "chrome/browser/android/data_usage/data_use_ui_tab_model.h"
#include "chrome/browser/android/data_usage/data_use_ui_tab_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(DataUseTabHelper);

DataUseTabHelper::~DataUseTabHelper() {}

DataUseTabHelper::DataUseTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
}

void DataUseTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  // TODO(tbansal): Consider the case of a page that provides a navigation bar
  // and loads pages in a sub-frame.
  if (!navigation_handle->IsInMainFrame())
    return;

  // crbug.com/564871: Calling GetPageTransition() may fail if the navigation
  // has not committed.
  if (!navigation_handle->HasCommitted())
    return;

  // Notify the DataUseUITabModel.
  android::DataUseUITabModel* data_use_ui_tab_model =
      android::DataUseUITabModelFactory::GetForBrowserContext(
          Profile::FromBrowserContext(web_contents()->GetBrowserContext()));
  SessionID tab_id = SessionTabHelper::IdForTab(web_contents());
  if (!data_use_ui_tab_model || !tab_id.is_valid())
    return;

  // The last committed navigation entry should correspond to the current
  // navigation. This should not be null since the DidFinishNavigation
  // callback is received for a committed navigation.
  auto* navigation_entry = navigation_handle->GetWebContents()
                               ->GetController()
                               .GetLastCommittedEntry();
  DCHECK(navigation_entry);
  DCHECK_EQ(navigation_handle->GetURL(), navigation_entry->GetURL());
  data_use_ui_tab_model->ReportBrowserNavigation(
      navigation_handle->GetURL(),
      ui::PageTransitionFromInt(navigation_handle->GetPageTransition()), tab_id,
      navigation_entry);
}

void DataUseTabHelper::FrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  // Check if it is a main frame.
  if (render_frame_host->GetParent())
    return;

  // Notify the DataUseUITabModel.
  android::DataUseUITabModel* data_use_ui_tab_model =
      android::DataUseUITabModelFactory::GetForBrowserContext(
          Profile::FromBrowserContext(web_contents()->GetBrowserContext()));
  SessionID tab_id = SessionTabHelper::IdForTab(web_contents());
  if (data_use_ui_tab_model && tab_id.is_valid())
    data_use_ui_tab_model->ReportTabClosure(tab_id);
}
