// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/load_complete_listener.h"

#include "base/logging.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"

LoadCompleteListener::LoadCompleteListener(Delegate* delegate)
    : delegate_(delegate) {
  DCHECK(delegate);
  // Register for notification of when initial page load is complete to ensure
  // that we wait until start-up is complete before calling the callback.
  registrar_.Add(this, content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME,
                 content::NotificationService::AllSources());
}

LoadCompleteListener::~LoadCompleteListener() {
  if (registrar_.IsRegistered(this,
      content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME,
      content::NotificationService::AllSources())) {
    registrar_.Remove(this, content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME,
                      content::NotificationService::AllSources());
  }
}

void LoadCompleteListener::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME, type);

  delegate_->OnLoadCompleted();
  registrar_.Remove(this, content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME,
                    content::NotificationService::AllSources());
}
