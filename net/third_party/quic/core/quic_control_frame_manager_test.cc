// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quic/core/quic_control_frame_manager.h"

#include "net/third_party/quic/platform/api/quic_flags.h"
#include "net/third_party/quic/platform/api/quic_ptr_util.h"
#include "net/third_party/quic/platform/api/quic_test.h"
#include "net/third_party/quic/test_tools/quic_test_utils.h"

using testing::_;
using testing::InSequence;
using testing::Return;
using testing::StrictMock;

namespace net {
namespace test {

class QuicControlFrameManagerPeer {
 public:
  static size_t QueueSize(QuicControlFrameManager* manager) {
    return manager->control_frames_.size();
  }
};

namespace {

const QuicStreamId kTestStreamId = 5;

class QuicControlFrameManagerTest : public QuicTest {
 public:
  bool ClearControlFrame(const QuicFrame& frame) {
    DeleteFrame(&const_cast<QuicFrame&>(frame));
    return true;
  }

 protected:
  void Initialize() {
    connection_ = new MockQuicConnection(&helper_, &alarm_factory_,
                                         Perspective::IS_SERVER);
    session_ = QuicMakeUnique<StrictMock<MockQuicSession>>(connection_);
    manager_ = QuicMakeUnique<QuicControlFrameManager>(session_.get());
    EXPECT_EQ(0u, QuicControlFrameManagerPeer::QueueSize(manager_.get()));
    EXPECT_FALSE(manager_->HasPendingRetransmission());
    EXPECT_FALSE(manager_->WillingToWrite());

    EXPECT_CALL(*connection_, SendControlFrame(_)).WillOnce(Return(false));
    manager_->WriteOrBufferRstStream(kTestStreamId, QUIC_STREAM_CANCELLED, 0);
    manager_->WriteOrBufferGoAway(QUIC_PEER_GOING_AWAY, kTestStreamId,
                                  "Going away.");
    manager_->WriteOrBufferWindowUpdate(kTestStreamId, 100);
    manager_->WriteOrBufferBlocked(kTestStreamId);
    EXPECT_EQ(4u, QuicControlFrameManagerPeer::QueueSize(manager_.get()));
    EXPECT_TRUE(manager_->IsControlFrameOutstanding(QuicFrame(&rst_stream_)));
    EXPECT_TRUE(manager_->IsControlFrameOutstanding(QuicFrame(&goaway_)));
    EXPECT_TRUE(
        manager_->IsControlFrameOutstanding(QuicFrame(&window_update_)));
    EXPECT_TRUE(manager_->IsControlFrameOutstanding(QuicFrame(&blocked_)));
    EXPECT_FALSE(
        manager_->IsControlFrameOutstanding(QuicFrame(QuicPingFrame(5))));

    EXPECT_FALSE(manager_->HasPendingRetransmission());
    EXPECT_TRUE(manager_->WillingToWrite());
  }

  QuicRstStreamFrame rst_stream_ = {1, kTestStreamId, QUIC_STREAM_CANCELLED, 0};
  QuicGoAwayFrame goaway_ = {2, QUIC_PEER_GOING_AWAY, kTestStreamId,
                             "Going away."};
  QuicWindowUpdateFrame window_update_ = {3, kTestStreamId, 100};
  QuicBlockedFrame blocked_ = {4, kTestStreamId};
  MockQuicConnectionHelper helper_;
  MockAlarmFactory alarm_factory_;
  MockQuicConnection* connection_;
  std::unique_ptr<StrictMock<MockQuicSession>> session_;
  std::unique_ptr<QuicControlFrameManager> manager_;
};

TEST_F(QuicControlFrameManagerTest, OnControlFrameAcked) {
  Initialize();
  InSequence s;
  EXPECT_CALL(*connection_, SendControlFrame(_))
      .Times(3)
      .WillRepeatedly(
          Invoke(this, &QuicControlFrameManagerTest::ClearControlFrame));
  EXPECT_CALL(*connection_, SendControlFrame(_)).WillOnce(Return(false));
  // Send control frames 1, 2, 3.
  manager_->OnCanWrite();
  EXPECT_TRUE(manager_->IsControlFrameOutstanding(QuicFrame(&rst_stream_)));
  EXPECT_TRUE(manager_->IsControlFrameOutstanding(QuicFrame(&goaway_)));
  EXPECT_TRUE(manager_->IsControlFrameOutstanding(QuicFrame(&window_update_)));
  EXPECT_TRUE(manager_->IsControlFrameOutstanding(QuicFrame(&blocked_)));
  EXPECT_FALSE(
      manager_->IsControlFrameOutstanding(QuicFrame(QuicPingFrame(5))));

  EXPECT_TRUE(manager_->OnControlFrameAcked(QuicFrame(&window_update_)));
  EXPECT_FALSE(manager_->IsControlFrameOutstanding(QuicFrame(&window_update_)));
  EXPECT_EQ(4u, QuicControlFrameManagerPeer::QueueSize(manager_.get()));

  EXPECT_TRUE(manager_->OnControlFrameAcked(QuicFrame(&goaway_)));
  EXPECT_FALSE(manager_->IsControlFrameOutstanding(QuicFrame(&goaway_)));
  EXPECT_EQ(4u, QuicControlFrameManagerPeer::QueueSize(manager_.get()));
  EXPECT_TRUE(manager_->OnControlFrameAcked(QuicFrame(&rst_stream_)));
  EXPECT_FALSE(manager_->IsControlFrameOutstanding(QuicFrame(&rst_stream_)));
  EXPECT_EQ(1u, QuicControlFrameManagerPeer::QueueSize(manager_.get()));
  // Duplicate ack.
  EXPECT_FALSE(manager_->OnControlFrameAcked(QuicFrame(&goaway_)));

  EXPECT_FALSE(manager_->HasPendingRetransmission());
  EXPECT_TRUE(manager_->WillingToWrite());

  // Send control frames 4, 5.
  EXPECT_CALL(*connection_, SendControlFrame(_))
      .WillRepeatedly(
          Invoke(this, &QuicControlFrameManagerTest::ClearControlFrame));
  manager_->OnCanWrite();
  manager_->WritePing();
  EXPECT_FALSE(manager_->WillingToWrite());
}

TEST_F(QuicControlFrameManagerTest, OnControlFrameLost) {
  Initialize();
  InSequence s;
  EXPECT_CALL(*connection_, SendControlFrame(_))
      .Times(3)
      .WillRepeatedly(
          Invoke(this, &QuicControlFrameManagerTest::ClearControlFrame));
  EXPECT_CALL(*connection_, SendControlFrame(_)).WillOnce(Return(false));
  // Send control frames 1, 2, 3.
  manager_->OnCanWrite();

  // Lost control frames 1, 2, 3.
  manager_->OnControlFrameLost(QuicFrame(&rst_stream_));
  manager_->OnControlFrameLost(QuicFrame(&goaway_));
  manager_->OnControlFrameLost(QuicFrame(&window_update_));
  EXPECT_TRUE(manager_->HasPendingRetransmission());

  // Ack control frame 2.
  manager_->OnControlFrameAcked(QuicFrame(&goaway_));

  // Retransmit control frames 1, 3.
  EXPECT_CALL(*connection_, SendControlFrame(_))
      .Times(2)
      .WillRepeatedly(
          Invoke(this, &QuicControlFrameManagerTest::ClearControlFrame));
  manager_->OnCanWrite();
  EXPECT_FALSE(manager_->HasPendingRetransmission());
  EXPECT_TRUE(manager_->WillingToWrite());

  // Send control frames 4, 5.
  EXPECT_CALL(*connection_, SendControlFrame(_))
      .Times(2)
      .WillRepeatedly(
          Invoke(this, &QuicControlFrameManagerTest::ClearControlFrame));
  manager_->OnCanWrite();
  manager_->WritePing();
  EXPECT_FALSE(manager_->WillingToWrite());
}

TEST_F(QuicControlFrameManagerTest, RetransmitControlFrame) {
  Initialize();
  InSequence s;
  // Send control frames 1, 2, 3, 4.
  EXPECT_CALL(*connection_, SendControlFrame(_))
      .Times(4)
      .WillRepeatedly(
          Invoke(this, &QuicControlFrameManagerTest::ClearControlFrame));
  manager_->OnCanWrite();

  // Ack control frame 2.
  manager_->OnControlFrameAcked(QuicFrame(&goaway_));
  // Do not retransmit an acked frame.
  EXPECT_CALL(*connection_, SendControlFrame(_)).Times(0);
  EXPECT_TRUE(manager_->RetransmitControlFrame(QuicFrame(&goaway_)));

  // Retransmit control frame 3.
  EXPECT_CALL(*connection_, SendControlFrame(_))
      .WillOnce(Invoke(this, &QuicControlFrameManagerTest::ClearControlFrame));
  EXPECT_TRUE(manager_->RetransmitControlFrame(QuicFrame(&window_update_)));

  // Retransmit control frame 4, and connection is write blocked.
  EXPECT_CALL(*connection_, SendControlFrame(_)).WillOnce(Return(false));
  EXPECT_FALSE(manager_->RetransmitControlFrame(QuicFrame(&window_update_)));
}

TEST_F(QuicControlFrameManagerTest, DonotSendPingWithBufferedFrames) {
  Initialize();
  InSequence s;
  EXPECT_CALL(*connection_, SendControlFrame(_))
      .WillOnce(Invoke(this, &QuicControlFrameManagerTest::ClearControlFrame));
  EXPECT_CALL(*connection_, SendControlFrame(_)).WillOnce(Return(false));
  // Send control frames 1.
  manager_->OnCanWrite();
  EXPECT_FALSE(manager_->HasPendingRetransmission());
  EXPECT_TRUE(manager_->WillingToWrite());

  // Send PING when there is buffered frames.
  manager_->WritePing();
  // Verify only the buffered 3 frames are sent.
  EXPECT_CALL(*connection_, SendControlFrame(_))
      .Times(3)
      .WillRepeatedly(
          Invoke(this, &QuicControlFrameManagerTest::ClearControlFrame));
  manager_->OnCanWrite();
  EXPECT_FALSE(manager_->HasPendingRetransmission());
  EXPECT_FALSE(manager_->WillingToWrite());
}

}  // namespace
}  // namespace test
}  // namespace net
