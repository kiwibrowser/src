/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "video/buffered_frame_decryptor.h"

#include <map>
#include <memory>
#include <vector>

#include "absl/memory/memory.h"
#include "api/test/mock_frame_decryptor.h"
#include "modules/video_coding/packet_buffer.h"
#include "rtc_base/ref_counted_object.h"
#include "system_wrappers/include/clock.h"
#include "test/gmock.h"
#include "test/gtest.h"

using ::testing::Return;

namespace webrtc {
namespace {

class FakePacketBuffer : public video_coding::PacketBuffer {
 public:
  FakePacketBuffer() : PacketBuffer(nullptr, 0, 0, nullptr) {}
  ~FakePacketBuffer() override {}

  VCMPacket* GetPacket(uint16_t seq_num) override {
    auto packet_it = packets_.find(seq_num);
    return packet_it == packets_.end() ? nullptr : &packet_it->second;
  }

  bool InsertPacket(VCMPacket* packet) override {
    packets_[packet->seqNum] = *packet;
    return true;
  }

  bool GetBitstream(const video_coding::RtpFrameObject& frame,
                    uint8_t* destination) override {
    return true;
  }

  void ReturnFrame(video_coding::RtpFrameObject* frame) override {
    packets_.erase(frame->first_seq_num());
  }

 private:
  std::map<uint16_t, VCMPacket> packets_;
};

}  // namespace

class BufferedFrameDecryptorTest
    : public ::testing::Test,
      public OnDecryptedFrameCallback,
      public video_coding::OnReceivedFrameCallback {
 public:
  // Implements the OnDecryptedFrameCallbackInterface
  void OnDecryptedFrame(
      std::unique_ptr<video_coding::RtpFrameObject> frame) override {
    decrypted_frame_call_count_++;
  }

  // Implements the OnReceivedFrameCallback interface.
  void OnReceivedFrame(
      std::unique_ptr<video_coding::RtpFrameObject> frame) override {}

  // Returns a new fake RtpFrameObject it abstracts the difficult construction
  // of the RtpFrameObject to simplify testing.
  std::unique_ptr<video_coding::RtpFrameObject> CreateRtpFrameObject(
      bool key_frame) {
    seq_num_++;

    VCMPacket packet;
    packet.codec = kVideoCodecGeneric;
    packet.seqNum = seq_num_;
    packet.frameType = key_frame ? kVideoFrameKey : kVideoFrameDelta;
    packet.generic_descriptor = RtpGenericFrameDescriptor();
    fake_packet_buffer_->InsertPacket(&packet);
    packet.seqNum = seq_num_;
    packet.is_last_packet_in_frame = true;
    fake_packet_buffer_->InsertPacket(&packet);

    return std::unique_ptr<video_coding::RtpFrameObject>(
        new video_coding::RtpFrameObject(fake_packet_buffer_.get(), seq_num_,
                                         seq_num_, 0, 0, 0, 0));
  }

 protected:
  BufferedFrameDecryptorTest() : fake_packet_buffer_(new FakePacketBuffer()) {}
  void SetUp() override {
    fake_packet_data_ = std::vector<uint8_t>(100);
    decrypted_frame_call_count_ = 0;
    seq_num_ = 0;
    mock_frame_decryptor_ = new rtc::RefCountedObject<MockFrameDecryptor>();
    buffered_frame_decryptor_ = absl::make_unique<BufferedFrameDecryptor>(
        this, mock_frame_decryptor_.get());
  }

  static const size_t kMaxStashedFrames;

  std::vector<uint8_t> fake_packet_data_;
  rtc::scoped_refptr<FakePacketBuffer> fake_packet_buffer_;
  rtc::scoped_refptr<MockFrameDecryptor> mock_frame_decryptor_;
  std::unique_ptr<BufferedFrameDecryptor> buffered_frame_decryptor_;
  size_t decrypted_frame_call_count_;
  uint16_t seq_num_;
};

const size_t BufferedFrameDecryptorTest::kMaxStashedFrames = 24;

// Callback should always be triggered on a successful decryption.
TEST_F(BufferedFrameDecryptorTest, CallbackCalledOnSuccessfulDecryption) {
  EXPECT_CALL(*mock_frame_decryptor_, Decrypt).Times(1).WillOnce(Return(0));
  EXPECT_CALL(*mock_frame_decryptor_, GetMaxPlaintextByteSize)
      .Times(1)
      .WillOnce(Return(0));
  buffered_frame_decryptor_->ManageEncryptedFrame(CreateRtpFrameObject(true));
  EXPECT_EQ(decrypted_frame_call_count_, static_cast<size_t>(1));
}

// An initial fail to decrypt should not trigger the callback.
TEST_F(BufferedFrameDecryptorTest, CallbackNotCalledOnFailedDecryption) {
  EXPECT_CALL(*mock_frame_decryptor_, Decrypt).Times(1).WillOnce(Return(1));
  EXPECT_CALL(*mock_frame_decryptor_, GetMaxPlaintextByteSize)
      .Times(1)
      .WillOnce(Return(0));
  buffered_frame_decryptor_->ManageEncryptedFrame(CreateRtpFrameObject(true));
  EXPECT_EQ(decrypted_frame_call_count_, static_cast<size_t>(0));
}

// Initial failures should be stored and retried after the first successful
// decryption.
TEST_F(BufferedFrameDecryptorTest, DelayedCallbackOnBufferedFrames) {
  EXPECT_CALL(*mock_frame_decryptor_, Decrypt)
      .Times(3)
      .WillOnce(Return(1))
      .WillOnce(Return(0))
      .WillOnce(Return(0));
  EXPECT_CALL(*mock_frame_decryptor_, GetMaxPlaintextByteSize)
      .Times(3)
      .WillRepeatedly(Return(0));

  // The first decrypt will fail stashing the first frame.
  buffered_frame_decryptor_->ManageEncryptedFrame(CreateRtpFrameObject(true));
  EXPECT_EQ(decrypted_frame_call_count_, static_cast<size_t>(0));
  // The second call will succeed playing back both frames.
  buffered_frame_decryptor_->ManageEncryptedFrame(CreateRtpFrameObject(false));
  EXPECT_EQ(decrypted_frame_call_count_, static_cast<size_t>(2));
}

// Subsequent failure to decrypts after the first successful decryption should
// fail to decryptk
TEST_F(BufferedFrameDecryptorTest, FTDDiscardedAfterFirstSuccess) {
  EXPECT_CALL(*mock_frame_decryptor_, Decrypt)
      .Times(4)
      .WillOnce(Return(1))
      .WillOnce(Return(0))
      .WillOnce(Return(0))
      .WillOnce(Return(1));
  EXPECT_CALL(*mock_frame_decryptor_, GetMaxPlaintextByteSize)
      .Times(4)
      .WillRepeatedly(Return(0));

  // The first decrypt will fail stashing the first frame.
  buffered_frame_decryptor_->ManageEncryptedFrame(CreateRtpFrameObject(true));
  EXPECT_EQ(decrypted_frame_call_count_, static_cast<size_t>(0));
  // The second call will succeed playing back both frames.
  buffered_frame_decryptor_->ManageEncryptedFrame(CreateRtpFrameObject(false));
  EXPECT_EQ(decrypted_frame_call_count_, static_cast<size_t>(2));
  // A new failure call will not result in an additional decrypted frame
  // callback.
  buffered_frame_decryptor_->ManageEncryptedFrame(CreateRtpFrameObject(true));
  EXPECT_EQ(decrypted_frame_call_count_, static_cast<size_t>(2));
}

// Validate that the maximum number of stashed frames cannot be exceeded even if
// more than its maximum arrives before the first successful decryption.
TEST_F(BufferedFrameDecryptorTest, MaximumNumberOfFramesStored) {
  const size_t failed_to_decrypt_count = kMaxStashedFrames * 2;
  EXPECT_CALL(*mock_frame_decryptor_, Decrypt)
      .Times(failed_to_decrypt_count)
      .WillRepeatedly(Return(1));
  EXPECT_CALL(*mock_frame_decryptor_, GetMaxPlaintextByteSize)
      .WillRepeatedly(Return(0));

  for (size_t i = 0; i < failed_to_decrypt_count; ++i) {
    buffered_frame_decryptor_->ManageEncryptedFrame(CreateRtpFrameObject(true));
  }
  EXPECT_EQ(decrypted_frame_call_count_, static_cast<size_t>(0));

  EXPECT_CALL(*mock_frame_decryptor_, Decrypt)
      .Times(kMaxStashedFrames + 1)
      .WillRepeatedly(Return(0));
  buffered_frame_decryptor_->ManageEncryptedFrame(CreateRtpFrameObject(true));
  EXPECT_EQ(decrypted_frame_call_count_, kMaxStashedFrames + 1);
}

}  // namespace webrtc
