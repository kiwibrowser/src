// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_SYNC_ONE_CLICK_SIGNIN_SYNC_OBSERVER_H_
#define CHROME_BROWSER_UI_SYNC_ONE_CLICK_SIGNIN_SYNC_OBSERVER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/sync/driver/sync_service_observer.h"
#include "content/public/browser/web_contents_observer.h"
#include "url/gurl.h"

namespace browser_sync {
class ProfileSyncService;
}

namespace content {
class WebContents;
}

class OneClickSigninSyncObserver : public content::WebContentsObserver,
                                   public syncer::SyncServiceObserver {
 public:
  // Waits for Sync to be initialized, then navigates the |web_contents| to the
  // |continue_url|.  Instances of this class delete themselves once the job is
  // done.
  OneClickSigninSyncObserver(content::WebContents* web_contents,
                             const GURL& continue_url);

 protected:
  // Exposed for testing.
  ~OneClickSigninSyncObserver() override;

 private:
  // content::WebContentsObserver:
  void WebContentsDestroyed() override;

  // syncer::SyncServiceObserver:
  void OnStateChanged(syncer::SyncService* sync) override;

  // Loads the |continue_url_| in the |web_contents()|.
  void LoadContinueUrl();

  // Returns the ProfileSyncService associated with the |web_contents|.
  // The returned value may be NULL.
  browser_sync::ProfileSyncService* GetSyncService(
      content::WebContents* web_contents);

  // Deletes the |observer|. Intended to be used as a callback for base::Bind.
  static void DeleteObserver(
      base::WeakPtr<OneClickSigninSyncObserver> observer);

  // The URL to redirect to once Sync is initialized.
  const GURL continue_url_;

  base::WeakPtrFactory<OneClickSigninSyncObserver> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OneClickSigninSyncObserver);
};

#endif  // CHROME_BROWSER_UI_SYNC_ONE_CLICK_SIGNIN_SYNC_OBSERVER_H_
