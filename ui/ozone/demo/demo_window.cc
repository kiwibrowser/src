// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/demo/demo_window.h"

#include "base/threading/thread_task_runner_handle.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/ozone/demo/renderer.h"
#include "ui/ozone/demo/renderer_factory.h"
#include "ui/ozone/demo/window_manager.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ui/platform_window/platform_window.h"

namespace ui {

DemoWindow::DemoWindow(WindowManager* window_manager,
                       RendererFactory* renderer_factory,
                       const gfx::Rect& bounds)
    : window_manager_(window_manager),
      renderer_factory_(renderer_factory),
      weak_ptr_factory_(this) {
  platform_window_ =
      OzonePlatform::GetInstance()->CreatePlatformWindow(this, bounds);
  platform_window_->Show();
}

DemoWindow::~DemoWindow() {}

gfx::AcceleratedWidget DemoWindow::GetAcceleratedWidget() {
  // TODO(spang): We should start rendering asynchronously.
  DCHECK_NE(widget_, gfx::kNullAcceleratedWidget)
      << "Widget not available synchronously";
  return widget_;
}

gfx::Size DemoWindow::GetSize() {
  return platform_window_->GetBounds().size();
}

void DemoWindow::Start() {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&DemoWindow::StartOnGpu, weak_ptr_factory_.GetWeakPtr()));
}

void DemoWindow::Quit() {
  window_manager_->Quit();
}

void DemoWindow::OnBoundsChanged(const gfx::Rect& new_bounds) {}

void DemoWindow::OnDamageRect(const gfx::Rect& damaged_region) {}

void DemoWindow::DispatchEvent(Event* event) {
  if (event->IsKeyEvent() && event->AsKeyEvent()->code() == DomCode::US_Q)
    Quit();
}

void DemoWindow::OnCloseRequest() {
  Quit();
}

void DemoWindow::OnClosed() {}

void DemoWindow::OnWindowStateChanged(PlatformWindowState new_state) {}

void DemoWindow::OnLostCapture() {}

void DemoWindow::OnAcceleratedWidgetAvailable(gfx::AcceleratedWidget widget,
                                              float device_pixel_ratio) {
  DCHECK_NE(widget, gfx::kNullAcceleratedWidget);
  widget_ = widget;
}

void DemoWindow::OnAcceleratedWidgetDestroying() {}

void DemoWindow::OnAcceleratedWidgetDestroyed() {
  widget_ = gfx::kNullAcceleratedWidget;
}

void DemoWindow::OnActivationChanged(bool active) {}

void DemoWindow::StartOnGpu() {
  renderer_ =
      renderer_factory_->CreateRenderer(GetAcceleratedWidget(), GetSize());
  if (!renderer_->Initialize())
    LOG(ERROR) << "Failed to initialize renderer.";
}

}  // namespace ui
