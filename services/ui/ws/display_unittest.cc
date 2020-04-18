// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include <string>

#include "base/macros.h"
#include "services/ui/common/types.h"
#include "services/ui/common/util.h"
#include "services/ui/display/viewport_metrics.h"
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
#include "ui/display/display.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/rect.h"

namespace ui {
namespace ws {
namespace test {
namespace {

// Returns the root ServerWindow for the specified Display.
ServerWindow* GetRootOnDisplay(WindowTree* tree, Display* display) {
  for (const ServerWindow* root : tree->roots()) {
    if (tree->GetDisplay(root) == display)
      return const_cast<ServerWindow*>(root);
  }
  return nullptr;
}

// Tracks destruction of a ServerWindow, setting a bool* to true when
// OnWindowDestroyed() is called
class ServerWindowDestructionObserver : public ServerWindowObserver {
 public:
  ServerWindowDestructionObserver(ServerWindow* window, bool* destroyed)
      : window_(window), destroyed_(destroyed) {
    window_->AddObserver(this);
  }
  ~ServerWindowDestructionObserver() override {
    if (window_)
      window_->RemoveObserver(this);
  }

  // ServerWindowObserver:
  void OnWindowDestroyed(ServerWindow* window) override {
    *destroyed_ = true;
    window_->RemoveObserver(this);
    window_ = nullptr;
  }

 private:
  ServerWindow* window_;
  bool* destroyed_;

  DISALLOW_COPY_AND_ASSIGN(ServerWindowDestructionObserver);
};

}  // namespace

// -----------------------------------------------------------------------------

class DisplayTest : public testing::Test {
 public:
  DisplayTest() {}
  ~DisplayTest() override {}

  WindowServer* window_server() { return ws_test_helper_.window_server(); }
  DisplayManager* display_manager() {
    return window_server()->display_manager();
  }
  TestWindowServerDelegate* window_server_delegate() {
    return ws_test_helper_.window_server_delegate();
  }
  TestScreenManager& screen_manager() { return screen_manager_; }
  const ui::CursorData& cursor() { return ws_test_helper_.cursor(); }

 protected:
  // testing::Test:
  void SetUp() override {
    screen_manager_.Init(window_server()->display_manager());
  }

 private:
  WindowServerTestHelper ws_test_helper_;
  TestScreenManager screen_manager_;

  DISALLOW_COPY_AND_ASSIGN(DisplayTest);
};

TEST_F(DisplayTest, CreateDisplay) {
  AddWindowManager(window_server());
  const int64_t display_id =
      screen_manager().AddDisplay(MakeDisplay(0, 0, 1024, 768, 1.0f));

  ASSERT_EQ(1u, display_manager()->displays().size());
  Display* display = display_manager()->GetDisplayById(display_id);

  // Display should have root window with correct size.
  ASSERT_NE(nullptr, display->root_window());
  EXPECT_EQ("0,0 1024x768", display->root_window()->bounds().ToString());

  // Display should have a WM root window with the correct size too.
  WindowManagerDisplayRoot* root1 = display->window_manager_display_root();
  ASSERT_NE(nullptr, root1);
  ASSERT_NE(nullptr, root1->root());
  EXPECT_EQ("0,0 1024x768", root1->root()->bounds().ToString());
}

TEST_F(DisplayTest, CreateDisplayBeforeWM) {
  // Add one display, no WM exists yet.
  const int64_t display_id =
      screen_manager().AddDisplay(MakeDisplay(0, 0, 1024, 768, 1.0f));
  EXPECT_EQ(1u, display_manager()->displays().size());

  Display* display = display_manager()->GetDisplayById(display_id);

  // Display should have root window with correct size.
  ASSERT_NE(nullptr, display->root_window());
  EXPECT_EQ("0,0 1024x768", display->root_window()->bounds().ToString());

  // There should be no WM state for display yet.
  EXPECT_EQ(nullptr, display->window_manager_display_root());

  AddWindowManager(window_server());

  // After adding a WM display should have WM state and WM root for the display.
  WindowManagerDisplayRoot* root1 = display->window_manager_display_root();
  ASSERT_NE(nullptr, root1);
  ASSERT_NE(nullptr, root1->root());
  EXPECT_EQ("0,0 1024x768", root1->root()->bounds().ToString());
}

TEST_F(DisplayTest, CreateDisplayWithDeviceScaleFactor) {
  // The display bounds should be the pixel_size / device_scale_factor.
  display::Display display = MakeDisplay(0, 0, 1024, 768, 2.0f);
  EXPECT_EQ("0,0 512x384", display.bounds().ToString());

  const int64_t display_id = screen_manager().AddDisplay(display);
  display.set_id(display_id);
  Display* ws_display = display_manager()->GetDisplayById(display_id);

  // The root ServerWindow bounds should be in PP.
  EXPECT_EQ("0,0 1024x768", ws_display->root_window()->bounds().ToString());

  // Modify the display work area to be 48 DIPs smaller.
  display::Display modified_display = display;
  gfx::Rect modified_work_area = display.work_area();
  modified_work_area.set_height(modified_work_area.height() - 48);
  modified_display.set_work_area(modified_work_area);
  screen_manager().ModifyDisplay(modified_display);

  // The display work area should have changed.
  EXPECT_EQ("0,0 512x336", ws_display->GetDisplay().work_area().ToString());

  // The root ServerWindow should still be in PP after updating the work area.
  EXPECT_EQ("0,0 1024x768", ws_display->root_window()->bounds().ToString());
}

TEST_F(DisplayTest, Destruction) {
  AddWindowManager(window_server());

  int64_t display_id = screen_manager().AddDisplay();

  ASSERT_EQ(1u, display_manager()->displays().size());
  EXPECT_EQ(1u, window_server()->num_trees());

  WindowManagerState* state = window_server()->GetWindowManagerState();
  // Destroy the tree associated with |state|. Should result in deleting
  // |state|.
  window_server()->DestroyTree(state->window_tree());
  EXPECT_EQ(1u, display_manager()->displays().size());
  EXPECT_EQ(0u, window_server()->num_trees());
  EXPECT_FALSE(window_server_delegate()->got_on_no_more_displays());
  screen_manager().RemoveDisplay(display_id);
  EXPECT_TRUE(window_server_delegate()->got_on_no_more_displays());
}

// Verifies a single tree is used for multiple displays.
TEST_F(DisplayTest, MultipleDisplays) {
  screen_manager().AddDisplay();
  screen_manager().AddDisplay();
  AddWindowManager(window_server());
  ASSERT_EQ(1u, window_server_delegate()->bindings()->size());
  TestWindowTreeBinding* window_tree_binding =
      (*window_server_delegate()->bindings())[0];
  WindowTree* tree = window_tree_binding->tree();
  ASSERT_EQ(2u, tree->roots().size());
  std::set<const ServerWindow*> roots = tree->roots();
  auto it = roots.begin();
  ServerWindow* root1 = const_cast<ServerWindow*>(*it);
  ++it;
  ServerWindow* root2 = const_cast<ServerWindow*>(*it);
  ASSERT_NE(root1, root2);
  Display* display1 = tree->GetDisplay(root1);
  WindowManagerState* display1_wms =
      display1->window_manager_display_root()->window_manager_state();
  Display* display2 = tree->GetDisplay(root2);
  WindowManagerState* display2_wms =
      display2->window_manager_display_root()->window_manager_state();
  EXPECT_EQ(display1_wms->window_tree(), display2_wms->window_tree());
}

// Assertions around destroying a secondary display.
TEST_F(DisplayTest, DestroyingDisplayDoesntDelete) {
  AddWindowManager(window_server());
  screen_manager().AddDisplay();
  const int64_t secondary_display_id = screen_manager().AddDisplay();
  ASSERT_EQ(1u, window_server_delegate()->bindings()->size());
  WindowTree* tree = (*window_server_delegate()->bindings())[0]->tree();
  ASSERT_EQ(2u, tree->roots().size());
  Display* secondary_display =
      display_manager()->GetDisplayById(secondary_display_id);
  ASSERT_TRUE(secondary_display);
  bool secondary_root_destroyed = false;
  ServerWindow* secondary_root = GetRootOnDisplay(tree, secondary_display);
  ASSERT_TRUE(secondary_root);
  ServerWindowDestructionObserver observer(secondary_root,
                                           &secondary_root_destroyed);
  ClientWindowId secondary_root_id =
      ClientWindowIdForWindow(tree, secondary_root);
  TestWindowTreeClient* tree_client =
      static_cast<TestWindowTreeClient*>(tree->client());
  tree_client->tracker()->changes()->clear();
  TestWindowManager* test_window_manager =
      window_server_delegate()->last_binding()->window_manager();
  EXPECT_FALSE(test_window_manager->got_display_removed());
  screen_manager().RemoveDisplay(secondary_display_id);

  // Destroying the display should result in the following:
  // . The WindowManager should be told it was removed with the right id.
  EXPECT_TRUE(test_window_manager->got_display_removed());
  EXPECT_EQ(secondary_display_id, test_window_manager->display_removed_id());
  EXPECT_FALSE(secondary_root_destroyed);
  // The window should still be valid on the server side.
  ASSERT_TRUE(tree->GetWindowByClientId(secondary_root_id));
  // No changes.
  ASSERT_EQ(0u, tree_client->tracker()->changes()->size());

  // The window should be destroyed when the client says so.
  ASSERT_TRUE(tree->DeleteWindow(secondary_root_id));
  EXPECT_TRUE(secondary_root_destroyed);
}

}  // namespace test
}  // namespace ws
}  // namespace ui
