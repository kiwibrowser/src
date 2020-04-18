// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/window_server_test_base.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/test/test_timeouts.h"
#include "base/threading/thread_task_runner_handle.h"
#include "services/service_manager/public/cpp/connector.h"
#include "ui/aura/env.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/mus/window_tree_host_mus.h"
#include "ui/base/ui_base_features.h"
#include "ui/base/ui_base_switches.h"
#include "ui/display/display.h"
#include "ui/display/display_list.h"
#include "ui/wm/core/capture_controller.h"

namespace ui {
namespace {

base::RunLoop* current_run_loop = nullptr;

void TimeoutRunLoop(const base::Closure& timeout_task, bool* timeout) {
  CHECK(current_run_loop);
  *timeout = true;
  timeout_task.Run();
}

}  // namespace

WindowServerTestBase::WindowServerTestBase() {
  registry_.AddInterface<mojom::WindowTreeClient>(
      base::Bind(&WindowServerTestBase::BindWindowTreeClientRequest,
                 base::Unretained(this)));
}

WindowServerTestBase::~WindowServerTestBase() {}

// static
bool WindowServerTestBase::DoRunLoopWithTimeout() {
  if (current_run_loop != nullptr)
    return false;

  bool timeout = false;
  base::RunLoop run_loop;
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, base::Bind(&TimeoutRunLoop, run_loop.QuitClosure(), &timeout),
      TestTimeouts::action_timeout());

  current_run_loop = &run_loop;
  current_run_loop->Run();
  current_run_loop = nullptr;
  return !timeout;
}

// static
bool WindowServerTestBase::QuitRunLoop() {
  if (!current_run_loop)
    return false;

  current_run_loop->Quit();
  current_run_loop = nullptr;
  return true;
}

void WindowServerTestBase::DeleteWindowTreeClient(
    aura::WindowTreeClient* client) {
  for (auto iter = window_tree_clients_.begin();
       iter != window_tree_clients_.end(); ++iter) {
    if (iter->get() == client) {
      window_tree_clients_.erase(iter);
      return;
    }
  }
  NOTREACHED();
}

std::unique_ptr<aura::WindowTreeClient>
WindowServerTestBase::ReleaseMostRecentClient() {
  if (window_tree_clients_.empty())
    return nullptr;

  std::unique_ptr<aura::WindowTreeClient> result =
      std::move(window_tree_clients_.back());
  window_tree_clients_.pop_back();
  return result;
}

void WindowServerTestBase::SetUp() {
  feature_list_.InitAndEnableFeature(features::kMash);
  WindowServerServiceTestBase::SetUp();

  env_ = aura::Env::CreateInstance(aura::Env::Mode::MUS);
  display::Screen::SetScreenInstance(&screen_);
  std::unique_ptr<aura::WindowTreeClient> window_manager_window_tree_client =
      aura::WindowTreeClient::CreateForWindowManager(connector(), this, this);
  window_manager_ = window_manager_window_tree_client.get();
  window_tree_clients_.push_back(std::move(window_manager_window_tree_client));

  // Connecting as the WindowManager results in OnWmNewDisplay() being called
  // with the display (and root). Wait for it to be called so we have display
  // and root window information (otherwise we can't really do anything).
  ASSERT_TRUE(DoRunLoopWithTimeout());
}

void WindowServerTestBase::TearDown() {
  // WindowTreeHost depends on WindowTreeClient.
  window_tree_hosts_.clear();
  window_tree_clients_.clear();
  env_.reset();
  display::Screen::SetScreenInstance(nullptr);

  WindowServerServiceTestBase::TearDown();
}

void WindowServerTestBase::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

void WindowServerTestBase::OnEmbed(
    std::unique_ptr<aura::WindowTreeHostMus> window_tree_host) {
  EXPECT_TRUE(QuitRunLoop());
  window_tree_hosts_.push_back(std::move(window_tree_host));
}

void WindowServerTestBase::OnLostConnection(aura::WindowTreeClient* client) {
  window_tree_client_lost_connection_ = true;
  DeleteWindowTreeClient(client);
}

void WindowServerTestBase::OnEmbedRootDestroyed(
    aura::WindowTreeHostMus* window_tree_host) {
  if (!DeleteWindowTreeHost(window_tree_host)) {
    // Assume a subclass called Embed() and wants us to destroy it.
    delete window_tree_host;
  }
}

void WindowServerTestBase::OnPointerEventObserved(const ui::PointerEvent& event,
                                                  int64_t display_id,
                                                  aura::Window* target) {}

aura::PropertyConverter* WindowServerTestBase::GetPropertyConverter() {
  return &property_converter_;
}

void WindowServerTestBase::SetWindowManagerClient(
    aura::WindowManagerClient* client) {
  window_manager_client_ = client;
}

void WindowServerTestBase::OnWmConnected() {}

void WindowServerTestBase::OnWmSetBounds(aura::Window* window,
                                         const gfx::Rect& bounds) {
  if (!window_manager_delegate_)
    return;
  window_manager_delegate_->OnWmSetBounds(window, bounds);
}

bool WindowServerTestBase::OnWmSetProperty(
    aura::Window* window,
    const std::string& name,
    std::unique_ptr<std::vector<uint8_t>>* new_data) {
  return window_manager_delegate_
             ? window_manager_delegate_->OnWmSetProperty(window, name, new_data)
             : true;
}

void WindowServerTestBase::OnWmSetModalType(aura::Window* window,
                                            ui::ModalType type) {
  if (window_manager_delegate_)
    window_manager_delegate_->OnWmSetModalType(window, type);
}

void WindowServerTestBase::OnWmSetCanFocus(aura::Window* window,
                                           bool can_focus) {
  if (window_manager_delegate_)
    window_manager_delegate_->OnWmSetCanFocus(window, can_focus);
}

aura::Window* WindowServerTestBase::OnWmCreateTopLevelWindow(
    ui::mojom::WindowType window_type,
    std::map<std::string, std::vector<uint8_t>>* properties) {
  return window_manager_delegate_
             ? window_manager_delegate_->OnWmCreateTopLevelWindow(window_type,
                                                                  properties)
             : nullptr;
}

void WindowServerTestBase::OnWmClientJankinessChanged(
    const std::set<aura::Window*>& client_windows,
    bool janky) {
  if (window_manager_delegate_)
    window_manager_delegate_->OnWmClientJankinessChanged(client_windows, janky);
}

void WindowServerTestBase::OnWmWillCreateDisplay(
    const display::Display& display) {
  screen_.display_list().AddDisplay(display,
                                    display::DisplayList::Type::PRIMARY);
  if (window_manager_delegate_)
    window_manager_delegate_->OnWmWillCreateDisplay(display);
}

void WindowServerTestBase::OnWmNewDisplay(
    std::unique_ptr<aura::WindowTreeHostMus> window_tree_host,
    const display::Display& display) {
  EXPECT_TRUE(QuitRunLoop());
  ASSERT_TRUE(window_manager_client_);
  window_manager_client_->AddActivationParent(window_tree_host->window());
  window_tree_hosts_.push_back(std::move(window_tree_host));

  if (window_manager_delegate_)
    window_manager_delegate_->OnWmNewDisplay(nullptr, display);
}

void WindowServerTestBase::OnWmDisplayRemoved(
    aura::WindowTreeHostMus* window_tree_host) {
  if (window_manager_delegate_)
    window_manager_delegate_->OnWmDisplayRemoved(window_tree_host);
  ASSERT_TRUE(DeleteWindowTreeHost(window_tree_host));
}

void WindowServerTestBase::OnWmDisplayModified(
    const display::Display& display) {
  if (window_manager_delegate_)
    window_manager_delegate_->OnWmDisplayModified(display);
}

ui::mojom::EventResult WindowServerTestBase::OnAccelerator(
    uint32_t accelerator_id,
    const ui::Event& event,
    base::flat_map<std::string, std::vector<uint8_t>>* properties) {
  return window_manager_delegate_ ? window_manager_delegate_->OnAccelerator(
                                        accelerator_id, event, properties)
                                  : ui::mojom::EventResult::UNHANDLED;
}

void WindowServerTestBase::OnCursorTouchVisibleChanged(bool enabled) {
  if (window_manager_delegate_)
    window_manager_delegate_->OnCursorTouchVisibleChanged(enabled);
}

void WindowServerTestBase::OnWmPerformMoveLoop(
    aura::Window* window,
    ui::mojom::MoveLoopSource source,
    const gfx::Point& cursor_location,
    const base::Callback<void(bool)>& on_done) {
  if (window_manager_delegate_) {
    window_manager_delegate_->OnWmPerformMoveLoop(window, source,
                                                  cursor_location, on_done);
  }
}

void WindowServerTestBase::OnWmCancelMoveLoop(aura::Window* window) {
  if (window_manager_delegate_)
    window_manager_delegate_->OnWmCancelMoveLoop(window);
}

void WindowServerTestBase::OnWmSetClientArea(
    aura::Window* window,
    const gfx::Insets& insets,
    const std::vector<gfx::Rect>& additional_client_areas) {
  if (window_manager_delegate_) {
    window_manager_delegate_->OnWmSetClientArea(window, insets,
                                                additional_client_areas);
  }
}

bool WindowServerTestBase::IsWindowActive(aura::Window* window) {
  if (window_manager_delegate_)
    window_manager_delegate_->IsWindowActive(window);
  return false;
}

void WindowServerTestBase::OnWmDeactivateWindow(aura::Window* window) {
  if (window_manager_delegate_)
    window_manager_delegate_->OnWmDeactivateWindow(window);
}

void WindowServerTestBase::BindWindowTreeClientRequest(
    ui::mojom::WindowTreeClientRequest request) {
  const bool create_discardable_memory = false;
  window_tree_clients_.push_back(aura::WindowTreeClient::CreateForEmbedding(
      connector(), this, std::move(request), create_discardable_memory));
}

bool WindowServerTestBase::DeleteWindowTreeHost(
    aura::WindowTreeHostMus* window_tree_host) {
  for (auto iter = window_tree_hosts_.begin(); iter != window_tree_hosts_.end();
       ++iter) {
    if ((*iter).get() == window_tree_host) {
      window_tree_hosts_.erase(iter);
      return true;
    }
  }
  return false;
}

}  // namespace ui
