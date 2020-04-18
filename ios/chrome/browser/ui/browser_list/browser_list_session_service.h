// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_LIST_SESSION_SERVICE_H_
#define IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_LIST_SESSION_SERVICE_H_

#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"

// BrowserListSessionService allow saving and restoring saved sessions.
class BrowserListSessionService : public KeyedService {
 public:
  BrowserListSessionService() = default;
  ~BrowserListSessionService() override = default;

  // Restores the saved session on the current thread. Returns whether the
  // load was successful or not.
  virtual bool RestoreSession() = 0;

  // Schedules deletion of the file containing the last session.
  virtual void ScheduleLastSessionDeletion() = 0;

  // Schedules recording the session on a background thread. If |immediately|
  // is false, then the session is recorded after a delay. In all cases, if
  // another session recording was previously scheduled with a delay, that will
  // be cancelled.
  virtual void ScheduleSaveSession(bool immediately) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(BrowserListSessionService);
};

#endif  // IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_LIST_SESSION_SERVICE_H_
