// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/focus_controller.h"

#include <stddef.h>
#include <stdint.h>

#include "base/macros.h"
#include "services/ui/ws/focus_controller_observer.h"
#include "services/ui/ws/server_window.h"
#include "services/ui/ws/test_server_window_delegate.h"
#include "services/ui/ws/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ui {

namespace ws {
namespace {

class TestFocusControllerObserver : public FocusControllerObserver {
 public:
  TestFocusControllerObserver()
      : ignore_explicit_(true),
        focus_change_count_(0u),
        old_focused_window_(nullptr),
        new_focused_window_(nullptr),
        old_active_window_(nullptr),
        new_active_window_(nullptr) {}

  void ClearAll() {
    focus_change_count_ = 0u;
    old_focused_window_ = nullptr;
    new_focused_window_ = nullptr;
    old_active_window_ = nullptr;
    new_active_window_ = nullptr;
  }
  size_t focus_change_count() const { return focus_change_count_; }
  ServerWindow* old_focused_window() { return old_focused_window_; }
  ServerWindow* new_focused_window() { return new_focused_window_; }

  ServerWindow* old_active_window() { return old_active_window_; }
  ServerWindow* new_active_window() { return new_active_window_; }

  void set_ignore_explicit(bool ignore) { ignore_explicit_ = ignore; }

 private:
  // FocusControllerObserver:
  void OnActivationChanged(ServerWindow* old_active_window,
                           ServerWindow* new_active_window) override {
    old_active_window_ = old_active_window;
    new_active_window_ = new_active_window;
  }
  void OnFocusChanged(FocusControllerChangeSource source,
                      ServerWindow* old_focused_window,
                      ServerWindow* new_focused_window) override {
    if (ignore_explicit_ && source == FocusControllerChangeSource::EXPLICIT)
      return;

    focus_change_count_++;
    old_focused_window_ = old_focused_window;
    new_focused_window_ = new_focused_window;
  }

  bool ignore_explicit_;
  size_t focus_change_count_;
  ServerWindow* old_focused_window_;
  ServerWindow* new_focused_window_;
  ServerWindow* old_active_window_;
  ServerWindow* new_active_window_;

  DISALLOW_COPY_AND_ASSIGN(TestFocusControllerObserver);
};

}  // namespace

class FocusControllerTest : public testing::Test {
 public:
  FocusControllerTest() {}
  ~FocusControllerTest() override {}

  VizHostProxy* viz_host_proxy() {
    return ws_test_helper_.window_server()->GetVizHostProxy();
  }

 private:
  test::WindowServerTestHelper ws_test_helper_;

  DISALLOW_COPY_AND_ASSIGN(FocusControllerTest);
};

TEST_F(FocusControllerTest, Basic) {
  TestServerWindowDelegate server_window_delegate(viz_host_proxy());
  ServerWindow root(&server_window_delegate, viz::FrameSinkId(1, 1));
  server_window_delegate.set_root_window(&root);
  root.SetVisible(true);
  root.set_is_activation_parent(true);
  ServerWindow child(&server_window_delegate, viz::FrameSinkId(1, 2));
  child.SetVisible(true);
  child.set_is_activation_parent(true);
  root.Add(&child);
  ServerWindow child_child(&server_window_delegate, viz::FrameSinkId(1, 3));
  child_child.SetVisible(true);
  child.Add(&child_child);
  child_child.set_is_activation_parent(true);

  // Sibling of |child|.
  ServerWindow child2(&server_window_delegate, viz::FrameSinkId(1, 4));
  child2.SetVisible(true);
  root.Add(&child2);

  TestFocusControllerObserver focus_observer;
  FocusController focus_controller(&root);
  focus_controller.AddObserver(&focus_observer);

  focus_controller.SetFocusedWindow(&child_child);
  EXPECT_EQ(0u, focus_observer.focus_change_count());

  // Remove the ancestor of the focused window, focus should go to |child2|.
  root.Remove(&child);
  EXPECT_EQ(1u, focus_observer.focus_change_count());
  EXPECT_EQ(&child2, focus_observer.new_focused_window());
  EXPECT_EQ(&child_child, focus_observer.old_focused_window());
  focus_observer.ClearAll();

  // Make the focused window invisible. Focus is lost in this case (as no one
  // to give focus to).
  child2.SetVisible(false);
  EXPECT_EQ(1u, focus_observer.focus_change_count());
  EXPECT_EQ(nullptr, focus_observer.new_focused_window());
  EXPECT_EQ(&child2, focus_observer.old_focused_window());
  focus_observer.ClearAll();

  // Go back to initial state and focus |child_child|.
  child2.SetVisible(false);
  root.Add(&child);
  focus_controller.SetFocusedWindow(&child_child);
  EXPECT_EQ(0u, focus_observer.focus_change_count());

  // Hide the focused window, focus should go to parent.
  child_child.SetVisible(false);
  EXPECT_EQ(1u, focus_observer.focus_change_count());
  EXPECT_EQ(&child, focus_observer.new_focused_window());
  EXPECT_EQ(&child_child, focus_observer.old_focused_window());
  focus_observer.ClearAll();

  child_child.SetVisible(true);
  focus_controller.SetFocusedWindow(&child_child);
  EXPECT_EQ(0u, focus_observer.focus_change_count());

  // Hide the parent of the focused window, focus is lost as there no visible
  // windows to move focus to.
  child.SetVisible(false);
  EXPECT_EQ(1u, focus_observer.focus_change_count());
  EXPECT_EQ(nullptr, focus_observer.new_focused_window());
  EXPECT_EQ(&child_child, focus_observer.old_focused_window());
  focus_observer.ClearAll();
  focus_controller.RemoveObserver(&focus_observer);
}

namespace {

// ServerWindowDelegate implementation whose GetRootWindow() implementation
// returns the last ancestor as the root of a window.
class TestServerWindowDelegate2 : public ServerWindowDelegate {
 public:
  explicit TestServerWindowDelegate2(VizHostProxy* viz_host_proxy)
      : viz_host_proxy_(viz_host_proxy) {}
  ~TestServerWindowDelegate2() override = default;

  // ServerWindowDelegate:
  VizHostProxy* GetVizHostProxy() override { return viz_host_proxy_; }
  ServerWindow* GetRootWindowForDrawn(const ServerWindow* window) override {
    const ServerWindow* root = window;
    while (root && root->parent())
      root = root->parent();
    // TODO(sky): this cast shouldn't be necessary!
    return const_cast<ServerWindow*>(root);
  }
  void OnFirstSurfaceActivation(const viz::SurfaceInfo& surface_info,
                                ServerWindow* window) override {}

 private:
  VizHostProxy* viz_host_proxy_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(TestServerWindowDelegate2);
};

}  // namespace

TEST_F(FocusControllerTest, ActiveWindowMovesToDifferentDisplay) {
  TestServerWindowDelegate2 server_window_delegate(viz_host_proxy());
  ServerWindow root1(&server_window_delegate, viz::FrameSinkId(1, 1));
  root1.SetVisible(true);
  root1.set_is_activation_parent(true);
  ServerWindow root2(&server_window_delegate, viz::FrameSinkId(1, 2));
  root2.SetVisible(true);
  root2.set_is_activation_parent(true);

  ServerWindow child(&server_window_delegate, viz::FrameSinkId(1, 3));
  root1.Add(&child);
  child.SetVisible(true);
  child.set_is_activation_parent(true);

  ServerWindow child_child(&server_window_delegate, viz::FrameSinkId(1, 4));
  child.Add(&child_child);
  child_child.SetVisible(true);

  TestFocusControllerObserver focus_observer;
  focus_observer.set_ignore_explicit(false);
  FocusController focus_controller(&root1);
  focus_controller.AddObserver(&focus_observer);

  focus_controller.SetFocusedWindow(&child_child);
  EXPECT_EQ(&child_child, focus_controller.GetFocusedWindow());

  root2.Add(&child_child);
  // As the focused window is moving to a different root focus should move
  // to the parent of the old focused window.
  EXPECT_EQ(&child, focus_controller.GetFocusedWindow());
}

}  // namespace ws
}  // namespace ui
