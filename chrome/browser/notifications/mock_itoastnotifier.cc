// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/mock_itoastnotifier.h"

#include <wrl/client.h>

#include "chrome/browser/notifications/notification_launch_id.h"
#include "chrome/browser/notifications/notification_platform_bridge_win.h"
#include "chrome/browser/notifications/notification_platform_bridge_win_util.h"

namespace winui = ABI::Windows::UI;

MockIToastNotifier::MockIToastNotifier() = default;
MockIToastNotifier::~MockIToastNotifier() = default;

void MockIToastNotifier::SetNotificationShownCallback(
    const base::RepeatingCallback<void(const NotificationLaunchId& launch_id)>&
        callback) {
  notification_shown_callback_ = callback;
}

HRESULT MockIToastNotifier::QueryInterface(REFIID riid, void** ppvObject) {
  if (riid == ABI::Windows::UI::Notifications::IID_IToastNotifier) {
    AddRef();
    *ppvObject = static_cast<IToastNotifier*>(this);
    return S_OK;
  }

  return E_NOINTERFACE;
}

ULONG MockIToastNotifier::AddRef() {
  return ++refcount_;
}

ULONG MockIToastNotifier::Release() {
  return --refcount_;
}

HRESULT MockIToastNotifier::GetIids(ULONG* iidCount, IID** iids) {
  return E_NOTIMPL;
}

HRESULT MockIToastNotifier::GetRuntimeClassName(HSTRING* className) {
  return E_NOTIMPL;
}

HRESULT MockIToastNotifier::GetTrustLevel(TrustLevel* trustLevel) {
  return E_NOTIMPL;
}

HRESULT MockIToastNotifier::Show(
    winui::Notifications::IToastNotification* notification) {
  NotificationLaunchId launch_id = GetNotificationLaunchId(notification);
  notification_shown_callback_.Run(launch_id);
  return S_OK;
}

HRESULT MockIToastNotifier::Hide(
    winui::Notifications::IToastNotification* notification) {
  return E_NOTIMPL;
}

HRESULT MockIToastNotifier::get_Setting(
    winui::Notifications::NotificationSetting* value) {
  *value = winui::Notifications::NotificationSetting_Enabled;
  return S_OK;
}

HRESULT MockIToastNotifier::AddToSchedule(
    winui::Notifications::IScheduledToastNotification* scheduledToast) {
  return E_NOTIMPL;
}

HRESULT MockIToastNotifier::RemoveFromSchedule(
    winui::Notifications::IScheduledToastNotification* scheduledToast) {
  return E_NOTIMPL;
}

HRESULT MockIToastNotifier::GetScheduledToastNotifications(
    __FIVectorView_1_Windows__CUI__CNotifications__CScheduledToastNotification**
        scheduledToasts) {
  return E_NOTIMPL;
}
