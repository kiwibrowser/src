// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws2/window_service.h"

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "services/ui/ws2/client_window.h"
#include "services/ui/ws2/gpu_support.h"
#include "services/ui/ws2/screen_provider.h"
#include "services/ui/ws2/window_service_client.h"
#include "services/ui/ws2/window_service_delegate.h"
#include "services/ui/ws2/window_tree_factory.h"
#include "ui/aura/env.h"

namespace ui {
namespace ws2 {

WindowService::WindowService(WindowServiceDelegate* delegate,
                             std::unique_ptr<GpuSupport> gpu_support,
                             aura::client::FocusClient* focus_client)
    : delegate_(delegate),
      gpu_support_(std::move(gpu_support)),
      screen_provider_(std::make_unique<ScreenProvider>()),
      focus_client_(focus_client),
      ime_registrar_(&ime_driver_) {
  DCHECK(focus_client);  // A |focus_client| must be provided.
  // MouseLocationManager is necessary for providing the shared memory with the
  // location of the mouse to clients.
  aura::Env::GetInstance()->CreateMouseLocationManager();

  input_device_server_.RegisterAsObserver();
}

WindowService::~WindowService() = default;

ClientWindow* WindowService::GetClientWindowForWindowCreateIfNecessary(
    aura::Window* window) {
  ClientWindow* client_window = ClientWindow::GetMayBeNull(window);
  if (client_window)
    return client_window;

  const viz::FrameSinkId frame_sink_id =
      ClientWindowId(kWindowServerClientId, next_window_id_++);
  CHECK_NE(0u, next_window_id_);
  const bool is_top_level = false;
  return ClientWindow::Create(window, nullptr, frame_sink_id, is_top_level);
}

std::unique_ptr<WindowServiceClient> WindowService::CreateWindowServiceClient(
    mojom::WindowTreeClient* window_tree_client,
    bool intercepts_events) {
  const ClientSpecificId client_id = next_client_id_++;
  CHECK_NE(0u, next_client_id_);
  return std::make_unique<WindowServiceClient>(
      this, client_id, window_tree_client, intercepts_events);
}

void WindowService::SetFrameDecorationValues(
    const gfx::Insets& client_area_insets,
    int max_title_bar_button_width) {
  screen_provider_->SetFrameDecorationValues(client_area_insets,
                                             max_title_bar_button_width);
}

void WindowService::OnStart() {
  window_tree_factory_ = std::make_unique<WindowTreeFactory>(this);

  registry_.AddInterface(base::BindRepeating(
      &WindowService::BindClipboardRequest, base::Unretained(this)));
  registry_.AddInterface(base::BindRepeating(
      &WindowService::BindScreenProviderRequest, base::Unretained(this)));
  registry_.AddInterface(base::BindRepeating(
      &WindowService::BindImeRegistrarRequest, base::Unretained(this)));
  registry_.AddInterface(base::BindRepeating(
      &WindowService::BindImeDriverRequest, base::Unretained(this)));
  registry_.AddInterface(base::BindRepeating(
      &WindowService::BindInputDeviceServerRequest, base::Unretained(this)));
  registry_.AddInterface(base::BindRepeating(
      &WindowService::BindWindowTreeFactoryRequest, base::Unretained(this)));

  // |gpu_support_| may be null in tests.
  if (gpu_support_) {
    registry_.AddInterface(
        base::BindRepeating(
            &GpuSupport::BindDiscardableSharedMemoryManagerOnGpuTaskRunner,
            base::Unretained(gpu_support_.get())),
        gpu_support_->GetGpuTaskRunner());
    registry_.AddInterface(
        base::BindRepeating(&GpuSupport::BindGpuRequestOnGpuTaskRunner,
                            base::Unretained(gpu_support_.get())),
        gpu_support_->GetGpuTaskRunner());
  }
}

void WindowService::OnBindInterface(
    const service_manager::BindSourceInfo& remote_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle handle) {
  registry_.BindInterface(interface_name, std::move(handle));
}

void WindowService::BindClipboardRequest(mojom::ClipboardRequest request) {
  // TODO: https://crbug.com/839591.
  NOTIMPLEMENTED();
}

void WindowService::BindScreenProviderRequest(
    mojom::ScreenProviderRequest request) {
  screen_provider_->AddBinding(std::move(request));
}

void WindowService::BindImeRegistrarRequest(
    mojom::IMERegistrarRequest request) {
  ime_registrar_.AddBinding(std::move(request));
}

void WindowService::BindImeDriverRequest(mojom::IMEDriverRequest request) {
  ime_driver_.AddBinding(std::move(request));
}

void WindowService::BindInputDeviceServerRequest(
    mojom::InputDeviceServerRequest request) {
  input_device_server_.AddBinding(std::move(request));
}

void WindowService::BindWindowTreeFactoryRequest(
    ui::mojom::WindowTreeFactoryRequest request) {
  window_tree_factory_->AddBinding(std::move(request));
}

}  // namespace ws2
}  // namespace ui
