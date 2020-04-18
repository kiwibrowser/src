// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "services/ui/ws/server_window.h"
#include "services/ui/ws/server_window_observer.h"
#include "services/ui/ws/test_server_window_delegate.h"
#include "services/ui/ws/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ui {
namespace ws {

namespace {

class TestTransientWindowObserver : public ServerWindowObserver {
 public:
  TestTransientWindowObserver() : add_count_(0), remove_count_(0) {}

  ~TestTransientWindowObserver() override {}

  int add_count() const { return add_count_; }
  int remove_count() const { return remove_count_; }

  // TransientWindowObserver overrides:
  void OnTransientWindowAdded(ServerWindow* window,
                              ServerWindow* transient) override {
    add_count_++;
  }
  void OnTransientWindowRemoved(ServerWindow* window,
                                ServerWindow* transient) override {
    remove_count_++;
  }

 private:
  int add_count_;
  int remove_count_;

  DISALLOW_COPY_AND_ASSIGN(TestTransientWindowObserver);
};

ServerWindow* CreateTestWindow(TestServerWindowDelegate* delegate,
                               const viz::FrameSinkId& frame_sink_id,
                               ServerWindow* parent) {
  ServerWindow* window = new ServerWindow(delegate, frame_sink_id);
  window->SetVisible(true);
  if (parent)
    parent->Add(window);
  else
    delegate->set_root_window(window);
  return window;
}

std::string ChildWindowIDsAsString(ServerWindow* parent) {
  std::string result;
  for (auto i = parent->children().begin(); i != parent->children().end();
       ++i) {
    if (!result.empty())
      result += " ";
    const viz::FrameSinkId& id = (*i)->frame_sink_id();
    if (id.client_id() != 0) {
      // All use cases of this expect the client_id to be 0.
      return "unexpected-client-id";
    }
    result += base::IntToString(id.sink_id());
  }
  return result;
}

}  // namespace

class TransientWindowsTest : public testing::Test {
 public:
  TransientWindowsTest() {}
  ~TransientWindowsTest() override {}

  VizHostProxy* viz_host_proxy() {
    return ws_test_helper_.window_server()->GetVizHostProxy();
  }

 private:
  test::WindowServerTestHelper ws_test_helper_;

  DISALLOW_COPY_AND_ASSIGN(TransientWindowsTest);
};

TEST_F(TransientWindowsTest, TransientChildren) {
  TestServerWindowDelegate server_window_delegate(viz_host_proxy());

  std::unique_ptr<ServerWindow> parent(CreateTestWindow(
      &server_window_delegate, viz::FrameSinkId(1, 0), nullptr));
  std::unique_ptr<ServerWindow> w1(CreateTestWindow(
      &server_window_delegate, viz::FrameSinkId(1, 1), parent.get()));
  std::unique_ptr<ServerWindow> w3(CreateTestWindow(
      &server_window_delegate, viz::FrameSinkId(1, 2), parent.get()));

  ServerWindow* w2 = CreateTestWindow(&server_window_delegate,
                                      viz::FrameSinkId(1, 3), parent.get());

  // w2 is now owned by w1.
  w1->AddTransientWindow(w2);
  // Stack w1 at the top (end), this should force w2 to be last (on top of w1).
  parent->StackChildAtTop(w1.get());

  // Destroy w1, which should also destroy w3 (since it's a transient child).
  w1.reset();
  w2 = nullptr;
  ASSERT_EQ(1u, parent->children().size());
  EXPECT_EQ(w3.get(), parent->children()[0]);
}

// Verifies adding doesn't restack at all.
TEST_F(TransientWindowsTest, DontStackUponCreation) {
  TestServerWindowDelegate delegate(viz_host_proxy());
  std::unique_ptr<ServerWindow> parent(
      CreateTestWindow(&delegate, viz::FrameSinkId(0, 1), nullptr));
  std::unique_ptr<ServerWindow> window0(
      CreateTestWindow(&delegate, viz::FrameSinkId(0, 2), parent.get()));
  std::unique_ptr<ServerWindow> window1(
      CreateTestWindow(&delegate, viz::FrameSinkId(0, 3), parent.get()));

  ServerWindow* window2 =
      CreateTestWindow(&delegate, viz::FrameSinkId(0, 4), parent.get());
  window0->AddTransientWindow(window2);
  EXPECT_EQ("2 3 4", ChildWindowIDsAsString(parent.get()));
}

// More variations around verifying ordering doesn't change when
// adding/removing transients.
TEST_F(TransientWindowsTest, RestackUponAddOrRemoveTransientWindow) {
  TestServerWindowDelegate delegate(viz_host_proxy());
  std::unique_ptr<ServerWindow> parent(
      CreateTestWindow(&delegate, viz::FrameSinkId(0, 1), nullptr));
  std::unique_ptr<ServerWindow> windows[4];
  for (int i = 0; i < 4; i++)
    windows[i].reset(
        CreateTestWindow(&delegate, viz::FrameSinkId(0, i + 2), parent.get()));

  EXPECT_EQ("2 3 4 5", ChildWindowIDsAsString(parent.get()));

  windows[0]->AddTransientWindow(windows[2].get());
  EXPECT_EQ(windows[0].get(), windows[2]->transient_parent());
  ASSERT_EQ(1u, windows[0]->transient_children().size());
  EXPECT_EQ(windows[2].get(), windows[0]->transient_children()[0]);
  EXPECT_EQ("2 3 4 5", ChildWindowIDsAsString(parent.get()));

  windows[0]->AddTransientWindow(windows[3].get());
  EXPECT_EQ(windows[0].get(), windows[3]->transient_parent());
  ASSERT_EQ(2u, windows[0]->transient_children().size());
  EXPECT_EQ(windows[2].get(), windows[0]->transient_children()[0]);
  EXPECT_EQ(windows[3].get(), windows[0]->transient_children()[1]);
  EXPECT_EQ("2 3 4 5", ChildWindowIDsAsString(parent.get()));

  windows[0]->RemoveTransientWindow(windows[2].get());
  EXPECT_EQ(nullptr, windows[2]->transient_parent());
  ASSERT_EQ(1u, windows[0]->transient_children().size());
  EXPECT_EQ(windows[3].get(), windows[0]->transient_children()[0]);
  EXPECT_EQ("2 3 4 5", ChildWindowIDsAsString(parent.get()));

  windows[0]->RemoveTransientWindow(windows[3].get());
  EXPECT_EQ(nullptr, windows[3]->transient_parent());
  ASSERT_EQ(0u, windows[0]->transient_children().size());
  EXPECT_EQ("2 3 4 5", ChildWindowIDsAsString(parent.get()));
}

// Verifies TransientWindowObserver is notified appropriately.
TEST_F(TransientWindowsTest, TransientWindowObserverNotified) {
  TestServerWindowDelegate delegate(viz_host_proxy());
  std::unique_ptr<ServerWindow> parent(
      CreateTestWindow(&delegate, viz::FrameSinkId(0, 1), nullptr));
  std::unique_ptr<ServerWindow> w1(
      CreateTestWindow(&delegate, viz::FrameSinkId(0, 2), parent.get()));

  TestTransientWindowObserver test_observer;
  parent->AddObserver(&test_observer);

  parent->AddTransientWindow(w1.get());
  EXPECT_EQ(1, test_observer.add_count());
  EXPECT_EQ(0, test_observer.remove_count());

  parent->RemoveTransientWindow(w1.get());
  EXPECT_EQ(1, test_observer.add_count());
  EXPECT_EQ(1, test_observer.remove_count());

  parent->RemoveObserver(&test_observer);
}

}  // namespace ws
}  // namespace ui
