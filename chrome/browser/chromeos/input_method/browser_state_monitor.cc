// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/input_method/browser_state_monitor.h"

#include "base/logging.h"
#include "chrome/browser/chrome_notification_types.h"
#include "content/public/browser/notification_service.h"
#include "ui/base/ime/chromeos/input_method_delegate.h"
#include "ui/base/ime/chromeos/input_method_util.h"

namespace chromeos {
namespace input_method {

BrowserStateMonitor::BrowserStateMonitor(
    const base::Callback<void(InputMethodManager::UISessionState)>& observer)
    : observer_(observer), ui_session_(InputMethodManager::STATE_LOGIN_SCREEN) {
  notification_registrar_.Add(this,
                              chrome::NOTIFICATION_LOGIN_USER_CHANGED,
                              content::NotificationService::AllSources());
  notification_registrar_.Add(this,
                              chrome::NOTIFICATION_SESSION_STARTED,
                              content::NotificationService::AllSources());
  notification_registrar_.Add(this,
                              chrome::NOTIFICATION_SCREEN_LOCK_STATE_CHANGED,
                              content::NotificationService::AllSources());
  // We should not use ALL_BROWSERS_CLOSING here since logout might be cancelled
  // by JavaScript after ALL_BROWSERS_CLOSING is sent (crosbug.com/11055).
  notification_registrar_.Add(this,
                              chrome::NOTIFICATION_APP_TERMINATING,
                              content::NotificationService::AllSources());

  if (!observer_.is_null())
    observer_.Run(ui_session_);
}

BrowserStateMonitor::~BrowserStateMonitor() {
}

void BrowserStateMonitor::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  const InputMethodManager::UISessionState old_ui_session = ui_session_;
  switch (type) {
    case chrome::NOTIFICATION_APP_TERMINATING: {
      ui_session_ = InputMethodManager::STATE_TERMINATING;
      break;
    }
    case chrome::NOTIFICATION_LOGIN_USER_CHANGED: {
      // The user logged in, but the browser window for user session is not yet
      // ready. An initial input method hasn't been set to the manager.
      // Note that the notification is also sent when Chrome crashes/restarts
      // as of writing, but it might be changed in the future (therefore we need
      // to listen to NOTIFICATION_SESSION_STARTED as well.)
      DVLOG(1) << "Received chrome::NOTIFICATION_LOGIN_USER_CHANGED";
      ui_session_ = InputMethodManager::STATE_BROWSER_SCREEN;
      break;
    }
    case chrome::NOTIFICATION_SESSION_STARTED: {
      // The user logged in, and the browser window for user session is ready.
      // An initial input method has already been set.
      // We should NOT call InitializePrefMembers() here since the notification
      // is sent in the PreProfileInit phase in case when Chrome crashes and
      // restarts.
      DVLOG(1) << "Received chrome::NOTIFICATION_SESSION_STARTED";
      ui_session_ = InputMethodManager::STATE_BROWSER_SCREEN;
      break;
    }
    case chrome::NOTIFICATION_SCREEN_LOCK_STATE_CHANGED: {
      const bool is_screen_locked = *content::Details<bool>(details).ptr();
      ui_session_ = is_screen_locked ? InputMethodManager::STATE_LOCK_SCREEN
                                     : InputMethodManager::STATE_BROWSER_SCREEN;
      break;
    }
    default: {
      NOTREACHED();
      break;
    }
  }

  if (old_ui_session != ui_session_ && !observer_.is_null())
    observer_.Run(ui_session_);

  // Note: browser notifications are sent in the following order.
  //
  // Normal login:
  // 1. chrome::NOTIFICATION_LOGIN_USER_CHANGED is sent.
  // 2. Preferences::NotifyPrefChanged() is called. preload_engines (which
  //    might change the current input method) and current/previous input method
  //    are sent to the manager.
  // 3. chrome::NOTIFICATION_SESSION_STARTED is sent.
  //
  // Chrome crash/restart (after logging in):
  // 1. chrome::NOTIFICATION_LOGIN_USER_CHANGED might be sent.
  // 2. chrome::NOTIFICATION_SESSION_STARTED is sent.
  // 3. Preferences::NotifyPrefChanged() is called. The same things as above
  //    happen.
  //
  // We have to be careful not to overwrite both local and user prefs when
  // preloaded engine is set. Note that it does not work to do nothing in
  // InputMethodChanged() between chrome::NOTIFICATION_LOGIN_USER_CHANGED and
  // chrome::NOTIFICATION_SESSION_STARTED because SESSION_STARTED is sent very
  // early on Chrome crash/restart.
}

}  // namespace input_method
}  // namespace chromeos
