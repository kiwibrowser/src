// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/sync/one_click_signin_sync_observer.h"

#include <string>

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/signin_promo.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "components/browser_sync/profile_sync_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"

namespace {

void CloseTab(content::WebContents* tab) {
  content::WebContentsDelegate* tab_delegate = tab->GetDelegate();
  if (tab_delegate)
    tab_delegate->CloseContents(tab);
}

}  // namespace


OneClickSigninSyncObserver::OneClickSigninSyncObserver(
    content::WebContents* web_contents,
    const GURL& continue_url)
    : content::WebContentsObserver(web_contents),
      continue_url_(continue_url),
      weak_ptr_factory_(this) {
  DCHECK(!continue_url_.is_empty());

  browser_sync::ProfileSyncService* sync_service = GetSyncService(web_contents);
  if (sync_service) {
    sync_service->AddObserver(this);
  } else {
    LoadContinueUrl();
    // Post a task rather than calling |delete this| here, so that the
    // destructor is not called directly from the constructor. Note that it's
    // important to pass a weak pointer rather than base::Unretained(this)
    // because it's possible for e.g. WebContentsDestroyed() to be called
    // before this task has a chance to run.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&OneClickSigninSyncObserver::DeleteObserver,
                                  weak_ptr_factory_.GetWeakPtr()));
  }
}

OneClickSigninSyncObserver::~OneClickSigninSyncObserver() {}

void OneClickSigninSyncObserver::WebContentsDestroyed() {
  browser_sync::ProfileSyncService* sync_service =
      GetSyncService(web_contents());
  if (sync_service)
    sync_service->RemoveObserver(this);

  delete this;
}

void OneClickSigninSyncObserver::OnStateChanged(syncer::SyncService* sync) {
  browser_sync::ProfileSyncService* sync_service =
      GetSyncService(web_contents());

  // At this point, the sign-in process is complete, and control has been handed
  // back to the sync engine. Close the gaia sign in tab if the |continue_url_|
  // contains the |auto_close| parameter. Otherwise, wait for sync setup to
  // complete and then navigate to the |continue_url_|.
  if (signin::IsAutoCloseEnabledInURL(continue_url_)) {
    // Close the Gaia sign-in tab via a task to make sure we aren't in the
    // middle of any WebUI handler code.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&CloseTab, base::Unretained(web_contents())));
  } else {
    if (sync_service->IsFirstSetupInProgress()) {
      // Sync setup has not completed yet. Wait for it to complete.
      return;
    }

    if (sync_service->IsSyncActive() &&
        signin::GetAccessPointForPromoURL(continue_url_) !=
            signin_metrics::AccessPoint::ACCESS_POINT_SETTINGS) {
      // TODO(isherman): Having multiple settings pages open can cause issues
      // redirecting after Sync is set up: http://crbug.com/355885
      LoadContinueUrl();
    }
  }

  sync_service->RemoveObserver(this);
  delete this;
}

void OneClickSigninSyncObserver::LoadContinueUrl() {
  web_contents()->GetController().LoadURL(
      continue_url_,
      content::Referrer(),
      ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
      std::string());
}

browser_sync::ProfileSyncService* OneClickSigninSyncObserver::GetSyncService(
    content::WebContents* web_contents) {
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  return ProfileSyncServiceFactory::GetForProfile(profile);
}

// static
void OneClickSigninSyncObserver::DeleteObserver(
    base::WeakPtr<OneClickSigninSyncObserver> observer) {
  if (observer)
    delete observer.get();
}
