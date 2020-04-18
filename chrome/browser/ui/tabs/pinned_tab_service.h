// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TABS_PINNED_TAB_SERVICE_H_
#define CHROME_BROWSER_UI_TABS_PINNED_TAB_SERVICE_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/scoped_observer.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class Profile;

// PinnedTabService is responsible for updating preferences with the set of
// pinned tabs to restore at startup. PinnedTabService listens for the
// appropriate set of notifications to know it should update preferences.
class PinnedTabService : public content::NotificationObserver,
                         public BrowserListObserver,
                         public KeyedService {
 public:
  explicit PinnedTabService(Profile* profile);
  ~PinnedTabService() override;

 private:
  // content::NotificationObserver.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // BrowserListObserver:
  void OnBrowserClosing(Browser* browser) override;
  void OnBrowserRemoved(Browser* browser) override;

  Profile* profile_;

  // True if we should save the pinned tabs when a browser window closes or the
  // user exits the application.
  bool save_pinned_tabs_;

  // True if there is at least one normal browser for our profile.
  bool has_normal_browser_;

  content::NotificationRegistrar registrar_;

  ScopedObserver<BrowserList, BrowserListObserver> browser_list_observer_;

  DISALLOW_COPY_AND_ASSIGN(PinnedTabService);
};

#endif  // CHROME_BROWSER_UI_TABS_PINNED_TAB_SERVICE_H_
