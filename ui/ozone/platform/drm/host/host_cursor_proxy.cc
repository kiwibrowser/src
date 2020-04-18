// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/host/host_cursor_proxy.h"

#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "ui/ozone/public/gpu_platform_support_host.h"

namespace ui {

// We assume that this is invoked only on the Mus/UI thread.
HostCursorProxy::HostCursorProxy(
    ui::ozone::mojom::DeviceCursorPtr main_cursor_ptr,
    ui::ozone::mojom::DeviceCursorPtr evdev_cursor_ptr)
    : main_cursor_ptr_(std::move(main_cursor_ptr)),
      evdev_cursor_ptr_(std::move(evdev_cursor_ptr)),
      ui_thread_ref_(base::PlatformThread::CurrentRef()) {}

HostCursorProxy::~HostCursorProxy() {}

void HostCursorProxy::CursorSet(gfx::AcceleratedWidget widget,
                                const std::vector<SkBitmap>& bitmaps,
                                const gfx::Point& location,
                                int frame_delay_ms) {
  InitializeOnEvdevIfNecessary();
  if (ui_thread_ref_ == base::PlatformThread::CurrentRef()) {
    main_cursor_ptr_->SetCursor(widget, bitmaps, location, frame_delay_ms);
  } else {
    evdev_cursor_ptr_->SetCursor(widget, bitmaps, location, frame_delay_ms);
  }
}

void HostCursorProxy::Move(gfx::AcceleratedWidget widget,
                           const gfx::Point& location) {
  InitializeOnEvdevIfNecessary();
  if (ui_thread_ref_ == base::PlatformThread::CurrentRef()) {
    main_cursor_ptr_->MoveCursor(widget, location);
  } else {
    evdev_cursor_ptr_->MoveCursor(widget, location);
  }
}

// Evdev runs this method on starting. But if a HostCursorProxy is created long
// after Evdev has started (e.g. if the Viz process crashes (and the
// |HostCursorProxy| self-destructs and then a new |HostCursorProxy| is built
// when the GpuThread/DrmThread pair are once again running), we need to run it
// on cursor motions.
void HostCursorProxy::InitializeOnEvdevIfNecessary() {
  // TODO(rjkroege): Rebind on Viz process restart.
  if (evdev_bound_)
    return;

  if (ui_thread_ref_ != base::PlatformThread::CurrentRef()) {
    // Rebind the mojo pipe on the current thread. We expect this to be the
    // thread running EVDEV.
    evdev_cursor_ptr_.Bind(evdev_cursor_ptr_.PassInterface());
  }
}

}  // namespace ui
