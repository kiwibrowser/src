// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_TEST_AURA_TEST_BASE_H_
#define UI_AURA_TEST_AURA_TEST_BASE_H_

#include <memory>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/test/scoped_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/mus/property_converter.h"
#include "ui/aura/mus/window_manager_delegate.h"
#include "ui/aura/mus/window_tree_client_delegate.h"
#include "ui/aura/test/aura_test_helper.h"

namespace ui {
namespace mojom {
class WindowTreeClient;
}
}

namespace aura {
class Window;
class WindowDelegate;
class WindowManagerDelegate;
class WindowTreeClientDelegate;

namespace client {
class FocusClient;
}

namespace test {

class AuraTestContextFactory;

// TODO(sky): remove MUS. https://crbug.com/842365.
// MUS2 targets ws2. See WindowTreeClient::Config::kMus2 for details.
enum class BackendType { CLASSIC, MUS, MUS2 };

// A base class for aura unit tests.
// TODO(beng): Instances of this test will create and own a RootWindow.
class AuraTestBase : public testing::Test,
                     public WindowTreeClientDelegate,
                     public WindowManagerDelegate {
 public:
  AuraTestBase();
  ~AuraTestBase() override;

  // testing::Test:
  void SetUp() override;
  void TearDown() override;

  // Creates a normal window parented to |parent|.
  aura::Window* CreateNormalWindow(int id, Window* parent,
                                   aura::WindowDelegate* delegate);

 protected:
  void set_window_manager_delegate(
      WindowManagerDelegate* window_manager_delegate) {
    window_manager_delegate_ = window_manager_delegate;
  }

  void set_window_tree_client_delegate(
      WindowTreeClientDelegate* window_tree_client_delegate) {
    window_tree_client_delegate_ = window_tree_client_delegate;
  }

  // Turns on mus with a test WindowTree. Must be called before SetUp().
  void EnableMusWithTestWindowTree();

  // Deletes the WindowTreeClient now. Normally the WindowTreeClient is deleted
  // at the right time and there is no need to call this. This is provided for
  // testing shutdown ordering.
  void DeleteWindowTreeClient();

  // Used to configure the backend. This is exposed to make parameterized tests
  // easy to write. This *must* be called from SetUp().
  void ConfigureBackend(BackendType type);

  void RunAllPendingInMessageLoop();

  void ParentWindow(Window* window);

  // A convenience function for dispatching an event to |dispatcher()|.
  // Returns whether |event| was handled.
  bool DispatchEventUsingWindowDispatcher(ui::Event* event);

  Window* root_window() { return helper_->root_window(); }
  WindowTreeHost* host() { return helper_->host(); }
  ui::EventSink* event_sink() { return helper_->event_sink(); }
  TestScreen* test_screen() { return helper_->test_screen(); }
  client::FocusClient* focus_client() { return helper_->focus_client(); }

  TestWindowTree* window_tree() { return helper_->window_tree(); }
  WindowTreeClient* window_tree_client_impl() {
    return helper_->window_tree_client();
  }
  ui::mojom::WindowTreeClient* window_tree_client();

  std::vector<std::unique_ptr<ui::PointerEvent>>& observed_pointer_events() {
    return observed_pointer_events_;
  }

  // WindowTreeClientDelegate:
  void OnEmbed(std::unique_ptr<WindowTreeHostMus> window_tree_host) override;
  void OnUnembed(Window* root) override;
  void OnEmbedRootDestroyed(WindowTreeHostMus* window_tree_host) override;
  void OnLostConnection(WindowTreeClient* client) override;
  void OnPointerEventObserved(const ui::PointerEvent& event,
                              int64_t display_id,
                              Window* target) override;

  // WindowManagerDelegate:
  void SetWindowManagerClient(WindowManagerClient* client) override;
  void OnWmConnected() override;
  void OnWmAcceleratedWidgetAvailableForDisplay(
      int64_t display_id,
      gfx::AcceleratedWidget widget) override {}
  void OnWmSetBounds(Window* window, const gfx::Rect& bounds) override;
  bool OnWmSetProperty(
      Window* window,
      const std::string& name,
      std::unique_ptr<std::vector<uint8_t>>* new_data) override;
  void OnWmSetModalType(Window* window, ui::ModalType type) override;
  void OnWmSetCanFocus(Window* window, bool can_focus) override;
  Window* OnWmCreateTopLevelWindow(
      ui::mojom::WindowType window_type,
      std::map<std::string, std::vector<uint8_t>>* properties) override;
  void OnWmClientJankinessChanged(const std::set<Window*>& client_windows,
                                  bool janky) override;
  void OnWmBuildDragImage(const gfx::Point& cursor_location,
                          const SkBitmap& drag_image,
                          const gfx::Vector2d& drag_image_offset,
                          ui::mojom::PointerKind source) override {}
  void OnWmMoveDragImage(const gfx::Point& cursor_location) override {}
  void OnWmDestroyDragImage() override {}
  void OnWmWillCreateDisplay(const display::Display& display) override;
  void OnWmNewDisplay(std::unique_ptr<WindowTreeHostMus> window_tree_host,
                      const display::Display& display) override;
  void OnWmDisplayRemoved(WindowTreeHostMus* window_tree_host) override;
  void OnWmDisplayModified(const display::Display& display) override;
  ui::mojom::EventResult OnAccelerator(
      uint32_t id,
      const ui::Event& event,
      base::flat_map<std::string, std::vector<uint8_t>>* properties) override;
  void OnCursorTouchVisibleChanged(bool enabled) override;
  void OnWmPerformMoveLoop(Window* window,
                           ui::mojom::MoveLoopSource source,
                           const gfx::Point& cursor_location,
                           const base::Callback<void(bool)>& on_done) override;
  void OnWmCancelMoveLoop(Window* window) override;
  void OnWmSetClientArea(
      Window* window,
      const gfx::Insets& insets,
      const std::vector<gfx::Rect>& additional_client_areas) override;
  bool IsWindowActive(aura::Window* window) override;
  void OnWmDeactivateWindow(Window* window) override;
  PropertyConverter* GetPropertyConverter() override;

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  // Only used for mus. Both are are initialized to this, but may be reset.
  WindowManagerDelegate* window_manager_delegate_;
  WindowTreeClientDelegate* window_tree_client_delegate_;

  BackendType backend_type_ = BackendType::CLASSIC;
  bool setup_called_ = false;
  bool teardown_called_ = false;
  PropertyConverter property_converter_;
  std::unique_ptr<AuraTestHelper> helper_;
  std::unique_ptr<AuraTestContextFactory> mus_context_factory_;
  std::vector<std::unique_ptr<WindowTreeHostMus>> window_tree_hosts_;
  std::vector<std::unique_ptr<ui::PointerEvent>> observed_pointer_events_;

  DISALLOW_COPY_AND_ASSIGN(AuraTestBase);
};

// Use as a base class for tests that want to target both backends.
class AuraTestBaseWithType : public AuraTestBase,
                             public ::testing::WithParamInterface<BackendType> {
 public:
  AuraTestBaseWithType();
  ~AuraTestBaseWithType() override;

  // AuraTestBase:
  void SetUp() override;

 private:
  bool setup_called_ = false;

  DISALLOW_COPY_AND_ASSIGN(AuraTestBaseWithType);
};

class AuraTestBaseMus : public AuraTestBase {
 public:
  AuraTestBaseMus();
  ~AuraTestBaseMus() override;

  // AuraTestBase:
  void SetUp() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AuraTestBaseMus);
};

}  // namespace test
}  // namespace aura

#endif  // UI_AURA_TEST_AURA_TEST_BASE_H_
