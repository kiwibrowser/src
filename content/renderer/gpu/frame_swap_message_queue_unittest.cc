// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/gpu/frame_swap_message_queue.h"

#include <utility>

#include "ipc/ipc_message.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class FrameSwapMessageQueueTest : public testing::Test {
 public:
  FrameSwapMessageQueueTest()
      : first_message_(41, 1, IPC::Message::PRIORITY_NORMAL),
        second_message_(42, 2, IPC::Message::PRIORITY_NORMAL),
        third_message_(43, 3, IPC::Message::PRIORITY_NORMAL),
        queue_(new FrameSwapMessageQueue(0)) {}

 protected:
  void QueueNextSwapMessage(std::unique_ptr<IPC::Message> msg) {
    queue_->QueueMessageForFrame(MESSAGE_DELIVERY_POLICY_WITH_NEXT_SWAP, 0,
                                 std::move(msg), nullptr);
  }

  void QueueNextSwapMessage(std::unique_ptr<IPC::Message> msg, bool* first) {
    queue_->QueueMessageForFrame(MESSAGE_DELIVERY_POLICY_WITH_NEXT_SWAP, 0,
                                 std::move(msg), first);
  }

  void QueueVisualStateMessage(int source_frame_number,
                               std::unique_ptr<IPC::Message> msg) {
    queue_->QueueMessageForFrame(MESSAGE_DELIVERY_POLICY_WITH_VISUAL_STATE,
                                 source_frame_number, std::move(msg), nullptr);
  }

  void QueueVisualStateMessage(int source_frame_number,
                               std::unique_ptr<IPC::Message> msg,
                               bool* first) {
    queue_->QueueMessageForFrame(MESSAGE_DELIVERY_POLICY_WITH_VISUAL_STATE,
                                 source_frame_number, std::move(msg), first);
  }

  void DrainMessages(int source_frame_number,
                     std::vector<std::unique_ptr<IPC::Message>>* messages) {
    messages->clear();
    queue_->DidActivate(source_frame_number);
    queue_->DidSwap(source_frame_number);
    std::unique_ptr<FrameSwapMessageQueue::SendMessageScope>
        send_message_scope = queue_->AcquireSendMessageScope();
    queue_->DrainMessages(messages);
  }

  bool HasMessageForId(
      const std::vector<std::unique_ptr<IPC::Message>>& messages,
      int routing_id) {
    for (const auto& msg : messages) {
      if (msg->routing_id() == routing_id)
        return true;
    }
    return false;
  }

  std::unique_ptr<IPC::Message> CloneMessage(const IPC::Message& other) {
    return std::make_unique<IPC::Message>(other);
  }

  void TestDidNotSwap(cc::SwapPromise::DidNotSwapReason reason);

  IPC::Message first_message_;
  IPC::Message second_message_;
  IPC::Message third_message_;
  scoped_refptr<FrameSwapMessageQueue> queue_;
};

TEST_F(FrameSwapMessageQueueTest, TestEmptyQueueDrain) {
  std::vector<std::unique_ptr<IPC::Message>> messages;

  DrainMessages(0, &messages);
  ASSERT_TRUE(messages.empty());
}

TEST_F(FrameSwapMessageQueueTest, TestEmpty) {
  std::vector<std::unique_ptr<IPC::Message>> messages;
  ASSERT_TRUE(queue_->Empty());
  QueueNextSwapMessage(CloneMessage(first_message_));
  ASSERT_FALSE(queue_->Empty());
  DrainMessages(0, &messages);
  ASSERT_TRUE(queue_->Empty());
  QueueVisualStateMessage(1, CloneMessage(first_message_));
  ASSERT_FALSE(queue_->Empty());
  queue_->DidActivate(1);
  queue_->DidSwap(1);
  ASSERT_FALSE(queue_->Empty());
}

TEST_F(FrameSwapMessageQueueTest, TestQueueMessageFirst) {
  std::vector<std::unique_ptr<IPC::Message>> messages;
  bool visual_state_first = false;
  bool next_swap_first = false;

  // Queuing the first time should result in true.
  QueueVisualStateMessage(1, CloneMessage(first_message_), &visual_state_first);
  ASSERT_TRUE(visual_state_first);
  // Queuing the second time should result in true.
  QueueVisualStateMessage(
      1, CloneMessage(second_message_), &visual_state_first);
  ASSERT_FALSE(visual_state_first);
  // Queuing for a different frame should result in true.
  QueueVisualStateMessage(2, CloneMessage(first_message_), &visual_state_first);
  ASSERT_TRUE(visual_state_first);

  // Queuing for a different policy should result in true.
  QueueNextSwapMessage(CloneMessage(first_message_), &next_swap_first);
  ASSERT_TRUE(next_swap_first);
  // Second time for the same policy is still false.
  QueueNextSwapMessage(CloneMessage(first_message_), &next_swap_first);
  ASSERT_FALSE(next_swap_first);

  DrainMessages(4, &messages);
  // Queuing after all messages are drained is a true again.
  QueueVisualStateMessage(4, CloneMessage(first_message_), &visual_state_first);
  ASSERT_TRUE(visual_state_first);
}

TEST_F(FrameSwapMessageQueueTest, TestNextSwapMessageSentWithNextFrame) {
  std::vector<std::unique_ptr<IPC::Message>> messages;

  DrainMessages(1, &messages);
  QueueNextSwapMessage(CloneMessage(first_message_));
  DrainMessages(2, &messages);
  ASSERT_EQ(1u, messages.size());
  ASSERT_EQ(first_message_.routing_id(), messages.front()->routing_id());
  messages.clear();

  DrainMessages(2, &messages);
  ASSERT_TRUE(messages.empty());
}

TEST_F(FrameSwapMessageQueueTest, TestNextSwapMessageSentWithCurrentFrame) {
  std::vector<std::unique_ptr<IPC::Message>> messages;

  DrainMessages(1, &messages);
  QueueNextSwapMessage(CloneMessage(first_message_));
  DrainMessages(1, &messages);
  ASSERT_EQ(1u, messages.size());
  ASSERT_EQ(first_message_.routing_id(), messages.front()->routing_id());
  messages.clear();

  DrainMessages(1, &messages);
  ASSERT_TRUE(messages.empty());
}

TEST_F(FrameSwapMessageQueueTest,
       TestDrainsVisualStateMessagesForCorrespondingFrames) {
  std::vector<std::unique_ptr<IPC::Message>> messages;

  QueueVisualStateMessage(1, CloneMessage(first_message_));
  QueueVisualStateMessage(2, CloneMessage(second_message_));
  QueueVisualStateMessage(3, CloneMessage(third_message_));
  DrainMessages(0, &messages);
  ASSERT_TRUE(messages.empty());

  DrainMessages(2, &messages);
  ASSERT_EQ(2u, messages.size());
  ASSERT_TRUE(HasMessageForId(messages, first_message_.routing_id()));
  ASSERT_TRUE(HasMessageForId(messages, second_message_.routing_id()));
  messages.clear();

  DrainMessages(2, &messages);
  ASSERT_TRUE(messages.empty());

  DrainMessages(5, &messages);
  ASSERT_EQ(1u, messages.size());
  ASSERT_EQ(third_message_.routing_id(), messages.front()->routing_id());
}

TEST_F(FrameSwapMessageQueueTest,
       TestQueueNextSwapMessagePreservesFifoOrdering) {
  std::vector<std::unique_ptr<IPC::Message>> messages;

  QueueNextSwapMessage(CloneMessage(first_message_));
  QueueNextSwapMessage(CloneMessage(second_message_));
  DrainMessages(1, &messages);
  ASSERT_EQ(2u, messages.size());
  ASSERT_EQ(first_message_.routing_id(), messages[0]->routing_id());
  ASSERT_EQ(second_message_.routing_id(), messages[1]->routing_id());
}

TEST_F(FrameSwapMessageQueueTest,
       TestQueueVisualStateMessagePreservesFifoOrdering) {
  std::vector<std::unique_ptr<IPC::Message>> messages;

  QueueVisualStateMessage(1, CloneMessage(first_message_));
  QueueVisualStateMessage(1, CloneMessage(second_message_));
  DrainMessages(1, &messages);
  ASSERT_EQ(2u, messages.size());
  ASSERT_EQ(first_message_.routing_id(), messages[0]->routing_id());
  ASSERT_EQ(second_message_.routing_id(), messages[1]->routing_id());
}

void FrameSwapMessageQueueTest::TestDidNotSwap(
    cc::SwapPromise::DidNotSwapReason reason) {
  std::vector<std::unique_ptr<IPC::Message>> messages;

  QueueNextSwapMessage(CloneMessage(first_message_));
  QueueVisualStateMessage(2, CloneMessage(second_message_));
  QueueVisualStateMessage(3, CloneMessage(third_message_));
  const int rid[] = {first_message_.routing_id(),
                     second_message_.routing_id(),
                     third_message_.routing_id()};

  bool msg_delivered = reason != cc::SwapPromise::COMMIT_FAILS &&
                       reason != cc::SwapPromise::ACTIVATION_FAILS;

  queue_->DidNotSwap(2, reason, &messages);
  ASSERT_TRUE(msg_delivered == HasMessageForId(messages, rid[0]));
  ASSERT_TRUE(msg_delivered == HasMessageForId(messages, rid[1]));
  ASSERT_FALSE(HasMessageForId(messages, rid[2]));
  messages.clear();

  queue_->DidNotSwap(3, reason, &messages);
  ASSERT_FALSE(HasMessageForId(messages, rid[0]));
  ASSERT_FALSE(HasMessageForId(messages, rid[1]));
  ASSERT_TRUE(msg_delivered == HasMessageForId(messages, rid[2]));
  messages.clear();

  // all undelivered messages should still be available for RenderFrameHostImpl
  // to deliver.
  DrainMessages(3, &messages);
  ASSERT_TRUE(msg_delivered != HasMessageForId(messages, rid[0]));
  ASSERT_TRUE(msg_delivered != HasMessageForId(messages, rid[1]));
  ASSERT_TRUE(msg_delivered != HasMessageForId(messages, rid[2]));
}

TEST_F(FrameSwapMessageQueueTest, TestDidNotSwapNoUpdate) {
  TestDidNotSwap(cc::SwapPromise::COMMIT_NO_UPDATE);
}

TEST_F(FrameSwapMessageQueueTest, TestDidNotSwapSwapFails) {
  TestDidNotSwap(cc::SwapPromise::SWAP_FAILS);
}

TEST_F(FrameSwapMessageQueueTest, TestDidNotSwapCommitFails) {
  TestDidNotSwap(cc::SwapPromise::COMMIT_FAILS);
}

TEST_F(FrameSwapMessageQueueTest, TestDidNotSwapActivationFails) {
  TestDidNotSwap(cc::SwapPromise::ACTIVATION_FAILS);
}

class NotifiesDeletionMessage : public IPC::Message {
 public:
  NotifiesDeletionMessage(bool* deleted, const IPC::Message& other)
      : IPC::Message(other), deleted_(deleted) {}
  ~NotifiesDeletionMessage() override { *deleted_ = true; }

 private:
  bool* deleted_;
};

TEST_F(FrameSwapMessageQueueTest, TestDeletesNextSwapMessage) {
  bool message_deleted = false;
  QueueNextSwapMessage(std::make_unique<NotifiesDeletionMessage>(
      &message_deleted, first_message_));
  queue_ = nullptr;
  ASSERT_TRUE(message_deleted);
}

TEST_F(FrameSwapMessageQueueTest, TestDeletesVisualStateMessage) {
  bool message_deleted = false;
  QueueVisualStateMessage(1, std::make_unique<NotifiesDeletionMessage>(
                                 &message_deleted, first_message_));
  queue_ = nullptr;
  ASSERT_TRUE(message_deleted);
}

TEST_F(FrameSwapMessageQueueTest, TestDeletesQueuedVisualStateMessage) {
  bool message_deleted = false;
  QueueVisualStateMessage(1, std::make_unique<NotifiesDeletionMessage>(
                                 &message_deleted, first_message_));
  queue_->DidActivate(1);
  queue_->DidSwap(1);
  queue_ = nullptr;
  ASSERT_TRUE(message_deleted);
}

TEST_F(FrameSwapMessageQueueTest, TestDrainsMessageOnActivationThanDidNotSwap) {
  const int frame = 6;
  std::unique_ptr<IPC::Message> msg = CloneMessage(first_message_);
  IPC::Message* msgSent = msg.get();
  QueueVisualStateMessage(frame, std::move(msg));
  queue_->DidActivate(frame);
  EXPECT_TRUE(!queue_->Empty());

  std::vector<std::unique_ptr<IPC::Message>> messages;
  queue_->DidNotSwap(frame, cc::SwapPromise::SWAP_FAILS, &messages);
  CHECK_EQ(1UL, messages.size());
  EXPECT_EQ(messages[0].get(), msgSent);
  msgSent = nullptr;

  queue_ = nullptr;
}

}  // namespace content
