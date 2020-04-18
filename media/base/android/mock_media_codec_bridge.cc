// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/android/mock_media_codec_bridge.h"

#include "base/synchronization/waitable_event.h"
#include "media/base/encryption_scheme.h"
#include "media/base/subsample_entry.h"

using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::_;

namespace media {

MockMediaCodecBridge::MockMediaCodecBridge() {
  ON_CALL(*this, DequeueInputBuffer(_, _))
      .WillByDefault(Return(MEDIA_CODEC_TRY_AGAIN_LATER));
  ON_CALL(*this, DequeueOutputBuffer(_, _, _, _, _, _, _))
      .WillByDefault(Return(MEDIA_CODEC_TRY_AGAIN_LATER));
}

MockMediaCodecBridge::~MockMediaCodecBridge() {
  if (destruction_event_)
    destruction_event_->Signal();
}

void MockMediaCodecBridge::AcceptOneInput(IsEos eos) {
  EXPECT_CALL(*this, DequeueInputBuffer(_, _))
      .WillOnce(DoAll(SetArgPointee<1>(42), Return(MEDIA_CODEC_OK)))
      .WillRepeatedly(Return(MEDIA_CODEC_TRY_AGAIN_LATER));
  if (eos == kEos)
    EXPECT_CALL(*this, QueueEOS(_));
}

void MockMediaCodecBridge::ProduceOneOutput(IsEos eos) {
  EXPECT_CALL(*this, DequeueOutputBuffer(_, _, _, _, _, _, _))
      .WillOnce(DoAll(SetArgPointee<5>(eos == kEos ? true : false),
                      Return(MEDIA_CODEC_OK)))
      .WillRepeatedly(Return(MEDIA_CODEC_TRY_AGAIN_LATER));
}

void MockMediaCodecBridge::SetCodecDestroyedEvent(base::WaitableEvent* event) {
  destruction_event_ = event;
}

std::unique_ptr<MediaCodecBridge> MockMediaCodecBridge::CreateVideoDecoder(
    VideoCodec codec,
    CodecType codec_type,
    const gfx::Size& size,  // Output frame size.
    const base::android::JavaRef<jobject>& surface,
    const base::android::JavaRef<jobject>& media_crypto,
    const std::vector<uint8_t>& csd0,
    const std::vector<uint8_t>& csd1,
    const VideoColorSpace& color_space,
    const base::Optional<HDRMetadata>& hdr_metadata,
    bool allow_adaptive_playback) {
  return std::make_unique<MockMediaCodecBridge>();
}

}  // namespace media
