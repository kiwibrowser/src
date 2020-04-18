// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include <string>

#include "base/macros.h"
#include "services/ui/common/types.h"
#include "services/ui/common/util.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"
#include "services/ui/ws/display_manager.h"
#include "services/ui/ws/ids.h"
#include "services/ui/ws/platform_display.h"
#include "services/ui/ws/platform_display_factory.h"
#include "services/ui/ws/server_window.h"
#include "services/ui/ws/test_utils.h"
#include "services/ui/ws/window_manager_display_root.h"
#include "services/ui/ws/window_manager_state.h"
#include "services/ui/ws/window_server.h"
#include "services/ui/ws/window_server_delegate.h"
#include "services/ui/ws/window_tree.h"
#include "services/ui/ws/window_tree_binding.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/cursor/cursor.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/rect.h"

namespace ui {
namespace ws {
namespace test {

class CursorTest : public testing::Test {
 public:
  CursorTest() {}
  ~CursorTest() override {}

  WindowServer* window_server() { return ws_test_helper_.window_server(); }
  TestWindowServerDelegate* window_server_delegate() {
    return ws_test_helper_.window_server_delegate();
  }
  ui::CursorType cursor_type() const {
    return ws_test_helper_.cursor().cursor_type();
  }

 protected:
  // testing::Test:
  void SetUp() override {
    screen_manager_.Init(window_server()->display_manager());
    screen_manager_.AddDisplay();
    AddWindowManager(window_server());
  }

  ServerWindow* GetRoot() {
    DisplayManager* display_manager = window_server()->display_manager();
    Display* display = *display_manager->displays().begin();
    return display->window_manager_display_root()->GetClientVisibleRoot();
  }

  // Create a 30x30 window where the outer 10 pixels is non-client.
  ServerWindow* BuildServerWindow() {
    DisplayManager* display_manager = window_server()->display_manager();
    Display* display = *display_manager->displays().begin();
    WindowManagerDisplayRoot* active_display_root =
        display->window_manager_display_root();
    WindowTree* tree =
        active_display_root->window_manager_state()->window_tree();
    ClientWindowId child_window_id;
    if (!NewWindowInTree(tree, &child_window_id))
      return nullptr;

    ServerWindow* w = tree->GetWindowByClientId(child_window_id);
    w->SetBounds(gfx::Rect(10, 10, 30, 30));
    w->SetClientArea(gfx::Insets(10, 10), std::vector<gfx::Rect>());
    w->SetVisible(true);

    return w;
  }

  void MoveCursorTo(const gfx::Point& p) {
    DisplayManager* display_manager = window_server()->display_manager();
    ASSERT_EQ(1u, display_manager->displays().size());
    Display* display = *display_manager->displays().begin();
    WindowManagerDisplayRoot* active_display_root =
        display->window_manager_display_root();
    ASSERT_TRUE(active_display_root);
    PointerEvent event(
        MouseEvent(ET_MOUSE_MOVED, p, p, base::TimeTicks(), 0, 0));
    ignore_result(static_cast<PlatformDisplayDelegate*>(display)
                      ->GetEventSink()
                      ->OnEventFromSource(&event));
    WindowManagerState* wms = active_display_root->window_manager_state();
    ASSERT_TRUE(WindowManagerStateTestApi(wms).AckInFlightEvent(
        mojom::EventResult::HANDLED));
  }

 private:
  WindowServerTestHelper ws_test_helper_;
  TestScreenManager screen_manager_;
  DISALLOW_COPY_AND_ASSIGN(CursorTest);
};

TEST_F(CursorTest, ChangeByMouseMove) {
  ServerWindow* win = BuildServerWindow();
  win->SetCursor(ui::CursorData(ui::CursorType::kIBeam));
  win->parent()->SetCursor(ui::CursorData(ui::CursorType::kCell));
  EXPECT_EQ(ui::CursorType::kIBeam, win->cursor().cursor_type());
  win->SetNonClientCursor(ui::CursorData(ui::CursorType::kEastResize));
  EXPECT_EQ(ui::CursorType::kEastResize,
            win->non_client_cursor().cursor_type());

  // Non client area
  MoveCursorTo(gfx::Point(15, 15));
  EXPECT_EQ(ui::CursorType::kEastResize, cursor_type());

  // Client area, which comes from win->parent().
  MoveCursorTo(gfx::Point(25, 25));
  EXPECT_EQ(ui::CursorType::kCell, cursor_type());
}

TEST_F(CursorTest, ChangeByClientAreaChange) {
  ServerWindow* win = BuildServerWindow();
  win->parent()->SetCursor(ui::CursorData(ui::CursorType::kCross));
  win->SetCursor(ui::CursorData(ui::CursorType::kIBeam));
  EXPECT_EQ(ui::CursorType::kIBeam, win->cursor().cursor_type());
  win->SetNonClientCursor(ui::CursorData(ui::CursorType::kEastResize));
  EXPECT_EQ(ui::CursorType::kEastResize,
            win->non_client_cursor().cursor_type());

  // Non client area before we move.
  MoveCursorTo(gfx::Point(15, 15));
  EXPECT_EQ(ui::CursorType::kEastResize, cursor_type());

  // Changing the client area should cause a change. The cursor for the client
  // area comes from root ancestor, which is win->parent().
  win->SetClientArea(gfx::Insets(1, 1), std::vector<gfx::Rect>());
  EXPECT_EQ(ui::CursorType::kCross, cursor_type());
}

TEST_F(CursorTest, NonClientCursorChange) {
  ServerWindow* win = BuildServerWindow();
  win->SetCursor(ui::CursorData(ui::CursorType::kIBeam));
  EXPECT_EQ(ui::CursorType::kIBeam, win->cursor().cursor_type());
  win->SetNonClientCursor(ui::CursorData(ui::CursorType::kEastResize));
  EXPECT_EQ(ui::CursorType::kEastResize,
            win->non_client_cursor().cursor_type());

  MoveCursorTo(gfx::Point(15, 15));
  EXPECT_EQ(ui::CursorType::kEastResize, cursor_type());

  win->SetNonClientCursor(ui::CursorData(ui::CursorType::kWestResize));
  EXPECT_EQ(ui::CursorType::kWestResize, cursor_type());
}

TEST_F(CursorTest, IgnoreClientCursorChangeInNonClientArea) {
  ServerWindow* win = BuildServerWindow();
  win->SetCursor(ui::CursorData(ui::CursorType::kIBeam));
  EXPECT_EQ(ui::CursorType::kIBeam, win->cursor().cursor_type());
  win->SetNonClientCursor(ui::CursorData(ui::CursorType::kEastResize));
  EXPECT_EQ(ui::CursorType::kEastResize,
            win->non_client_cursor().cursor_type());

  MoveCursorTo(gfx::Point(15, 15));
  EXPECT_EQ(ui::CursorType::kEastResize, cursor_type());

  win->SetCursor(ui::CursorData(ui::CursorType::kHelp));
  EXPECT_EQ(ui::CursorType::kEastResize, cursor_type());
}

TEST_F(CursorTest, NonClientToClientByBoundsChange) {
  ServerWindow* win = BuildServerWindow();
  win->parent()->SetCursor(ui::CursorData(ui::CursorType::kCopy));
  win->SetCursor(ui::CursorData(ui::CursorType::kIBeam));
  EXPECT_EQ(ui::CursorType::kIBeam, win->cursor().cursor_type());
  win->SetNonClientCursor(ui::CursorData(ui::CursorType::kEastResize));
  EXPECT_EQ(ui::CursorType::kEastResize,
            win->non_client_cursor().cursor_type());

  // Non client area before we move.
  MoveCursorTo(gfx::Point(15, 15));
  EXPECT_EQ(ui::CursorType::kEastResize, cursor_type());

  win->SetBounds(gfx::Rect(0, 0, 30, 30));
  EXPECT_EQ(ui::CursorType::kCopy, cursor_type());
}

}  // namespace test
}  // namespace ws
}  // namespace ui
