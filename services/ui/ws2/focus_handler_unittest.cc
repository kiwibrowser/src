// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws2/focus_handler.h"

#include <stdint.h>

#include <memory>
#include <queue>

#include "services/ui/public/interfaces/window_tree_constants.mojom.h"
#include "services/ui/ws2/event_test_utils.h"
#include "services/ui/ws2/window_service.h"
#include "services/ui/ws2/window_service_client_test_helper.h"
#include "services/ui/ws2/window_service_test_setup.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/layout_manager.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/window.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/events/test/event_generator.h"
#include "ui/wm/core/focus_controller.h"

namespace ui {
namespace ws2 {
namespace {

TEST(FocusHandlerTest, FocusTopLevel) {
  aura::test::TestWindowDelegate test_window_delegate;
  WindowServiceTestSetup setup;
  test_window_delegate.set_can_focus(true);
  setup.delegate()->set_delegate_for_next_top_level(&test_window_delegate);
  aura::Window* top_level = setup.client_test_helper()->NewTopLevelWindow(1);
  ASSERT_TRUE(top_level);

  // SetFocus() should fail as |top_level| isn't visible.
  EXPECT_FALSE(setup.client_test_helper()->SetFocus(top_level));

  top_level->Show();
  EXPECT_TRUE(setup.client_test_helper()->SetFocus(top_level));
  EXPECT_TRUE(top_level->HasFocus());
}

TEST(FocusHandlerTest, FocusChild) {
  aura::test::TestWindowDelegate test_window_delegate;
  WindowServiceTestSetup setup;
  test_window_delegate.set_can_focus(true);
  setup.delegate()->set_delegate_for_next_top_level(&test_window_delegate);
  aura::Window* top_level = setup.client_test_helper()->NewTopLevelWindow(1);
  ASSERT_TRUE(top_level);
  top_level->Show();
  aura::Window* window = setup.client_test_helper()->NewWindow(3);
  ASSERT_TRUE(window);

  // SetFocus() should fail as |window| isn't parented yet.
  EXPECT_FALSE(setup.client_test_helper()->SetFocus(window));

  top_level->AddChild(window);
  // SetFocus() should still fail as |window| isn't visible.
  EXPECT_FALSE(setup.client_test_helper()->SetFocus(window));
  setup.client_test_helper()->SetCanFocus(window, false);
  window->Show();

  // SetFocus() should fail as SetCanFocus(false) was called.
  EXPECT_FALSE(setup.client_test_helper()->SetFocus(window));

  setup.client_test_helper()->SetCanFocus(window, true);
  EXPECT_TRUE(setup.client_test_helper()->SetFocus(window));
}

TEST(FocusHandlerTest, NotifyOnFocusChange) {
  aura::test::TestWindowDelegate test_window_delegate;
  WindowServiceTestSetup setup;
  test_window_delegate.set_can_focus(true);
  setup.delegate()->set_delegate_for_next_top_level(&test_window_delegate);
  aura::Window* top_level = setup.client_test_helper()->NewTopLevelWindow(1);
  ASSERT_TRUE(top_level);
  top_level->Show();
  aura::Window* window1 = setup.client_test_helper()->NewWindow(3);
  ASSERT_TRUE(window1);
  top_level->AddChild(window1);
  window1->Show();
  aura::Window* window2 = setup.client_test_helper()->NewWindow(4);
  ASSERT_TRUE(window2);
  top_level->AddChild(window2);
  window2->Show();
  setup.changes()->clear();

  // Window is parented and visible, so SetFocus() should succeed.
  EXPECT_TRUE(setup.client_test_helper()->SetFocus(window1));
  // As the client originated the request it is not notified of the change.
  EXPECT_TRUE(setup.changes()->empty());

  // Focus |window2| locally (not from the client), which should result in
  // notifying the client.
  window2->Focus();
  EXPECT_TRUE(window2->HasFocus());
  EXPECT_EQ("Focused id=0,4", SingleChangeToDescription(*setup.changes()));
}

TEST(FocusHandlerTest, FocusChangeFromEmbedded) {
  aura::test::TestWindowDelegate test_window_delegate;
  WindowServiceTestSetup setup;
  test_window_delegate.set_can_focus(true);
  setup.delegate()->set_delegate_for_next_top_level(&test_window_delegate);
  aura::Window* top_level = setup.client_test_helper()->NewTopLevelWindow(1);
  ASSERT_TRUE(top_level);
  top_level->Show();
  aura::Window* embed_window = setup.client_test_helper()->NewWindow(3);
  ASSERT_TRUE(embed_window);
  top_level->AddChild(embed_window);
  embed_window->Show();
  std::unique_ptr<EmbeddingHelper> embedding_helper =
      setup.CreateEmbedding(embed_window);
  setup.changes()->clear();
  embedding_helper->changes()->clear();

  // Set focus from the embedded client.
  EXPECT_TRUE(embedding_helper->client_test_helper->SetFocus(embed_window));
  EXPECT_TRUE(embed_window->HasFocus());
  EXPECT_TRUE(setup.changes()->empty());
  EXPECT_TRUE(embedding_helper->changes()->empty());

  // Send an event, the embedded client should get it.
  test::EventGenerator event_generator(setup.root());
  event_generator.PressKey(VKEY_A, EF_NONE);
  EXPECT_TRUE(setup.changes()->empty());
  EXPECT_EQ(
      "KEY_PRESSED",
      EventToEventType(
          embedding_helper->window_tree_client.PopInputEvent().event.get()));
  EXPECT_TRUE(embedding_helper->window_tree_client.input_events().empty());
  embedding_helper->changes()->clear();

  // Set focus from the parent. The embedded client should lose focus.
  setup.client_test_helper()->SetFocus(embed_window);
  EXPECT_TRUE(embed_window->HasFocus());
  EXPECT_TRUE(setup.changes()->empty());
  EXPECT_EQ("Focused id=null",
            SingleChangeToDescription(*embedding_helper->changes()));
  embedding_helper->changes()->clear();

  // And events should now target the parent.
  event_generator.PressKey(VKEY_B, EF_NONE);
  EXPECT_EQ("KEY_PRESSED",
            EventToEventType(
                setup.window_tree_client()->PopInputEvent().event.get()));
  EXPECT_TRUE(embedding_helper->changes()->empty());
}

TEST(FocusHandlerTest, EmbedderGetsInterceptedKeyEvents) {
  aura::test::TestWindowDelegate test_window_delegate;
  const bool intercepts_events = true;
  WindowServiceTestSetup setup(intercepts_events);
  test_window_delegate.set_can_focus(true);
  setup.delegate()->set_delegate_for_next_top_level(&test_window_delegate);
  aura::Window* top_level = setup.client_test_helper()->NewTopLevelWindow(1);
  ASSERT_TRUE(top_level);
  top_level->Show();
  aura::Window* embed_window = setup.client_test_helper()->NewWindow(3);
  ASSERT_TRUE(embed_window);
  top_level->AddChild(embed_window);
  embed_window->Show();

  std::unique_ptr<EmbeddingHelper> embedding_helper = setup.CreateEmbedding(
      embed_window, mojom::kEmbedFlagEmbedderInterceptsEvents);
  ASSERT_TRUE(embedding_helper);
  aura::Window* embed_child_window =
      embedding_helper->client_test_helper->NewWindow(4);
  ASSERT_TRUE(embed_child_window);
  embed_child_window->Show();
  embed_window->AddChild(embed_child_window);
  setup.changes()->clear();
  embedding_helper->changes()->clear();

  // Set focus from the embedded client.
  EXPECT_TRUE(
      embedding_helper->client_test_helper->SetFocus(embed_child_window));
  EXPECT_TRUE(embed_child_window->HasFocus());

  // Generate a key-press. Even though focus is on a window in the embedded
  // client, the event goes to the embedder because it intercepts events.
  test::EventGenerator event_generator(setup.root());
  event_generator.PressKey(VKEY_A, EF_NONE);
  EXPECT_TRUE(embedding_helper->changes()->empty());
  EXPECT_TRUE(embedding_helper->window_tree_client.input_events().empty());
  EXPECT_EQ("KEY_PRESSED",
            EventToEventType(
                setup.window_tree_client()->PopInputEvent().event.get()));
}

}  // namespace
}  // namespace ws2
}  // namespace ui
