// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/wm/test/wm_test_helper.h"

#include <memory>
#include <utility>

#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/public/cpp/input_devices/input_device_client.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "ui/aura/client/default_capture_client.h"
#include "ui/aura/env.h"
#include "ui/aura/mus/property_converter.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/mus/window_tree_host_mus.h"
#include "ui/aura/test/mus/window_tree_client_private.h"
#include "ui/aura/test/test_focus_client.h"
#include "ui/aura/window.h"
#include "ui/wm/core/compound_event_filter.h"
#include "ui/wm/core/default_activation_client.h"
#include "ui/wm/core/wm_state.h"

namespace wm {

WMTestHelper::WMTestHelper(const gfx::Size& default_window_size,
                           service_manager::Connector* connector,
                           ui::ContextFactory* context_factory) {
  wm_state_ = std::make_unique<WMState>();
  if (context_factory)
    aura::Env::GetInstance()->set_context_factory(context_factory);
  if (aura::Env::GetInstance()->mode() == aura::Env::Mode::LOCAL)
    InitLocalHost(default_window_size);
  else
    InitMusHost(connector);
  aura::client::SetWindowParentingClient(host_->window(), this);

  focus_client_.reset(new aura::test::TestFocusClient);
  aura::client::SetFocusClient(host_->window(), focus_client_.get());

  root_window_event_filter_.reset(new wm::CompoundEventFilter);
  host_->window()->AddPreTargetHandler(root_window_event_filter_.get());

  new wm::DefaultActivationClient(host_->window());

  capture_client_.reset(
      new aura::client::DefaultCaptureClient(host_->window()));
}

WMTestHelper::~WMTestHelper() {
}

aura::Window* WMTestHelper::GetDefaultParent(aura::Window* window,
                                             const gfx::Rect& bounds) {
  return host_->window();
}

void WMTestHelper::InitLocalHost(const gfx::Size& default_window_size) {
  host_.reset(aura::WindowTreeHost::Create(gfx::Rect(default_window_size)));
  host_->InitHost();
}

void WMTestHelper::InitMusHost(service_manager::Connector* connector) {
  DCHECK(!aura::Env::GetInstance()->HasWindowTreeClient());

  input_device_client_ = std::make_unique<ui::InputDeviceClient>();
  ui::mojom::InputDeviceServerPtr input_device_server;
  connector->BindInterface(ui::mojom::kServiceName, &input_device_server);
  input_device_client_->Connect(std::move(input_device_server));

  property_converter_ = std::make_unique<aura::PropertyConverter>();

  const bool create_discardable_memory = false;
  window_tree_client_ = aura::WindowTreeClient::CreateForWindowTreeHostFactory(
      connector, this, create_discardable_memory);
  aura::Env::GetInstance()->SetWindowTreeClient(window_tree_client_.get());
  aura::WindowTreeClientPrivate(window_tree_client_.get())
      .WaitForInitialDisplays();

  // ConnectViaWindowTreeHostFactory() should callback to OnEmbed() and set
  // |host_|.
  DCHECK(host_.get());
}

void WMTestHelper::OnEmbed(
    std::unique_ptr<aura::WindowTreeHostMus> window_tree_host) {
  host_ = std::move(window_tree_host);
}

void WMTestHelper::OnUnembed(aura::Window* root) {}

void WMTestHelper::OnEmbedRootDestroyed(
    aura::WindowTreeHostMus* window_tree_host) {}

void WMTestHelper::OnLostConnection(aura::WindowTreeClient* client) {}

void WMTestHelper::OnPointerEventObserved(const ui::PointerEvent& event,
                                          int64_t display_id,
                                          aura::Window* target) {}

aura::PropertyConverter* WMTestHelper::GetPropertyConverter() {
  return property_converter_.get();
}

}  // namespace wm
