// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/find_in_page_observer.h"

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/ui/find_bar/find_tab_helper.h"
#include "content/public/test/test_utils.h"

namespace ui_test_utils {

FindInPageNotificationObserver::FindInPageNotificationObserver(
    content::WebContents* parent_tab)
    : active_match_ordinal_(-1),
      number_of_matches_(0),
      current_find_request_id_(0),
      seen_(false),
      running_(false) {
  FindTabHelper* find_tab_helper =
      FindTabHelper::FromWebContents(parent_tab);
  current_find_request_id_ = find_tab_helper->current_find_request_id();
  registrar_.Add(this, chrome::NOTIFICATION_FIND_RESULT_AVAILABLE,
                 content::Source<content::WebContents>(parent_tab));
}

FindInPageNotificationObserver::~FindInPageNotificationObserver() {}

void FindInPageNotificationObserver::Wait() {
  if (seen_)
    return;
  running_ = true;
  message_loop_runner_ = new content::MessageLoopRunner;
  message_loop_runner_->Run();
}

void FindInPageNotificationObserver::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_FIND_RESULT_AVAILABLE, type);

  content::Details<FindNotificationDetails> find_details(details);
  if (find_details->request_id() != current_find_request_id_)
    return;

  // We get multiple responses and one of those will contain the ordinal.
  // This message comes to us before the final update is sent.
  if (find_details->active_match_ordinal() > -1) {
    active_match_ordinal_ = find_details->active_match_ordinal();
    selection_rect_ = find_details->selection_rect();
  }
  if (find_details->final_update()) {
    number_of_matches_ = find_details->number_of_matches();
    seen_ = true;
    if (running_) {
      running_ = false;
      message_loop_runner_->Quit();
    }
  } else {
    DVLOG(1) << "Ignoring, since we only care about the final message";
  }
}

}  // namespace ui_test_utils

