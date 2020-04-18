// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_WINDOW_SERVER_TEST_BASE_H_
#define SERVICES_UI_WS_WINDOW_SERVER_TEST_BASE_H_

#include <memory>
#include <set>

#include "base/macros.h"
#include "base/test/scoped_feature_list.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"
#include "services/ui/ws/window_server_service_test_base.h"
#include "ui/aura/mus/property_converter.h"
#include "ui/aura/mus/window_manager_delegate.h"
#include "ui/aura/mus/window_tree_client_delegate.h"
#include "ui/display/screen_base.h"
#include "ui/wm/core/wm_state.h"

namespace aura {
class Env;
}

namespace ui {

// WindowServerTestBase is a base class for use with shell tests that use
// WindowServer. SetUp() connects to the WindowServer and blocks until OnEmbed()
// has been invoked. window_manager() can be used to access the WindowServer
// established as part of SetUp().
class WindowServerTestBase : public WindowServerServiceTestBase,
                             public aura::WindowTreeClientDelegate,
                             public aura::WindowManagerDelegate {
 public:
  WindowServerTestBase();
  ~WindowServerTestBase() override;

  // True if WindowTreeClientDelegate::OnLostConnection() was called.
  bool window_tree_client_lost_connection() const {
    return window_tree_client_lost_connection_;
  }

  // Runs the MessageLoop until QuitRunLoop() is called, or a timeout occurs.
  // Returns true on success. Generally prefer running a RunLoop and
  // explicitly quiting that, but use this for times when that is not possible.
  static bool DoRunLoopWithTimeout() WARN_UNUSED_RESULT;

  // Quits a run loop started by DoRunLoopWithTimeout(). Returns true on
  // success, false if a RunLoop isn't running.
  static bool QuitRunLoop() WARN_UNUSED_RESULT;

  aura::WindowTreeClient* window_manager() { return window_manager_; }
  aura::WindowManagerClient* window_manager_client() {
    return window_manager_client_;
  }

 protected:
  void set_window_manager_delegate(aura::WindowManagerDelegate* delegate) {
    window_manager_delegate_ = delegate;
  }

  // Cleans up internal state then deletes |client|.
  void DeleteWindowTreeClient(aura::WindowTreeClient* client);

  // Returns the most recent WindowTreeClient that was created as the result of
  // InterfaceFactory<WindowTreeClient> being called. In other words the most
  // recent WindowTreeClient created as the result of a client embedding.
  std::unique_ptr<aura::WindowTreeClient> ReleaseMostRecentClient();

  // testing::Test:
  void SetUp() override;
  void TearDown() override;

  // WindowServerServiceTestBase:
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  // WindowTreeClientDelegate:
  void OnEmbed(
      std::unique_ptr<aura::WindowTreeHostMus> window_tree_host) override;
  void OnLostConnection(aura::WindowTreeClient* client) override;
  void OnEmbedRootDestroyed(aura::WindowTreeHostMus* window_tree_host) override;
  void OnPointerEventObserved(const ui::PointerEvent& event,
                              int64_t display_id,
                              aura::Window* target) override;
  aura::PropertyConverter* GetPropertyConverter() override;

  // WindowManagerDelegate:
  void SetWindowManagerClient(aura::WindowManagerClient* client) override;
  void OnWmConnected() override;
  void OnWmAcceleratedWidgetAvailableForDisplay(
      int64_t display_id,
      gfx::AcceleratedWidget widget) override {}
  void OnWmSetBounds(aura::Window* window, const gfx::Rect& bounds) override;
  bool OnWmSetProperty(
      aura::Window* window,
      const std::string& name,
      std::unique_ptr<std::vector<uint8_t>>* new_data) override;
  void OnWmSetModalType(aura::Window* window, ui::ModalType type) override;
  void OnWmSetCanFocus(aura::Window* window, bool can_focus) override;
  aura::Window* OnWmCreateTopLevelWindow(
      ui::mojom::WindowType window_type,
      std::map<std::string, std::vector<uint8_t>>* properties) override;
  void OnWmClientJankinessChanged(const std::set<aura::Window*>& client_windows,
                                  bool not_responding) override;
  void OnWmBuildDragImage(const gfx::Point& screen_location,
                          const SkBitmap& drag_image,
                          const gfx::Vector2d& drag_image_offset,
                          ui::mojom::PointerKind source) override {}
  void OnWmMoveDragImage(const gfx::Point& screen_location) override {}
  void OnWmDestroyDragImage() override {}
  void OnWmWillCreateDisplay(const display::Display& display) override;
  void OnWmNewDisplay(std::unique_ptr<aura::WindowTreeHostMus> window_tree_host,
                      const display::Display& display) override;
  void OnWmDisplayRemoved(aura::WindowTreeHostMus* window_tree_host) override;
  void OnWmDisplayModified(const display::Display& display) override;
  mojom::EventResult OnAccelerator(
      uint32_t accelerator_id,
      const ui::Event& event,
      base::flat_map<std::string, std::vector<uint8_t>>* properties) override;
  void OnCursorTouchVisibleChanged(bool enabled) override;
  void OnWmPerformMoveLoop(aura::Window* window,
                           mojom::MoveLoopSource source,
                           const gfx::Point& cursor_location,
                           const base::Callback<void(bool)>& on_done) override;
  void OnWmCancelMoveLoop(aura::Window* window) override;
  void OnWmSetClientArea(
      aura::Window* window,
      const gfx::Insets& insets,
      const std::vector<gfx::Rect>& additional_client_areas) override;
  bool IsWindowActive(aura::Window* window) override;
  void OnWmDeactivateWindow(aura::Window* window) override;

  void BindWindowTreeClientRequest(
      mojom::WindowTreeClientRequest request);

 private:
  // Removes |window_tree_host| from |window_tree_hosts_| and deletes it.
  // Returns true on success, and false if not found, in which case
  // |window_tree_host| is not deleted.
  bool DeleteWindowTreeHost(aura::WindowTreeHostMus* window_tree_host);

  base::test::ScopedFeatureList feature_list_;

  std::unique_ptr<aura::Env> env_;
  ::wm::WMState wm_state_;
  display::ScreenBase screen_;
  aura::PropertyConverter property_converter_;

  std::vector<std::unique_ptr<aura::WindowTreeClient>> window_tree_clients_;

  std::vector<std::unique_ptr<aura::WindowTreeHostMus>> window_tree_hosts_;

  // The window server connection held by the window manager (app running at
  // the root window).
  aura::WindowTreeClient* window_manager_ = nullptr;

  // A test can override the WM-related behaviour by installing its own
  // WindowManagerDelegate during the test.
  aura::WindowManagerDelegate* window_manager_delegate_ = nullptr;

  aura::WindowManagerClient* window_manager_client_ = nullptr;

  bool window_tree_client_lost_connection_ = false;

  service_manager::BinderRegistry registry_;

  DISALLOW_COPY_AND_ASSIGN(WindowServerTestBase);
};

}  // namespace ui

#endif  // SERVICES_UI_WS_WINDOW_SERVER_TEST_BASE_H_
