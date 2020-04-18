// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_LAZY_BACKGROUND_PAGE_TEST_UTIL_H_
#define CHROME_BROWSER_EXTENSIONS_LAZY_BACKGROUND_PAGE_TEST_UTIL_H_

#include "chrome/browser/chrome_notification_types.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/test_utils.h"

// Helper class to wait for a lazy background page to load and close again.
class LazyBackgroundObserver {
 public:
  LazyBackgroundObserver()
      : page_created_(extensions::NOTIFICATION_EXTENSION_BACKGROUND_PAGE_READY,
                      content::NotificationService::AllSources()),
        page_closed_(extensions::NOTIFICATION_EXTENSION_HOST_DESTROYED,
                     content::NotificationService::AllSources()) {}
  explicit LazyBackgroundObserver(Profile* profile)
      : page_created_(extensions::NOTIFICATION_EXTENSION_BACKGROUND_PAGE_READY,
                      content::NotificationService::AllSources()),
        page_closed_(extensions::NOTIFICATION_EXTENSION_HOST_DESTROYED,
                     content::Source<Profile>(profile)) {}
  void Wait() {
    page_created_.Wait();
    page_closed_.Wait();
  }

  void WaitUntilLoaded() {
    page_created_.Wait();
  }
  void WaitUntilClosed() {
    page_closed_.Wait();
  }

 private:
  content::WindowedNotificationObserver page_created_;
  content::WindowedNotificationObserver page_closed_;
};

#endif  // CHROME_BROWSER_EXTENSIONS_LAZY_BACKGROUND_PAGE_TEST_UTIL_H_
