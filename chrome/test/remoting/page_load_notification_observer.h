// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_REMOTING_PAGE_LOAD_NOTIFICATION_OBSERVER_H_
#define CHROME_TEST_REMOTING_PAGE_LOAD_NOTIFICATION_OBSERVER_H_

#include "base/macros.h"
#include "content/public/test/test_utils.h"
#include "url/gurl.h"

namespace remoting {

// A PageLoadNotificationObserver allows code to wait until a give URL is loaded
// in any tab in any browser window, i.e. NotificationService::AllSources().
// This simple pattern is not easy to implement using
// WindowedNotificationObserver because we need to bind the observer object
// in the callback but we also need to provide the callback when constructing
// the observer object.
class PageLoadNotificationObserver
    : public content::WindowedNotificationObserver {
 public:
  explicit PageLoadNotificationObserver(const GURL& target);

  ~PageLoadNotificationObserver() override;

  void set_ignore_url_parameters(bool ignore_url_parameters) {
    ignore_url_parameters_ = ignore_url_parameters;
  }

  // WindowedNotificationObserver caches the source parameter whenever
  // a page load occurs. Since we're only interested in the one for which
  // IsTargetLoaded returns true, we have to cache it separately.
  const content::NotificationSource& matched_source() {
    return matched_source_;
  }

 private:
  bool IsTargetLoaded();

  GURL target_;
  bool ignore_url_parameters_;
  content::NotificationSource matched_source_;

  DISALLOW_COPY_AND_ASSIGN(PageLoadNotificationObserver);
};

}  // namespace remoting

#endif  // CHROME_TEST_REMOTING_PAGE_LOAD_NOTIFICATION_OBSERVER_H_
