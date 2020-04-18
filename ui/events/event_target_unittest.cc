// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/event_target.h"

#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/test/events_test_utils.h"
#include "ui/events/test/test_event_handler.h"
#include "ui/events/test/test_event_processor.h"
#include "ui/events/test/test_event_target.h"

namespace ui {

namespace {

TEST(EventTargetTest, AddsAndRemovesHandlers) {
  test::TestEventTarget target;
  EventTargetTestApi test_api(&target);
  test::TestEventHandler handler;
  EventHandlerList list;

  // Try at the default priority
  target.AddPreTargetHandler(&handler);
  list = test_api.GetPreTargetHandlers();
  ASSERT_EQ(1u, list.size());
  target.RemovePreTargetHandler(&handler);
  list = test_api.GetPreTargetHandlers();
  ASSERT_EQ(0u, list.size());

  // Try at a different priority
  target.AddPreTargetHandler(&handler, EventTarget::Priority::kAccessibility);
  list = test_api.GetPreTargetHandlers();
  ASSERT_EQ(1u, list.size());
  target.RemovePreTargetHandler(&handler);
  list = test_api.GetPreTargetHandlers();
  ASSERT_EQ(0u, list.size());

  // Doesn't crash if we remove a handler that doesn't exist.
  target.RemovePreTargetHandler(&handler);
}

TEST(EventTargetTest, HandlerOrdering) {
  test::TestEventTarget target;
  EventTargetTestApi test_api(&target);
  test::TestEventHandler default_handler;
  test::TestEventHandler system_handler;
  test::TestEventHandler a11y_handler;
  EventHandlerList list;

  // Try adding default then system then a11y, which is backwards of the
  // desired order.
  target.AddPreTargetHandler(&default_handler, EventTarget::Priority::kDefault);
  target.AddPreTargetHandler(&system_handler, EventTarget::Priority::kSystem);
  target.AddPreTargetHandler(&a11y_handler,
                             EventTarget::Priority::kAccessibility);
  list = test_api.GetPreTargetHandlers();
  ASSERT_EQ(3u, list.size());
  EXPECT_EQ(list[0], &a11y_handler);
  EXPECT_EQ(list[1], &system_handler);
  EXPECT_EQ(list[2], &default_handler);
}

TEST(EventTargetTest, HandlerOrderingComplex) {
  test::TestEventTarget target;
  EventTargetTestApi test_api(&target);
  test::TestEventHandler default_handler_1;
  test::TestEventHandler default_handler_2;
  test::TestEventHandler system_handler_1;
  test::TestEventHandler system_handler_2;
  test::TestEventHandler system_handler_3;
  test::TestEventHandler a11y_handler_1;
  test::TestEventHandler a11y_handler_2;
  EventHandlerList list;

  // Adding a new system or accessibility handler will insert it before others
  // of its type. Adding a new default handler puts it at the end of the list,
  // but this will change in a later patch set.
  target.AddPreTargetHandler(&system_handler_3, EventTarget::Priority::kSystem);
  target.AddPreTargetHandler(&default_handler_2,
                             EventTarget::Priority::kDefault);
  target.AddPreTargetHandler(&system_handler_2, EventTarget::Priority::kSystem);
  target.AddPreTargetHandler(&a11y_handler_2,
                             EventTarget::Priority::kAccessibility);
  target.AddPreTargetHandler(&system_handler_1, EventTarget::Priority::kSystem);
  target.AddPreTargetHandler(&default_handler_1,
                             EventTarget::Priority::kDefault);
  target.AddPreTargetHandler(&a11y_handler_1,
                             EventTarget::Priority::kAccessibility);
  list = test_api.GetPreTargetHandlers();
  ASSERT_EQ(7u, list.size());
  EXPECT_EQ(list[0], &a11y_handler_1);
  EXPECT_EQ(list[1], &a11y_handler_2);
  EXPECT_EQ(list[2], &system_handler_1);
  EXPECT_EQ(list[3], &system_handler_2);
  EXPECT_EQ(list[4], &system_handler_3);
  EXPECT_EQ(list[5], &default_handler_2);
  EXPECT_EQ(list[6], &default_handler_1);
}

}  // namespace

}  // namespace ui
