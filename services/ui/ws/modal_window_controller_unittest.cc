// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/modal_window_controller.h"

#include "services/ui/ws/server_window.h"
#include "services/ui/ws/test_server_window_delegate.h"
#include "services/ui/ws/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ui {
namespace ws {

class ModalWindowControllerTest : public testing::Test {
 public:
  ModalWindowControllerTest() {}
  ~ModalWindowControllerTest() override {}

  VizHostProxy* viz_host_proxy() {
    return ws_test_helper_.window_server()->GetVizHostProxy();
  }

 private:
  test::WindowServerTestHelper ws_test_helper_;

  DISALLOW_COPY_AND_ASSIGN(ModalWindowControllerTest);
};

TEST_F(ModalWindowControllerTest, MinContainer) {
  TestServerWindowDelegate window_delegate(viz_host_proxy());
  ServerWindow root_window(&window_delegate, viz::FrameSinkId(1, 1));
  window_delegate.set_root_window(&root_window);
  ServerWindow container1(&window_delegate, viz::FrameSinkId(1, 2));
  ServerWindow container2(&window_delegate, viz::FrameSinkId(1, 3));
  ServerWindow container3(&window_delegate, viz::FrameSinkId(1, 4));

  root_window.SetVisible(true);
  container1.SetVisible(true);
  container2.SetVisible(true);
  container3.SetVisible(true);

  ModalWindowController modal_window_controller;

  std::vector<BlockingContainers> blocking_containers(1);
  blocking_containers[0].system_modal_container = &container3;
  blocking_containers[0].min_container = &container2;
  modal_window_controller.SetBlockingContainers(blocking_containers);

  ServerWindow window(&window_delegate, viz::FrameSinkId(1, 5));
  window.SetVisible(true);

  // Window should be blocked when not attached to hierarchy.
  EXPECT_TRUE(modal_window_controller.IsWindowBlocked(&window));

  container1.Add(&window);
  // Still blocked as still not drawn.
  EXPECT_TRUE(modal_window_controller.IsWindowBlocked(&window));

  root_window.Add(&container1);
  root_window.Add(&container2);
  root_window.Add(&container3);

  // Still blocked as window beneath min container.
  EXPECT_TRUE(modal_window_controller.IsWindowBlocked(&window));

  // container2 (and 3) are above min container, so shouldn't be blocked in
  // either of them.
  container2.Add(&window);
  EXPECT_FALSE(modal_window_controller.IsWindowBlocked(&window));

  container3.Add(&window);
  EXPECT_FALSE(modal_window_controller.IsWindowBlocked(&window));
}

TEST_F(ModalWindowControllerTest, SystemModalContainer) {
  TestServerWindowDelegate window_delegate(viz_host_proxy());
  ServerWindow root_window(&window_delegate, viz::FrameSinkId(1, 1));
  window_delegate.set_root_window(&root_window);
  ServerWindow container1(&window_delegate, viz::FrameSinkId(1, 2));
  ServerWindow container2(&window_delegate, viz::FrameSinkId(1, 3));
  ServerWindow container3(&window_delegate, viz::FrameSinkId(1, 4));

  root_window.SetVisible(true);
  container1.SetVisible(true);
  container2.SetVisible(true);
  container3.SetVisible(true);

  root_window.Add(&container1);
  root_window.Add(&container2);
  root_window.Add(&container3);

  ModalWindowController modal_window_controller;

  std::vector<BlockingContainers> blocking_containers(1);
  blocking_containers[0].system_modal_container = &container3;
  blocking_containers[0].min_container = &container2;
  modal_window_controller.SetBlockingContainers(blocking_containers);

  ServerWindow window(&window_delegate, viz::FrameSinkId(1, 5));
  window.SetVisible(true);

  container2.Add(&window);
  EXPECT_FALSE(modal_window_controller.IsWindowBlocked(&window));

  ServerWindow modal_window(&window_delegate, viz::FrameSinkId(1, 6));
  modal_window.SetModalType(MODAL_TYPE_SYSTEM);
  modal_window_controller.AddSystemModalWindow(&modal_window);

  // |window| should still not be blocked as the system modal window isn't
  // drawn.
  EXPECT_FALSE(modal_window_controller.IsWindowBlocked(&window));

  container3.Add(&modal_window);
  EXPECT_FALSE(modal_window_controller.IsWindowBlocked(&window));

  modal_window.SetVisible(true);
  EXPECT_TRUE(modal_window_controller.IsWindowBlocked(&window));

  // Move the window to container3, it should still be blocked by the sytem
  // modal.
  container3.Add(&window);
  EXPECT_TRUE(modal_window_controller.IsWindowBlocked(&window));

  // Make window a child of the modal window.
  modal_window.Add(&window);
  EXPECT_FALSE(modal_window_controller.IsWindowBlocked(&window));

  container3.Add(&window);
  modal_window.AddTransientWindow(&window);
  EXPECT_FALSE(modal_window_controller.IsWindowBlocked(&window));

  container2.Add(&window);
  EXPECT_TRUE(modal_window_controller.IsWindowBlocked(&window));

  modal_window.RemoveTransientWindow(&window);
}

}  // namespace ws
}  // namespace ui
