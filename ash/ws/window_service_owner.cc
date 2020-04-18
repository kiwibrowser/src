// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/ws/window_service_owner.h"

#include "ash/public/cpp/window_properties.h"
#include "ash/shell.h"
#include "ash/wm/non_client_frame_controller.h"
#include "ash/ws/window_service_delegate_impl.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/ui/ws2/gpu_support.h"
#include "services/ui/ws2/window_service.h"
#include "ui/wm/core/focus_controller.h"

namespace ash {
namespace {

void BindWindowServiceOnMainThread(
    service_manager::mojom::ServiceRequest request) {
  Shell::Get()->window_service_owner()->BindWindowService(std::move(request));
}

}  // namespace

WindowServiceOwner::WindowServiceOwner(
    std::unique_ptr<ui::ws2::GpuSupport> gpu_support)
    : gpu_support_(std::move(gpu_support)) {}

WindowServiceOwner::~WindowServiceOwner() = default;

void WindowServiceOwner::BindWindowService(
    service_manager::mojom::ServiceRequest request) {
  // This should only be called once. If called more than once it means the
  // WindowService lost its connection to the service_manager, which triggered
  // a new connection WindowService to be created. That should never happen.
  DCHECK(!service_context_);

  window_service_delegate_ = std::make_unique<WindowServiceDelegateImpl>();
  std::unique_ptr<ui::ws2::WindowService> window_service =
      std::make_unique<ui::ws2::WindowService>(
          window_service_delegate_.get(), std::move(gpu_support_),
          Shell::Get()->focus_controller());
  window_service_ = window_service.get();
  window_service_->SetFrameDecorationValues(
      NonClientFrameController::GetPreferredClientAreaInsets(),
      NonClientFrameController::GetMaxTitleBarButtonWidth());
  RegisterWindowProperties(window_service_->property_converter());
  service_context_ = std::make_unique<service_manager::ServiceContext>(
      std::move(window_service), std::move(request));
}

void BindWindowServiceOnIoThread(
    scoped_refptr<base::SingleThreadTaskRunner> main_runner,
    service_manager::mojom::ServiceRequest request) {
  main_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&BindWindowServiceOnMainThread, std::move(request)));
}

}  // namespace ash
