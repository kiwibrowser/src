// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/input_method/browser_state_monitor.h"

#include <string>

#include "base/bind.h"
#include "base/macros.h"
#include "chrome/browser/chrome_notification_types.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace input_method {
namespace {

class MockObserver {
 public:
  MockObserver()
      : ui_session_(InputMethodManager::STATE_TERMINATING),
        update_ui_session_count_(0) {}

  void SetState(InputMethodManager::UISessionState new_ui_session) {
    ++update_ui_session_count_;
    ui_session_ = new_ui_session;
  }

  base::Callback<void(InputMethodManager::UISessionState new_ui_session)>
  AsCallback() {
    return base::Bind(&MockObserver::SetState, base::Unretained(this));
  }

  int update_ui_session_count() const { return update_ui_session_count_; }

  InputMethodManager::UISessionState ui_session() const { return ui_session_; }

 private:
  InputMethodManager::UISessionState ui_session_;
  int update_ui_session_count_;

  DISALLOW_COPY_AND_ASSIGN(MockObserver);
};

}  // anonymous namespace

TEST(BrowserStateMonitorLifetimeTest, TestConstruction) {
  MockObserver mock_observer;
  BrowserStateMonitor monitor(mock_observer.AsCallback());

  // Check the initial ui_session_ of the |mock_observer| and |monitor| objects.
  EXPECT_EQ(1, mock_observer.update_ui_session_count());
  EXPECT_EQ(InputMethodManager::STATE_LOGIN_SCREEN, mock_observer.ui_session());
  EXPECT_EQ(InputMethodManager::STATE_LOGIN_SCREEN, monitor.ui_session());
}

namespace {

class BrowserStateMonitorTest :  public testing::Test {
 public:
  BrowserStateMonitorTest()
      : monitor_(mock_observer_.AsCallback()) {
  }

 protected:
  MockObserver mock_observer_;
  BrowserStateMonitor monitor_;

 private:
  DISALLOW_COPY_AND_ASSIGN(BrowserStateMonitorTest);
};

}  // anonymous namespace

TEST_F(BrowserStateMonitorTest, TestObserveLoginUserChanged) {
  EXPECT_EQ(1, mock_observer_.update_ui_session_count());
  monitor_.Observe(chrome::NOTIFICATION_LOGIN_USER_CHANGED,
                   content::NotificationService::AllSources(),
                   content::NotificationService::NoDetails());

  // Check if the ui_session of the |mock_observer_| as well as the |monitor|
  // are
  // both changed.
  EXPECT_EQ(2, mock_observer_.update_ui_session_count());
  EXPECT_EQ(InputMethodManager::STATE_BROWSER_SCREEN,
            mock_observer_.ui_session());
  EXPECT_EQ(InputMethodManager::STATE_BROWSER_SCREEN, monitor_.ui_session());
}

TEST_F(BrowserStateMonitorTest, TestObserveSessionStarted) {
  EXPECT_EQ(1, mock_observer_.update_ui_session_count());
  monitor_.Observe(chrome::NOTIFICATION_SESSION_STARTED,
                   content::NotificationService::AllSources(),
                   content::NotificationService::NoDetails());

  // Check if the state of the |mock_observer_| as well as the |monitor| are
  // both changed.
  EXPECT_EQ(2, mock_observer_.update_ui_session_count());
  EXPECT_EQ(InputMethodManager::STATE_BROWSER_SCREEN,
            mock_observer_.ui_session());
  EXPECT_EQ(InputMethodManager::STATE_BROWSER_SCREEN, monitor_.ui_session());
}

TEST_F(BrowserStateMonitorTest, TestObserveLoginUserChangedThenSessionStarted) {
  EXPECT_EQ(1, mock_observer_.update_ui_session_count());
  monitor_.Observe(chrome::NOTIFICATION_LOGIN_USER_CHANGED,
                   content::NotificationService::AllSources(),
                   content::NotificationService::NoDetails());

  // Check if the state of the |mock_observer_| as well as the |monitor| are
  // both changed.
  EXPECT_EQ(2, mock_observer_.update_ui_session_count());
  EXPECT_EQ(InputMethodManager::STATE_BROWSER_SCREEN,
            mock_observer_.ui_session());
  EXPECT_EQ(InputMethodManager::STATE_BROWSER_SCREEN, monitor_.ui_session());

  monitor_.Observe(chrome::NOTIFICATION_SESSION_STARTED,
                   content::NotificationService::AllSources(),
                   content::NotificationService::NoDetails());

  // The second notification should be nop.
  EXPECT_EQ(2, mock_observer_.update_ui_session_count());
  EXPECT_EQ(InputMethodManager::STATE_BROWSER_SCREEN,
            mock_observer_.ui_session());
  EXPECT_EQ(InputMethodManager::STATE_BROWSER_SCREEN, monitor_.ui_session());
}

TEST_F(BrowserStateMonitorTest, TestObserveScreenLockUnlock) {
  EXPECT_EQ(1, mock_observer_.update_ui_session_count());
  monitor_.Observe(chrome::NOTIFICATION_LOGIN_USER_CHANGED,
                   content::NotificationService::AllSources(),
                   content::NotificationService::NoDetails());
  EXPECT_EQ(2, mock_observer_.update_ui_session_count());
  monitor_.Observe(chrome::NOTIFICATION_SESSION_STARTED,
                   content::NotificationService::AllSources(),
                   content::NotificationService::NoDetails());
  EXPECT_EQ(2, mock_observer_.update_ui_session_count());
  bool locked = true;
  monitor_.Observe(chrome::NOTIFICATION_SCREEN_LOCK_STATE_CHANGED,
                   content::NotificationService::AllSources(),
                   content::Details<bool>(&locked));
  EXPECT_EQ(3, mock_observer_.update_ui_session_count());
  EXPECT_EQ(InputMethodManager::STATE_LOCK_SCREEN, mock_observer_.ui_session());
  EXPECT_EQ(InputMethodManager::STATE_LOCK_SCREEN, monitor_.ui_session());

  locked = false;
  monitor_.Observe(chrome::NOTIFICATION_SCREEN_LOCK_STATE_CHANGED,
                   content::NotificationService::AllSources(),
                   content::Details<bool>(&locked));
  EXPECT_EQ(4, mock_observer_.update_ui_session_count());
  EXPECT_EQ(InputMethodManager::STATE_BROWSER_SCREEN,
            mock_observer_.ui_session());
  EXPECT_EQ(InputMethodManager::STATE_BROWSER_SCREEN, monitor_.ui_session());
}

TEST_F(BrowserStateMonitorTest, TestObserveAppTerminating) {
  EXPECT_EQ(1, mock_observer_.update_ui_session_count());
  monitor_.Observe(chrome::NOTIFICATION_APP_TERMINATING,
                   content::NotificationService::AllSources(),
                   content::NotificationService::NoDetails());

  // Check if the state of the |mock_observer_| as well as the |monitor| are
  // both changed.
  EXPECT_EQ(2, mock_observer_.update_ui_session_count());
  EXPECT_EQ(InputMethodManager::STATE_TERMINATING, mock_observer_.ui_session());
  EXPECT_EQ(InputMethodManager::STATE_TERMINATING, monitor_.ui_session());
}

}  // namespace input_method
}  // namespace chromeos
