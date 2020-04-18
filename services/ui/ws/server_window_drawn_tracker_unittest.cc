// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/server_window_drawn_tracker.h"

#include <stddef.h>

#include "base/macros.h"
#include "services/ui/ws/server_window.h"
#include "services/ui/ws/server_window_drawn_tracker_observer.h"
#include "services/ui/ws/test_server_window_delegate.h"
#include "services/ui/ws/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ui {

namespace ws {
namespace {

class TestServerWindowDrawnTrackerObserver
    : public ServerWindowDrawnTrackerObserver {
 public:
  TestServerWindowDrawnTrackerObserver() = default;

  void clear_change_count() {
    change_count_ = 0u;
    root_will_change_count_ = 0u;
    root_did_change_count_ = 0u;
  }
  size_t change_count() const { return change_count_; }
  const ServerWindow* ancestor() const { return ancestor_; }
  const ServerWindow* window() const { return window_; }
  bool is_drawn() const { return is_drawn_; }
  size_t root_will_change_count() const { return root_will_change_count_; }
  size_t root_did_change_count() const { return root_did_change_count_; }

 private:
  // ServerWindowDrawnTrackerObserver:
  void OnDrawnStateWillChange(ServerWindow* ancestor,
                              ServerWindow* window,
                              bool is_drawn) override {
    change_count_++;
    ancestor_ = ancestor;
    window_ = window;
    is_drawn_ = is_drawn;
  }

  void OnDrawnStateChanged(ServerWindow* ancestor,
                           ServerWindow* window,
                           bool is_drawn) override {
    EXPECT_EQ(ancestor_, ancestor);
    EXPECT_EQ(window_, window);
    EXPECT_EQ(is_drawn_, is_drawn);
  }
  void OnRootWillChange(ServerWindow* ancestor, ServerWindow* window) override {
    root_will_change_count_++;
  }
  void OnRootDidChange(ServerWindow* ancestor, ServerWindow* window) override {
    root_did_change_count_++;
  }

  size_t change_count_ = 0u;
  size_t root_will_change_count_ = 0u;
  size_t root_did_change_count_ = 0u;
  const ServerWindow* ancestor_ = nullptr;
  const ServerWindow* window_ = nullptr;
  bool is_drawn_ = false;

  DISALLOW_COPY_AND_ASSIGN(TestServerWindowDrawnTrackerObserver);
};

viz::FrameSinkId MakeFrameSinkId() {
  constexpr int client_id = 1;
  static int window_id = 0;
  return viz::FrameSinkId(client_id, ++window_id);
}

}  // namespace

class ServerWindowDrawnTrackerTest : public testing::Test {
 public:
  ServerWindowDrawnTrackerTest() {}
  ~ServerWindowDrawnTrackerTest() override {}

  VizHostProxy* viz_host_proxy() {
    return ws_test_helper_.window_server()->GetVizHostProxy();
  }

 private:
  test::WindowServerTestHelper ws_test_helper_;

  DISALLOW_COPY_AND_ASSIGN(ServerWindowDrawnTrackerTest);
};

TEST_F(ServerWindowDrawnTrackerTest, ChangeBecauseOfDeletionAndVisibility) {
  TestServerWindowDelegate server_window_delegate(viz_host_proxy());
  std::unique_ptr<ServerWindow> window(
      new ServerWindow(&server_window_delegate, MakeFrameSinkId()));
  server_window_delegate.set_root_window(window.get());
  TestServerWindowDrawnTrackerObserver drawn_observer;
  ServerWindowDrawnTracker tracker(window.get(), &drawn_observer);
  window->SetVisible(true);
  EXPECT_EQ(1u, drawn_observer.change_count());
  EXPECT_EQ(0u, drawn_observer.root_will_change_count());
  EXPECT_EQ(0u, drawn_observer.root_did_change_count());
  EXPECT_EQ(window.get(), drawn_observer.window());
  EXPECT_EQ(nullptr, drawn_observer.ancestor());
  EXPECT_TRUE(drawn_observer.is_drawn());
  drawn_observer.clear_change_count();

  window->SetVisible(false);
  EXPECT_EQ(1u, drawn_observer.change_count());
  EXPECT_EQ(0u, drawn_observer.root_will_change_count());
  EXPECT_EQ(0u, drawn_observer.root_did_change_count());
  EXPECT_EQ(window.get(), drawn_observer.window());
  EXPECT_EQ(nullptr, drawn_observer.ancestor());
  EXPECT_FALSE(drawn_observer.is_drawn());
  drawn_observer.clear_change_count();

  window->SetVisible(true);
  EXPECT_EQ(1u, drawn_observer.change_count());
  EXPECT_EQ(0u, drawn_observer.root_will_change_count());
  EXPECT_EQ(0u, drawn_observer.root_did_change_count());
  EXPECT_EQ(window.get(), drawn_observer.window());
  EXPECT_EQ(nullptr, drawn_observer.ancestor());
  EXPECT_TRUE(drawn_observer.is_drawn());
  drawn_observer.clear_change_count();

  ServerWindow* old_window = window.get();
  window.reset();
  EXPECT_EQ(1u, drawn_observer.change_count());
  EXPECT_EQ(0u, drawn_observer.root_will_change_count());
  EXPECT_EQ(0u, drawn_observer.root_did_change_count());
  EXPECT_EQ(old_window, drawn_observer.window());
  EXPECT_EQ(nullptr, drawn_observer.ancestor());
  EXPECT_FALSE(drawn_observer.is_drawn());
}

TEST_F(ServerWindowDrawnTrackerTest, ChangeBecauseOfRemovingFromRoot) {
  TestServerWindowDelegate server_window_delegate(viz_host_proxy());
  ServerWindow root(&server_window_delegate, MakeFrameSinkId());
  server_window_delegate.set_root_window(&root);
  root.SetVisible(true);
  ServerWindow child(&server_window_delegate, MakeFrameSinkId());
  child.SetVisible(true);
  root.Add(&child);

  TestServerWindowDrawnTrackerObserver drawn_observer;
  ServerWindowDrawnTracker tracker(&child, &drawn_observer);
  root.Remove(&child);
  EXPECT_EQ(1u, drawn_observer.change_count());
  EXPECT_EQ(1u, drawn_observer.root_will_change_count());
  EXPECT_EQ(1u, drawn_observer.root_did_change_count());
  EXPECT_EQ(&child, drawn_observer.window());
  EXPECT_EQ(&root, drawn_observer.ancestor());
  EXPECT_FALSE(drawn_observer.is_drawn());
  drawn_observer.clear_change_count();

  root.Add(&child);
  EXPECT_EQ(1u, drawn_observer.change_count());
  EXPECT_EQ(1u, drawn_observer.root_will_change_count());
  EXPECT_EQ(1u, drawn_observer.root_did_change_count());
  EXPECT_EQ(&child, drawn_observer.window());
  EXPECT_EQ(nullptr, drawn_observer.ancestor());
  EXPECT_TRUE(drawn_observer.is_drawn());
}

TEST_F(ServerWindowDrawnTrackerTest, ChangeBecauseOfRemovingAncestorFromRoot) {
  TestServerWindowDelegate server_window_delegate(viz_host_proxy());
  ServerWindow root(&server_window_delegate, MakeFrameSinkId());
  server_window_delegate.set_root_window(&root);
  root.SetVisible(true);
  ServerWindow child(&server_window_delegate, MakeFrameSinkId());
  child.SetVisible(true);
  root.Add(&child);

  ServerWindow child_child(&server_window_delegate, MakeFrameSinkId());
  child_child.SetVisible(true);
  child.Add(&child_child);

  TestServerWindowDrawnTrackerObserver drawn_observer;
  ServerWindowDrawnTracker tracker(&child_child, &drawn_observer);
  root.Remove(&child);
  EXPECT_EQ(1u, drawn_observer.change_count());
  EXPECT_EQ(1u, drawn_observer.root_will_change_count());
  EXPECT_EQ(1u, drawn_observer.root_did_change_count());
  EXPECT_EQ(&child_child, drawn_observer.window());
  EXPECT_EQ(&root, drawn_observer.ancestor());
  EXPECT_FALSE(drawn_observer.is_drawn());
  drawn_observer.clear_change_count();

  root.Add(&child_child);
  EXPECT_EQ(1u, drawn_observer.change_count());
  EXPECT_EQ(1u, drawn_observer.root_will_change_count());
  EXPECT_EQ(1u, drawn_observer.root_did_change_count());
  EXPECT_EQ(&child_child, drawn_observer.window());
  EXPECT_EQ(nullptr, drawn_observer.ancestor());
  EXPECT_TRUE(drawn_observer.is_drawn());
}

TEST_F(ServerWindowDrawnTrackerTest, VisibilityChangeFromNonParentAncestor) {
  TestServerWindowDelegate server_window_delegate(viz_host_proxy());
  ServerWindow root(&server_window_delegate, MakeFrameSinkId());
  ServerWindow child1(&server_window_delegate, MakeFrameSinkId());
  ServerWindow child2(&server_window_delegate, MakeFrameSinkId());
  ServerWindow child3(&server_window_delegate, MakeFrameSinkId());
  server_window_delegate.set_root_window(&root);

  root.Add(&child1);
  child1.Add(&child2);
  child2.Add(&child3);

  root.SetVisible(true);
  child1.SetVisible(false);
  child2.SetVisible(false);
  child3.SetVisible(true);

  TestServerWindowDrawnTrackerObserver drawn_observer;
  ServerWindowDrawnTracker tracker(&child3, &drawn_observer);

  EXPECT_FALSE(child3.IsDrawn());

  // Make |child1| visible. |child3| should still be not drawn, since |child2|
  // is still invisible.
  child1.SetVisible(true);
  EXPECT_EQ(0u, drawn_observer.change_count());
  EXPECT_EQ(0u, drawn_observer.root_will_change_count());
  EXPECT_EQ(0u, drawn_observer.root_did_change_count());
  EXPECT_EQ(nullptr, drawn_observer.window());
  EXPECT_EQ(nullptr, drawn_observer.ancestor());
  EXPECT_FALSE(drawn_observer.is_drawn());
  EXPECT_FALSE(child3.IsDrawn());

  child2.SetVisible(true);
  EXPECT_EQ(1u, drawn_observer.change_count());
  EXPECT_EQ(0u, drawn_observer.root_will_change_count());
  EXPECT_EQ(0u, drawn_observer.root_did_change_count());
  EXPECT_EQ(&child3, drawn_observer.window());
  EXPECT_EQ(nullptr, drawn_observer.ancestor());
  EXPECT_TRUE(drawn_observer.is_drawn());
  EXPECT_TRUE(child3.IsDrawn());
}

TEST_F(ServerWindowDrawnTrackerTest, TreeHierarchyChangeFromNonParentAncestor) {
  TestServerWindowDelegate server_window_delegate(viz_host_proxy());
  ServerWindow root(&server_window_delegate, MakeFrameSinkId());
  ServerWindow child1(&server_window_delegate, MakeFrameSinkId());
  ServerWindow child2(&server_window_delegate, MakeFrameSinkId());
  ServerWindow child11(&server_window_delegate, MakeFrameSinkId());
  ServerWindow child111(&server_window_delegate, MakeFrameSinkId());
  server_window_delegate.set_root_window(&root);

  root.Add(&child1);
  root.Add(&child2);
  child1.Add(&child11);
  child11.Add(&child111);

  root.SetVisible(true);
  child1.SetVisible(false);
  child2.SetVisible(true);
  child11.SetVisible(false);
  child111.SetVisible(true);

  TestServerWindowDrawnTrackerObserver drawn_observer;
  ServerWindowDrawnTracker tracker(&child111, &drawn_observer);
  EXPECT_FALSE(child111.IsDrawn());

  // Move |child11| as a child of |child2|. |child111| should remain not drawn.
  child2.Add(&child11);
  EXPECT_EQ(0u, drawn_observer.change_count());
  EXPECT_EQ(0u, drawn_observer.root_will_change_count());
  EXPECT_EQ(0u, drawn_observer.root_did_change_count());
  EXPECT_EQ(nullptr, drawn_observer.window());
  EXPECT_EQ(nullptr, drawn_observer.ancestor());
  EXPECT_FALSE(drawn_observer.is_drawn());
  EXPECT_FALSE(child111.IsDrawn());

  child11.SetVisible(true);
  EXPECT_EQ(1u, drawn_observer.change_count());
  EXPECT_EQ(0u, drawn_observer.root_will_change_count());
  EXPECT_EQ(0u, drawn_observer.root_did_change_count());
  EXPECT_EQ(&child111, drawn_observer.window());
  EXPECT_EQ(nullptr, drawn_observer.ancestor());
  EXPECT_TRUE(drawn_observer.is_drawn());
  EXPECT_TRUE(child111.IsDrawn());
}

}  // namespace ws

}  // namespace ui
