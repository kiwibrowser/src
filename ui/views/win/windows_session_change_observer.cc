// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/win/windows_session_change_observer.h"

#include <wtsapi32.h>

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/observer_list.h"
#include "base/task_scheduler/post_task.h"
#include "ui/gfx/win/singleton_hwnd.h"
#include "ui/gfx/win/singleton_hwnd_observer.h"
#include "ui/views/views_delegate.h"

namespace views {

class WindowsSessionChangeObserver::WtsRegistrationNotificationManager {
 public:
  static WtsRegistrationNotificationManager* GetInstance() {
    return base::Singleton<WtsRegistrationNotificationManager>::get();
  }

  WtsRegistrationNotificationManager() {
    AddSingletonHwndObserver();
  }

  ~WtsRegistrationNotificationManager() {
    RemoveSingletonHwndObserver();
  }

  void AddObserver(WindowsSessionChangeObserver* observer) {
    DCHECK(singleton_hwnd_observer_);
    observer_list_.AddObserver(observer);
  }

  void RemoveObserver(WindowsSessionChangeObserver* observer) {
    observer_list_.RemoveObserver(observer);
  }

 private:
  friend struct base::DefaultSingletonTraits<
      WtsRegistrationNotificationManager>;

  void OnWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
      case WM_WTSSESSION_CHANGE:
        for (WindowsSessionChangeObserver& observer : observer_list_)
          observer.OnSessionChange(wparam);
        break;
      case WM_DESTROY:
        RemoveSingletonHwndObserver();
        break;
    }
  }

  void AddSingletonHwndObserver() {
    DCHECK(!singleton_hwnd_observer_);
    singleton_hwnd_observer_.reset(new gfx::SingletonHwndObserver(
        base::Bind(&WtsRegistrationNotificationManager::OnWndProc,
                   base::Unretained(this))));

    base::OnceClosure wts_register = base::BindOnce(
        base::IgnoreResult(&WTSRegisterSessionNotification),
        gfx::SingletonHwnd::GetInstance()->hwnd(), NOTIFY_FOR_THIS_SESSION);

    // This should probably always be async but it wasn't async in the absence
    // of a ViewsDelegate prior to the migration to TaskScheduler and making it
    // async breaks many unrelated views unittests.
    if (ViewsDelegate::GetInstance()) {
      base::CreateCOMSTATaskRunnerWithTraits({})->PostTask(
          FROM_HERE, std::move(wts_register));
    } else {
      std::move(wts_register).Run();
    }
  }

  void RemoveSingletonHwndObserver() {
    if (!singleton_hwnd_observer_)
      return;

    singleton_hwnd_observer_.reset(nullptr);
    // There is no race condition between this code and the worker thread.
    // RemoveSingletonHwndObserver is only called from two places:
    //   1) Destruction due to Singleton Destruction.
    //   2) WM_DESTROY fired by SingletonHwnd.
    // Under both cases we are in shutdown, which means no other worker threads
    // can be running.
    WTSUnRegisterSessionNotification(gfx::SingletonHwnd::GetInstance()->hwnd());
    for (WindowsSessionChangeObserver& observer : observer_list_)
      observer.ClearCallback();
  }

  base::ObserverList<WindowsSessionChangeObserver, true> observer_list_;
  std::unique_ptr<gfx::SingletonHwndObserver> singleton_hwnd_observer_;

  DISALLOW_COPY_AND_ASSIGN(WtsRegistrationNotificationManager);
};

WindowsSessionChangeObserver::WindowsSessionChangeObserver(
    const WtsCallback& callback)
    : callback_(callback) {
  DCHECK(!callback_.is_null());
  WtsRegistrationNotificationManager::GetInstance()->AddObserver(this);
}

WindowsSessionChangeObserver::~WindowsSessionChangeObserver() {
  ClearCallback();
}

void WindowsSessionChangeObserver::OnSessionChange(WPARAM wparam) {
  callback_.Run(wparam);
}

void WindowsSessionChangeObserver::ClearCallback() {
  if (!callback_.is_null()) {
    callback_.Reset();
    WtsRegistrationNotificationManager::GetInstance()->RemoveObserver(this);
  }
}

}  // namespace views
