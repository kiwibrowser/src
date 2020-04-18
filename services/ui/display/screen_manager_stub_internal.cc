// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/display/screen_manager_stub_internal.h"

#include <memory>

#include "base/bind.h"
#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/ui/display/viewport_metrics.h"
#include "ui/display/screen_base.h"
#include "ui/gfx/geometry/dip_util.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

namespace display {
namespace {

constexpr gfx::Size kDisplayPixelSize(1024, 768);

// Build a 1024x768 pixel display.
Display DefaultDisplay() {
  float device_scale_factor = 1.f;
  if (Display::HasForceDeviceScaleFactor())
    device_scale_factor = Display::GetForcedDeviceScaleFactor();

  gfx::Size scaled_size =
      gfx::ConvertSizeToDIP(device_scale_factor, kDisplayPixelSize);

  Display display(1);
  display.set_bounds(gfx::Rect(scaled_size));
  display.set_work_area(display.bounds());
  display.set_device_scale_factor(device_scale_factor);

  return display;
}

}  // namespace

// static
std::unique_ptr<ScreenManager> ScreenManager::Create() {
  return std::make_unique<ScreenManagerStubInternal>();
}

ScreenManagerStubInternal::ScreenManagerStubInternal()
    : screen_(std::make_unique<display::ScreenBase>()),
      weak_ptr_factory_(this) {}

ScreenManagerStubInternal::~ScreenManagerStubInternal() {}

void ScreenManagerStubInternal::FixedSizeScreenConfiguration() {
  ViewportMetrics metrics;
  metrics.bounds_in_pixels = gfx::Rect(kDisplayPixelSize);
  metrics.device_scale_factor = display_.device_scale_factor();
  metrics.ui_scale_factor = 1.f;

  delegate_->OnDisplayAdded(display_, metrics);
}

void ScreenManagerStubInternal::AddInterfaces(
    service_manager::BinderRegistryWithArgs<
        const service_manager::BindSourceInfo&>* registry) {}

void ScreenManagerStubInternal::Init(ScreenManagerDelegate* delegate) {
  DCHECK(delegate);
  delegate_ = delegate;
  display_ = DefaultDisplay();
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&ScreenManagerStubInternal::FixedSizeScreenConfiguration,
                 weak_ptr_factory_.GetWeakPtr()));
}

void ScreenManagerStubInternal::RequestCloseDisplay(int64_t display_id) {
  if (display_id == display_.id()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&ScreenManagerDelegate::OnDisplayRemoved,
                              base::Unretained(delegate_), display_id));
  }
}

display::ScreenBase* ScreenManagerStubInternal::GetScreen() {
  return screen_.get();
}

}  // namespace display
