// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_BASE_FIND_IN_PAGE_OBSERVER_H_
#define CHROME_TEST_BASE_FIND_IN_PAGE_OBSERVER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "ui/gfx/geometry/rect.h"

namespace content {
class MessageLoopRunner;
class WebContents;
}

namespace ui_test_utils {

// FindInPageNotificationObserver allows blocking UI thread until find results
// are available. Typical usage:
// FindInPageWchar();
// FindInPageNotificationObserver observer(tab);
// observer.Wait();

// Always construct FindInPageNotificationObserver AFTER initiating the search.
// It captures the current search ID in constructor and waits for it only.
class FindInPageNotificationObserver : public content::NotificationObserver {
 public:
  explicit FindInPageNotificationObserver(content::WebContents* parent_tab);
  ~FindInPageNotificationObserver() override;

  void Wait();

  int active_match_ordinal() const { return active_match_ordinal_; }
  int number_of_matches() const { return number_of_matches_; }
  gfx::Rect selection_rect() const { return selection_rect_; }

 private:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  content::NotificationRegistrar registrar_;
  // We will at some point (before final update) be notified of the ordinal and
  // we need to preserve it so we can send it later.
  int active_match_ordinal_;
  int number_of_matches_;
  gfx::Rect selection_rect_;
  // The id of the current find request, obtained from WebContents. Allows us
  // to monitor when the search completes.
  int current_find_request_id_;
  scoped_refptr<content::MessageLoopRunner> message_loop_runner_;

  bool seen_; // true after transition to expected state has been seen
  bool running_; // indicates whether message loop is running

  DISALLOW_COPY_AND_ASSIGN(FindInPageNotificationObserver);
};

}  // namespace ui_test_utils

#endif  // CHROME_TEST_BASE_FIND_IN_PAGE_OBSERVER_H_
