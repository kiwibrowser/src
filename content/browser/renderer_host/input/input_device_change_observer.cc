// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/input_device_change_observer.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "content/public/common/web_preferences.h"

#if defined(OS_WIN)
#include "ui/events/devices/input_device_observer_win.h"
#elif defined(OS_LINUX)
#include "ui/events/devices/input_device_manager.h"
#elif defined(OS_ANDROID)
#include "ui/events/devices/input_device_observer_android.h"
#endif

namespace content {

InputDeviceChangeObserver::InputDeviceChangeObserver(RenderViewHost* rvh) {
  render_view_host_ = rvh;
#if defined(OS_WIN)
  ui::InputDeviceObserverWin::GetInstance()->AddObserver(this);
#elif defined(OS_LINUX)
  ui::InputDeviceManager::GetInstance()->AddObserver(this);
#elif defined(OS_ANDROID)
  ui::InputDeviceObserverAndroid::GetInstance()->AddObserver(this);
#endif
}

InputDeviceChangeObserver::~InputDeviceChangeObserver() {
#if defined(OS_WIN)
  ui::InputDeviceObserverWin::GetInstance()->RemoveObserver(this);
#elif defined(OS_LINUX)
  ui::InputDeviceManager::GetInstance()->RemoveObserver(this);
#elif defined(OS_ANDROID)
  ui::InputDeviceObserverAndroid::GetInstance()->RemoveObserver(this);
#endif
  render_view_host_ = nullptr;
}

void InputDeviceChangeObserver::OnTouchscreenDeviceConfigurationChanged() {
  NotifyRenderViewHost();
}

void InputDeviceChangeObserver::OnKeyboardDeviceConfigurationChanged() {
  NotifyRenderViewHost();
}

void InputDeviceChangeObserver::OnMouseDeviceConfigurationChanged() {
  NotifyRenderViewHost();
}

void InputDeviceChangeObserver::OnTouchpadDeviceConfigurationChanged() {
  NotifyRenderViewHost();
}

void InputDeviceChangeObserver::NotifyRenderViewHost() {
  WebPreferences prefs = render_view_host_->GetWebkitPreferences();
  int available_pointer_types, available_hover_types;
  std::tie(available_pointer_types, available_hover_types) =
      ui::GetAvailablePointerAndHoverTypes();
  bool input_device_changed =
      prefs.available_pointer_types != available_pointer_types ||
      prefs.available_hover_types != available_hover_types;

  if (input_device_changed) {
    TRACE_EVENT0("input", "InputDeviceChangeObserver::NotifyRendererViewHost");
    render_view_host_->OnWebkitPreferencesChanged();
  }
}

}  // namespace content
