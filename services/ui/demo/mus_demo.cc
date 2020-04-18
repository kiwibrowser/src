// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/demo/mus_demo.h"

#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/demo/window_tree_data.h"
#include "services/ui/public/cpp/gpu/gpu.h"
#include "ui/aura/client/default_capture_client.h"
#include "ui/aura/env.h"
#include "ui/aura/mus/property_converter.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/mus/window_tree_host_mus.h"
#include "ui/aura/window.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/wm/core/wm_state.h"

namespace ui {
namespace demo {

MusDemo::MusDemo() {}

MusDemo::~MusDemo() {
  display::Screen::SetScreenInstance(nullptr);
  // Destruction order is important here:
  // 1) Windows must be destroyed before WindowTreeClient's destructor is
  // called.
  // 2) WindowTreeClient must be destroyed before Env and WMState.
  window_tree_data_list_.clear();
  window_tree_client_.reset();
  env_.reset();
  wm_state_.reset();
}

void MusDemo::AddPrimaryDisplay(const display::Display& display) {
  screen_->display_list().AddDisplay(display,
                                     display::DisplayList::Type::PRIMARY);
}

bool MusDemo::HasPendingWindowTreeData() const {
  return !window_tree_data_list_.empty() &&
         !window_tree_data_list_.back()->IsInitialized();
}

void MusDemo::AppendWindowTreeData(
    std::unique_ptr<WindowTreeData> window_tree_data) {
  DCHECK(!HasPendingWindowTreeData());
  window_tree_data_list_.push_back(std::move(window_tree_data));
}

void MusDemo::InitWindowTreeData(
    std::unique_ptr<aura::WindowTreeHostMus> window_tree_host) {
  DCHECK(HasPendingWindowTreeData());
  window_tree_data_list_.back()->Init(std::move(window_tree_host));
}

void MusDemo::RemoveWindowTreeData(aura::WindowTreeHostMus* window_tree_host) {
  DCHECK(window_tree_host);
  auto it =
      std::find_if(window_tree_data_list_.begin(), window_tree_data_list_.end(),
                   [window_tree_host](std::unique_ptr<WindowTreeData>& data) {
                     return data->WindowTreeHost() == window_tree_host;
                   });
  DCHECK(it != window_tree_data_list_.end());
  window_tree_data_list_.erase(it);
}

void MusDemo::OnStart() {
  screen_ = std::make_unique<display::ScreenBase>();
  display::Screen::SetScreenInstance(screen_.get());

  env_ = aura::Env::CreateInstance(aura::Env::Mode::MUS);
  capture_client_ = std::make_unique<aura::client::DefaultCaptureClient>();
  property_converter_ = std::make_unique<aura::PropertyConverter>();
  wm_state_ = std::make_unique<::wm::WMState>();

  window_tree_client_ = CreateWindowTreeClient();
  env_->SetWindowTreeClient(window_tree_client_.get());

  OnStartImpl();
}

void MusDemo::OnEmbed(
    std::unique_ptr<aura::WindowTreeHostMus> window_tree_host) {
  NOTREACHED();
}

void MusDemo::OnUnembed(aura::Window* root) {
  NOTREACHED();
}

void MusDemo::OnEmbedRootDestroyed(aura::WindowTreeHostMus* window_tree_host) {
  NOTREACHED();
}

void MusDemo::OnLostConnection(aura::WindowTreeClient* client) {
  window_tree_client_.reset();
  window_tree_data_list_.clear();
}

void MusDemo::OnPointerEventObserved(const PointerEvent& event,
                                     int64_t display_id,
                                     aura::Window* target) {}

aura::PropertyConverter* MusDemo::GetPropertyConverter() {
  return property_converter_.get();
}

}  // namespace demo
}  // namespace ui
