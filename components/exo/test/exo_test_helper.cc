// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/test/exo_test_helper.h"

#include <memory>

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/wm/window_positioner.h"
#include "ash/wm/window_positioning_utils.h"
#include "components/exo/buffer.h"
#include "components/exo/client_controlled_shell_surface.h"
#include "components/exo/display.h"
#include "components/exo/surface.h"
#include "components/exo/wm_helper.h"
#include "components/exo/xdg_shell_surface.h"
#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"
#include "ui/aura/env.h"
#include "ui/compositor/compositor.h"
#include "ui/views/widget/widget.h"

namespace exo {
namespace test {

////////////////////////////////////////////////////////////////////////////////
// ExoTestHelper, public:

ExoTestWindow::ExoTestWindow(std::unique_ptr<gfx::GpuMemoryBuffer> gpu_buffer,
                             bool is_modal) {
  surface_.reset(new Surface());
  int container = is_modal ? ash::kShellWindowId_SystemModalContainer
                           : ash::kShellWindowId_DefaultContainer;
  shell_surface_ = std::make_unique<ShellSurface>(surface_.get(), gfx::Point(),
                                                  true, false, container);

  buffer_.reset(new Buffer(std::move(gpu_buffer)));
  surface_->Attach(buffer_.get());
  surface_->Commit();

  ash::wm::CenterWindow(shell_surface_->GetWidget()->GetNativeWindow());
}

ExoTestWindow::ExoTestWindow(ExoTestWindow&& other) {
  surface_ = std::move(other.surface_);
  buffer_ = std::move(other.buffer_);
  shell_surface_ = std::move(other.shell_surface_);
}

ExoTestWindow::~ExoTestWindow() {}

gfx::Point ExoTestWindow::origin() {
  return surface_->window()->GetBoundsInScreen().origin();
}

////////////////////////////////////////////////////////////////////////////////
// ExoTestHelper, public:

ExoTestHelper::ExoTestHelper() {
  ash::WindowPositioner::DisableAutoPositioning(true);
}

ExoTestHelper::~ExoTestHelper() {}

std::unique_ptr<gfx::GpuMemoryBuffer> ExoTestHelper::CreateGpuMemoryBuffer(
    const gfx::Size& size,
    gfx::BufferFormat format) {
  return aura::Env::GetInstance()
      ->context_factory()
      ->GetGpuMemoryBufferManager()
      ->CreateGpuMemoryBuffer(size, format, gfx::BufferUsage::GPU_READ,
                              gpu::kNullSurfaceHandle);
}

ExoTestWindow ExoTestHelper::CreateWindow(int width,
                                          int height,
                                          bool is_modal) {
  return ExoTestWindow(CreateGpuMemoryBuffer(gfx::Size(width, height)),
                       is_modal);
}

std::unique_ptr<ClientControlledShellSurface>
ExoTestHelper::CreateClientControlledShellSurface(Surface* surface,
                                                  bool is_modal) {
  int container = is_modal ? ash::kShellWindowId_SystemModalContainer
                           : ash::kShellWindowId_DefaultContainer;
  return Display().CreateClientControlledShellSurface(
      surface, container,
      WMHelper::GetInstance()->GetDefaultDeviceScaleFactor());
}

}  // namespace test
}  // namespace exo
