// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/custom/custom_element_reaction_queue.h"

#include <initializer_list>
#include <vector>
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/html/custom/custom_element_reaction.h"
#include "third_party/blink/renderer/core/html/custom/custom_element_reaction_test_helpers.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

TEST(CustomElementReactionQueueTest, invokeReactions_one) {
  std::vector<char> log;
  CustomElementReactionQueue* queue = new CustomElementReactionQueue();
  queue->Add(new TestReaction({new Log('a', log)}));
  queue->InvokeReactions(nullptr);
  EXPECT_EQ(log, std::vector<char>({'a'}))
      << "the reaction should have been invoked";
}

TEST(CustomElementReactionQueueTest, invokeReactions_many) {
  std::vector<char> log;
  CustomElementReactionQueue* queue = new CustomElementReactionQueue();
  queue->Add(new TestReaction({new Log('a', log)}));
  queue->Add(new TestReaction({new Log('b', log)}));
  queue->Add(new TestReaction({new Log('c', log)}));
  queue->InvokeReactions(nullptr);
  EXPECT_EQ(log, std::vector<char>({'a', 'b', 'c'}))
      << "the reaction should have been invoked";
}

TEST(CustomElementReactionQueueTest, invokeReactions_recursive) {
  std::vector<char> log;
  CustomElementReactionQueue* queue = new CustomElementReactionQueue();

  CustomElementReaction* third = new TestReaction(
      {new Log('c', log), new Recurse(queue)});  // "Empty" recursion

  CustomElementReaction* second = new TestReaction(
      {new Log('b', log),
       new Enqueue(queue, third)});  // Unwinds one level of recursion

  CustomElementReaction* first =
      new TestReaction({new Log('a', log), new Enqueue(queue, second),
                        new Recurse(queue)});  // Non-empty recursion

  queue->Add(first);
  queue->InvokeReactions(nullptr);
  EXPECT_EQ(log, std::vector<char>({'a', 'b', 'c'}))
      << "the reactions should have been invoked";
}

TEST(CustomElementReactionQueueTest, clear_duringInvoke) {
  std::vector<char> log;
  CustomElementReactionQueue* queue = new CustomElementReactionQueue();

  queue->Add(new TestReaction({new Log('a', log)}));
  queue->Add(new TestReaction({new Call(WTF::Bind(
      [](CustomElementReactionQueue* queue, Element*) { queue->Clear(); },
      WrapPersistent(queue)))}));
  queue->Add(new TestReaction({new Log('b', log)}));

  queue->InvokeReactions(nullptr);
  EXPECT_EQ(log, std::vector<char>({'a'}))
      << "only 'a' should be logged; the second log should have been cleared";
}

}  // namespace blink
