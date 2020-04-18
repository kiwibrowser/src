// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_EXCLUSIVE_ACCESS_FULLSCREEN_CONTROLLER_TEST_H_
#define CHROME_BROWSER_UI_EXCLUSIVE_ACCESS_FULLSCREEN_CONTROLLER_TEST_H_

#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_bubble_hide_callback.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_bubble_type.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/test_utils.h"

#if defined(OS_MACOSX)
#include "ui/base/test/scoped_fake_nswindow_fullscreen.h"
#endif

namespace base {
class TickClock;
}  // namespace base

// Observer for NOTIFICATION_FULLSCREEN_CHANGED notifications.
class FullscreenNotificationObserver
    : public content::WindowedNotificationObserver {
 public:
  FullscreenNotificationObserver() : WindowedNotificationObserver(
      chrome::NOTIFICATION_FULLSCREEN_CHANGED,
      content::NotificationService::AllSources()) {}
 protected:
  DISALLOW_COPY_AND_ASSIGN(FullscreenNotificationObserver);
};

// Observer for NOTIFICATION_MOUSE_LOCK_CHANGED notifications.
class MouseLockNotificationObserver
    : public content::WindowedNotificationObserver {
 public:
  MouseLockNotificationObserver() : WindowedNotificationObserver(
      chrome::NOTIFICATION_MOUSE_LOCK_CHANGED,
      content::NotificationService::AllSources()) {}
 protected:
  DISALLOW_COPY_AND_ASSIGN(MouseLockNotificationObserver);
};

// Test fixture with convenience functions for fullscreen, keyboard lock, and
// mouse lock.
class FullscreenControllerTest : public InProcessBrowserTest {
 protected:
  FullscreenControllerTest();
  ~FullscreenControllerTest() override;

  void SetUpOnMainThread() override;
  void TearDownOnMainThread() override;

  void RequestKeyboardLock(bool esc_key_locked);
  void RequestToLockMouse(bool user_gesture,
                          bool last_unlocked_by_target);
  void SetWebContentsGrantedSilentMouseLockPermission();
  void CancelKeyboardLock();
  void LostMouseLock();
  bool SendEscapeToFullscreenController();
  bool IsFullscreenForBrowser();
  bool IsWindowFullscreenForTabOrPending();
  ExclusiveAccessBubbleType GetExclusiveAccessBubbleType();
  bool IsFullscreenBubbleDisplayed();
  void GoBack();
  void Reload();
  void SetPrivilegedFullscreen(bool is_privileged);
  void EnterActiveTabFullscreen();
  void EnterExtensionInitiatedFullscreen();

  static const char kFullscreenKeyboardLockHTML[];
  static const char kFullscreenMouseLockHTML[];
  FullscreenController* GetFullscreenController();
  ExclusiveAccessManager* GetExclusiveAccessManager();

  void OnBubbleHidden(
      std::vector<ExclusiveAccessBubbleHideReason>* reason_recorder,
      ExclusiveAccessBubbleHideReason);

  void SetEscRepeatWindowLength(base::TimeDelta esc_repeat_window);

  void SetEscRepeatThresholdReachedCallback(base::OnceClosure callback);

  void SetEscRepeatTestTickClock(const base::TickClock* tick_clock_for_test);

  int InitialBubbleDelayMs() const;

  std::vector<ExclusiveAccessBubbleHideReason>
      mouse_lock_bubble_hide_reason_recorder_;

  std::vector<ExclusiveAccessBubbleHideReason>
      keyboard_lock_bubble_hide_reason_recorder_;

 private:
  void ToggleTabFullscreen_Internal(bool enter_fullscreen,
                                    bool retry_until_success);

  base::test::ScopedFeatureList scoped_feature_list_;

  base::WeakPtrFactory<FullscreenControllerTest> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(FullscreenControllerTest);
};

#endif  // CHROME_BROWSER_UI_EXCLUSIVE_ACCESS_FULLSCREEN_CONTROLLER_TEST_H_
