/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "video/partial_frame_assembler.h"

#include "api/video/i420_buffer.h"
#include "api/video/video_frame.h"
#include "test/gmock.h"
#include "test/gtest.h"

namespace webrtc {
namespace {

constexpr uint8_t kCol1 = 100;
constexpr uint8_t kCol2 = 200;

constexpr int kWidth = 640;
constexpr int kHeight = 480;

rtc::scoped_refptr<I420Buffer> CreatePicture(int width,
                                             int height,
                                             uint8_t data) {
  rtc::scoped_refptr<I420Buffer> buf = I420Buffer::Create(width, height);
  for (int row = 0; row < height; ++row) {
    for (int col = 0; col < width; ++col) {
      int pos_y = row * buf->StrideY() + col;
      int pos_u = row / 2 * buf->StrideU() + col / 2;
      int pos_v = row / 2 * buf->StrideV() + col / 2;
      buf->MutableDataY()[pos_y] = data;
      buf->MutableDataU()[pos_u] = data;
      buf->MutableDataV()[pos_v] = data;
    }
  }
  return buf;
}

VideoFrame CreateFrame(rtc::scoped_refptr<I420Buffer> buf) {
  VideoFrame frame = VideoFrame(buf, VideoRotation::kVideoRotation_0, 0);
  frame.set_cache_buffer_for_partial_updates(true);
  return frame;
}

bool TestPictureWithOneRect(rtc::scoped_refptr<I420BufferInterface> buf,
                            int offset_x,
                            int offset_y,
                            int rect_width,
                            int rect_height,
                            uint8_t in_rect_data,
                            uint8_t out_rect_data) {
  for (int row = 0; row < buf->height(); ++row) {
    for (int col = 0; col < buf->width(); ++col) {
      int pos_y = row * buf->StrideY() + col;
      int pos_u = row / 2 * buf->StrideU() + col / 2;
      int pos_v = row / 2 * buf->StrideV() + col / 2;
      uint8_t y = buf->DataY()[pos_y];
      uint8_t u = buf->DataU()[pos_u];
      uint8_t v = buf->DataV()[pos_v];
      bool in_rect = col >= offset_x && col < offset_x + rect_width &&
                     row >= offset_y && row < offset_y + rect_height;
      uint8_t expected_data = in_rect ? in_rect_data : out_rect_data;
      if (y != expected_data || u != expected_data || v != expected_data)
        return false;
    }
  }
  return true;
}

TEST(PartialFrameAssembler, FullPictureUpdates) {
  PartialFrameAssembler decompressor;

  // Full pic.
  auto full_pic1 = CreatePicture(kWidth, kHeight, kCol1);
  VideoFrame frame1 = CreateFrame(full_pic1);

  // Full pic.
  auto full_pic2 = CreatePicture(kWidth, kHeight, kCol2);
  VideoFrame frame2 = CreateFrame(full_pic2);

  EXPECT_TRUE(
      decompressor.ApplyPartialUpdate(full_pic1.get(), &frame1, nullptr));
  EXPECT_TRUE(TestPictureWithOneRect(frame1.video_frame_buffer()->ToI420(), 0,
                                     0, -1, -1, kCol1, kCol1));

  EXPECT_TRUE(
      decompressor.ApplyPartialUpdate(full_pic2.get(), &frame2, nullptr));
  EXPECT_TRUE(TestPictureWithOneRect(frame2.video_frame_buffer()->ToI420(), 0,
                                     0, -1, -1, kCol2, kCol2));
}

TEST(PartialFrameAssembler, PartialUpdateFirstFails) {
  PartialFrameAssembler decompressor;

  // Partial update.
  auto full_pic1 = CreatePicture(20, 20, kCol1);
  VideoFrame::PartialFrameDescription desc1{0, 0};
  VideoFrame frame1 = CreateFrame(full_pic1);

  EXPECT_FALSE(
      decompressor.ApplyPartialUpdate(full_pic1.get(), &frame1, &desc1));
}

TEST(PartialFrameAssembler, PartialUpdate) {
  PartialFrameAssembler decompressor;

  // Full pic.
  auto full_pic1 = CreatePicture(kWidth, kHeight, kCol1);
  VideoFrame frame1 = CreateFrame(full_pic1);

  // Partial pic update.
  auto full_pic2 = CreatePicture(10, 20, kCol2);
  VideoFrame::PartialFrameDescription desc2{30, 40};
  VideoFrame frame2 = CreateFrame(full_pic2);

  EXPECT_TRUE(
      decompressor.ApplyPartialUpdate(full_pic1.get(), &frame1, nullptr));
  EXPECT_TRUE(TestPictureWithOneRect(frame1.video_frame_buffer()->ToI420(), 0,
                                     0, -1, -1, kCol1, kCol1));

  EXPECT_TRUE(
      decompressor.ApplyPartialUpdate(full_pic2.get(), &frame2, &desc2));
  EXPECT_TRUE(TestPictureWithOneRect(frame2.video_frame_buffer()->ToI420(), 30,
                                     40, 10, 20, kCol2, kCol1));
}

TEST(PartialFrameAssembler, ProcessesUnchangedUpdate) {
  PartialFrameAssembler decompressor;

  auto full_pic1 = CreatePicture(kWidth, kHeight, kCol1);
  VideoFrame frame1 = CreateFrame(full_pic1);

  // Partial pic update.
  auto full_pic2 = CreatePicture(10, 20, kCol2);
  VideoFrame::PartialFrameDescription desc2{30, 40};
  VideoFrame frame2 = CreateFrame(full_pic2);

  // Empty update.
  VideoFrame::PartialFrameDescription desc3{0, 0};
  VideoFrame frame3 = CreateFrame(nullptr);

  EXPECT_TRUE(
      decompressor.ApplyPartialUpdate(full_pic1.get(), &frame1, nullptr));

  EXPECT_TRUE(
      decompressor.ApplyPartialUpdate(full_pic2.get(), &frame2, &desc2));

  EXPECT_TRUE(decompressor.ApplyPartialUpdate(nullptr, &frame3, &desc3));
}

TEST(PartialFrameAssembler, PartialUpdateFailsForOddXOffset) {
  PartialFrameAssembler decompressor;

  // Full pic.
  auto full_pic1 = CreatePicture(kWidth, kHeight, kCol1);
  VideoFrame frame1 = CreateFrame(full_pic1);

  // Partial pic update.
  auto full_pic2 = CreatePicture(10, 20, kCol2);
  // Offset is odd.
  VideoFrame::PartialFrameDescription desc2{31, 40};
  VideoFrame frame2 = CreateFrame(full_pic2);

  EXPECT_TRUE(
      decompressor.ApplyPartialUpdate(full_pic1.get(), &frame1, nullptr));

  EXPECT_FALSE(
      decompressor.ApplyPartialUpdate(full_pic2.get(), &frame2, &desc2));
}

TEST(PartialFrameAssembler, PartialUpdateFailsForOddYOffset) {
  PartialFrameAssembler decompressor;

  // Full pic.
  auto full_pic1 = CreatePicture(kWidth, kHeight, kCol1);
  VideoFrame frame1 = CreateFrame(full_pic1);

  // Partial pic update.
  auto full_pic2 = CreatePicture(10, 20, kCol2);
  // Offset is odd.
  VideoFrame::PartialFrameDescription desc2{30, 41};
  VideoFrame frame2 = CreateFrame(full_pic2);

  EXPECT_TRUE(
      decompressor.ApplyPartialUpdate(full_pic1.get(), &frame1, nullptr));

  EXPECT_FALSE(
      decompressor.ApplyPartialUpdate(full_pic2.get(), &frame2, &desc2));
}

TEST(PartialFrameAssembler, PartialUpdateFailsForOddWidth) {
  PartialFrameAssembler decompressor;

  // Full pic.
  auto full_pic1 = CreatePicture(kWidth, kHeight, kCol1);
  VideoFrame frame1 = CreateFrame(full_pic1);

  // Partial pic update. Odd width.
  auto full_pic2 = CreatePicture(11, 20, kCol2);
  VideoFrame::PartialFrameDescription desc2{30, 40};
  VideoFrame frame2 = CreateFrame(full_pic2);

  EXPECT_TRUE(
      decompressor.ApplyPartialUpdate(full_pic1.get(), &frame1, nullptr));

  EXPECT_FALSE(
      decompressor.ApplyPartialUpdate(full_pic2.get(), &frame2, &desc2));
}

TEST(PartialFrameAssembler, PartialUpdateWorksForOddWidthAtTheEnd) {
  PartialFrameAssembler decompressor;

  // Full pic. Odd resolution.
  auto full_pic1 = CreatePicture(kWidth + 1, kHeight + 1, kCol1);
  VideoFrame frame1 = CreateFrame(full_pic1);

  // Partial pic update. Odd width.
  auto full_pic2 = CreatePicture(11, 11, kCol2);
  VideoFrame::PartialFrameDescription desc2{kWidth + 1 - 11, kHeight + 1 - 11};
  VideoFrame frame2 = CreateFrame(full_pic2);

  EXPECT_TRUE(
      decompressor.ApplyPartialUpdate(full_pic1.get(), &frame1, nullptr));

  EXPECT_TRUE(
      decompressor.ApplyPartialUpdate(full_pic2.get(), &frame2, &desc2));
}

TEST(PartialFrameAssembler, PartialUpdateFailsForOddNotAtEnd) {
  PartialFrameAssembler decompressor;

  // Full pic. Odd resolution.
  auto full_pic1 = CreatePicture(kWidth + 1, kHeight + 1, kCol1);
  VideoFrame frame1 = CreateFrame(full_pic1);

  // Partial pic update. Odd width.
  auto full_pic2 = CreatePicture(11, 11, kCol2);
  VideoFrame::PartialFrameDescription desc2{kWidth + 1 - 11, kHeight + 1 - 20};
  VideoFrame frame2 = CreateFrame(full_pic2);

  EXPECT_TRUE(
      decompressor.ApplyPartialUpdate(full_pic1.get(), &frame1, nullptr));

  EXPECT_FALSE(
      decompressor.ApplyPartialUpdate(full_pic2.get(), &frame2, &desc2));
}

TEST(PartialFrameAssembler, FullPictureUpdatesCanChangeResolution) {
  PartialFrameAssembler decompressor;

  // Full pic.
  auto full_pic1 = CreatePicture(kWidth, kHeight, kCol1);
  VideoFrame frame1 = CreateFrame(full_pic1);

  // Full pic.
  auto full_pic2 = CreatePicture(kWidth + 100, kHeight + 100, kCol2);
  VideoFrame frame2 = CreateFrame(full_pic2);

  EXPECT_TRUE(
      decompressor.ApplyPartialUpdate(full_pic1.get(), &frame1, nullptr));
  EXPECT_TRUE(TestPictureWithOneRect(frame1.video_frame_buffer()->ToI420(), 0,
                                     0, -1, -1, kCol1, kCol1));

  EXPECT_EQ(frame1.video_frame_buffer()->width(), kWidth);
  EXPECT_EQ(frame1.video_frame_buffer()->height(), kHeight);

  EXPECT_TRUE(
      decompressor.ApplyPartialUpdate(full_pic2.get(), &frame2, nullptr));
  EXPECT_EQ(frame2.video_frame_buffer()->width(), kWidth + 100);
  EXPECT_EQ(frame2.video_frame_buffer()->height(), kHeight + 100);
}

}  // namespace
}  // namespace webrtc
