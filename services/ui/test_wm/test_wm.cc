// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "base/threading/thread_task_runner_handle.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/c/main.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_runner.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/client/default_capture_client.h"
#include "ui/aura/env.h"
#include "ui/aura/mus/property_converter.h"
#include "ui/aura/mus/property_utils.h"
#include "ui/aura/mus/window_manager_delegate.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/mus/window_tree_client_delegate.h"
#include "ui/aura/mus/window_tree_host_mus.h"
#include "ui/aura/test/test_focus_client.h"
#include "ui/aura/window.h"
#include "ui/display/display.h"
#include "ui/display/display_list.h"
#include "ui/display/screen_base.h"
#include "ui/wm/core/capture_controller.h"
#include "ui/wm/core/wm_state.h"

namespace ui {
namespace test {

class TestWM : public service_manager::Service,
               public aura::WindowTreeClientDelegate,
               public aura::WindowManagerDelegate {
 public:
  TestWM() {}

  ~TestWM() override {
    default_capture_client_.reset();

    // WindowTreeHost uses state from WindowTreeClient, so destroy it first.
    window_tree_host_.reset();

    // WindowTreeClient destruction may callback to us.
    window_tree_client_.reset();

    display::Screen::SetScreenInstance(nullptr);
  }

 private:
  // service_manager::Service:
  void OnStart() override {
    CHECK(!started_);
    started_ = true;
    screen_ = std::make_unique<display::ScreenBase>();
    display::Screen::SetScreenInstance(screen_.get());
    aura_env_ = aura::Env::CreateInstance(aura::Env::Mode::MUS);
    window_tree_client_ = aura::WindowTreeClient::CreateForWindowManager(
        context()->connector(), this, this);
    aura_env_->SetWindowTreeClient(window_tree_client_.get());
  }
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {}

  // aura::WindowTreeClientDelegate:
  void OnEmbed(
      std::unique_ptr<aura::WindowTreeHostMus> window_tree_host) override {
    // WindowTreeClients configured as the window manager should never get
    // OnEmbed().
    NOTREACHED();
  }
  void OnLostConnection(aura::WindowTreeClient* client) override {
    window_tree_host_.reset();
    window_tree_client_.reset();
  }
  void OnEmbedRootDestroyed(
      aura::WindowTreeHostMus* window_tree_host) override {
    // WindowTreeClients configured as the window manager should never get
    // OnEmbedRootDestroyed().
    NOTREACHED();
  }
  void OnPointerEventObserved(const ui::PointerEvent& event,
                              int64_t display_id,
                              aura::Window* target) override {
    // Don't care.
  }
  aura::PropertyConverter* GetPropertyConverter() override {
    return &property_converter_;
  }

  // aura::WindowManagerDelegate:
  void SetWindowManagerClient(aura::WindowManagerClient* client) override {
    window_manager_client_ = client;
  }
  void OnWmConnected() override {}
  void OnWmAcceleratedWidgetAvailableForDisplay(
      int64_t display_id,
      gfx::AcceleratedWidget widget) override {}
  void OnWmSetBounds(aura::Window* window, const gfx::Rect& bounds) override {
    window->SetBounds(bounds);
  }
  bool OnWmSetProperty(
      aura::Window* window,
      const std::string& name,
      std::unique_ptr<std::vector<uint8_t>>* new_data) override {
    return true;
  }
  void OnWmSetModalType(aura::Window* window, ui::ModalType type) override {}
  void OnWmSetCanFocus(aura::Window* window, bool can_focus) override {}
  aura::Window* OnWmCreateTopLevelWindow(
      ui::mojom::WindowType window_type,
      std::map<std::string, std::vector<uint8_t>>* properties) override {
    aura::Window* window = new aura::Window(nullptr);
    window->SetProperty(aura::client::kEmbedType,
                        aura::client::WindowEmbedType::TOP_LEVEL_IN_WM);
    SetWindowType(window, window_type);
    window->Init(LAYER_NOT_DRAWN);
    window->SetBounds(gfx::Rect(10, 10, 500, 500));
    root_->AddChild(window);
    return window;
  }
  void OnWmClientJankinessChanged(const std::set<aura::Window*>& client_windows,
                                  bool janky) override {
    // Don't care.
  }
  void OnWmBuildDragImage(const gfx::Point& screen_location,
                          const SkBitmap& drag_image,
                          const gfx::Vector2d& drag_image_offset,
                          ui::mojom::PointerKind source) override {}
  void OnWmMoveDragImage(const gfx::Point& screen_location) override {}
  void OnWmDestroyDragImage() override {}
  void OnWmWillCreateDisplay(const display::Display& display) override {
    // This class only deals with one display.
    DCHECK_EQ(0u, screen_->display_list().displays().size());
    screen_->display_list().AddDisplay(display,
                                       display::DisplayList::Type::PRIMARY);
  }
  void OnWmNewDisplay(std::unique_ptr<aura::WindowTreeHostMus> window_tree_host,
                      const display::Display& display) override {
    // Only handles a single root.
    DCHECK(!root_);
    window_tree_host_ = std::move(window_tree_host);
    root_ = window_tree_host_->window();
    default_capture_client_ =
        std::make_unique<aura::client::DefaultCaptureClient>(
            root_->GetRootWindow());
    DCHECK(window_manager_client_);
    window_manager_client_->AddActivationParent(root_);
    ui::mojom::FrameDecorationValuesPtr frame_decoration_values =
        ui::mojom::FrameDecorationValues::New();
    frame_decoration_values->max_title_bar_button_width = 0;
    window_manager_client_->SetFrameDecorationValues(
        std::move(frame_decoration_values));
    aura::client::SetFocusClient(root_, &focus_client_);
  }
  void OnWmDisplayRemoved(aura::WindowTreeHostMus* window_tree_host) override {
    DCHECK_EQ(window_tree_host, window_tree_host_.get());
    root_ = nullptr;
    default_capture_client_.reset();
    window_tree_host_.reset();
  }
  void OnWmDisplayModified(const display::Display& display) override {}
  void OnCursorTouchVisibleChanged(bool enabled) override {}
  void OnWmPerformMoveLoop(aura::Window* window,
                           mojom::MoveLoopSource source,
                           const gfx::Point& cursor_location,
                           const base::Callback<void(bool)>& on_done) override {
    // Don't care.
  }
  void OnWmCancelMoveLoop(aura::Window* window) override {}
  void OnWmSetClientArea(
      aura::Window* window,
      const gfx::Insets& insets,
      const std::vector<gfx::Rect>& additional_client_areas) override {}
  bool IsWindowActive(aura::Window* window) override {
    // Focus client interface doesn't expose this; assume true.
    return true;
  }
  void OnWmDeactivateWindow(aura::Window* window) override {
    aura::client::GetFocusClient(root_)->FocusWindow(nullptr);
  }

  std::unique_ptr<display::ScreenBase> screen_;

  std::unique_ptr<aura::Env> aura_env_;
  ::wm::WMState wm_state_;
  aura::PropertyConverter property_converter_;
  aura::test::TestFocusClient focus_client_;
  std::unique_ptr<aura::WindowTreeHostMus> window_tree_host_;
  aura::Window* root_ = nullptr;
  aura::WindowManagerClient* window_manager_client_ = nullptr;
  std::unique_ptr<aura::WindowTreeClient> window_tree_client_;
  std::unique_ptr<aura::client::DefaultCaptureClient> default_capture_client_;

  bool started_ = false;

  DISALLOW_COPY_AND_ASSIGN(TestWM);
};

}  // namespace test
}  // namespace ui

MojoResult ServiceMain(MojoHandle service_request_handle) {
  service_manager::ServiceRunner runner(new ui::test::TestWM);
  return runner.Run(service_request_handle);
}
