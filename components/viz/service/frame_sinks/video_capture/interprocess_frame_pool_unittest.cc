// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/frame_sinks/video_capture/interprocess_frame_pool.h"

#include "media/base/video_frame.h"
#include "media/base/video_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

using media::VideoFrame;

namespace viz {
namespace {

constexpr gfx::Size kSize = gfx::Size(32, 18);
constexpr media::VideoPixelFormat kFormat = media::PIXEL_FORMAT_I420;

void ExpectValidBufferForDelivery(
    const InterprocessFramePool::BufferAndSize& buffer_and_size) {
  EXPECT_TRUE(buffer_and_size.first.is_valid());
  constexpr int kI420BitsPerPixel = 12;
  EXPECT_LE(static_cast<size_t>(kSize.GetArea() * kI420BitsPerPixel / 8),
            buffer_and_size.second);
}

TEST(InterprocessFramePoolTest, FramesConfiguredCorrectly) {
  InterprocessFramePool pool(1);
  const scoped_refptr<media::VideoFrame> frame =
      pool.ReserveVideoFrame(kFormat, kSize);
  ASSERT_TRUE(frame);
  ASSERT_EQ(kSize, frame->coded_size());
  ASSERT_EQ(gfx::Rect(kSize), frame->visible_rect());
  ASSERT_EQ(kSize, frame->natural_size());
  ASSERT_TRUE(frame->IsMappable());
}

TEST(InterprocessFramePool, UsesAvailableBuffersIfPossible) {
  constexpr gfx::Size kSmallerSize =
      gfx::Size(kSize.width() / 2, kSize.height() / 2);
  constexpr gfx::Size kBiggerSize =
      gfx::Size(kSize.width() * 2, kSize.height() * 2);

  InterprocessFramePool pool(1);

  // Reserve a frame of baseline size and then free it to return it to the pool.
  scoped_refptr<media::VideoFrame> frame =
      pool.ReserveVideoFrame(kFormat, kSize);
  ASSERT_TRUE(frame);
  size_t baseline_bytes_allocated;
  {
    auto buffer_and_size = pool.CloneHandleForDelivery(frame.get());
    ExpectValidBufferForDelivery(buffer_and_size);
    baseline_bytes_allocated = buffer_and_size.second;
  }
  frame = nullptr;  // Returns frame to pool.

  // Now, attempt to reserve a smaller-sized frame. Expect that the same buffer
  // is backing the frame because it's large enough.
  frame = pool.ReserveVideoFrame(kFormat, kSmallerSize);
  ASSERT_TRUE(frame);
  {
    auto buffer_and_size = pool.CloneHandleForDelivery(frame.get());
    ExpectValidBufferForDelivery(buffer_and_size);
    EXPECT_EQ(baseline_bytes_allocated, buffer_and_size.second);
  }
  frame = nullptr;  // Returns frame to pool.

  // Now, attempt to reserve a larger-than-baseline-sized frame. Expect that a
  // different buffer is backing the frame because a larger one had to be
  // allocated.
  frame = pool.ReserveVideoFrame(kFormat, kBiggerSize);
  ASSERT_TRUE(frame);
  size_t larger_buffer_bytes_allocated;
  {
    auto buffer_and_size = pool.CloneHandleForDelivery(frame.get());
    ExpectValidBufferForDelivery(buffer_and_size);
    larger_buffer_bytes_allocated = buffer_and_size.second;
    EXPECT_LT(baseline_bytes_allocated, larger_buffer_bytes_allocated);
  }
  frame = nullptr;  // Returns frame to pool.

  // Finally, if either a baseline-sized or a smaller-than-baseline-sized frame
  // is reserved, expect that the same larger buffer is backing the frames.
  constexpr gfx::Size kTheSmallerSizes[] = {kSmallerSize, kSize};
  for (const auto& size : kTheSmallerSizes) {
    frame = pool.ReserveVideoFrame(kFormat, size);
    ASSERT_TRUE(frame);
    {
      auto buffer_and_size = pool.CloneHandleForDelivery(frame.get());
      ExpectValidBufferForDelivery(buffer_and_size);
      EXPECT_EQ(larger_buffer_bytes_allocated, buffer_and_size.second);
    }
    frame = nullptr;  // Returns frame to pool.
  }
}

TEST(InterprocessFramePoolTest, ReachesCapacityLimit) {
  InterprocessFramePool pool(2);
  scoped_refptr<media::VideoFrame> frames[5];

  // Reserve two frames from a pool of capacity 2.
  frames[0] = pool.ReserveVideoFrame(kFormat, kSize);
  ASSERT_TRUE(frames[0]);
  frames[1] = pool.ReserveVideoFrame(kFormat, kSize);
  ASSERT_TRUE(frames[1]);

  // Now, try to reserve a third frame. This should fail (return null).
  frames[2] = pool.ReserveVideoFrame(kFormat, kSize);
  ASSERT_FALSE(frames[2]);

  // Release the first frame. Then, retry reserving a frame. This should
  // succeed.
  frames[0] = nullptr;
  frames[3] = pool.ReserveVideoFrame(kFormat, kSize);
  ASSERT_TRUE(frames[3]);

  // Finally, try to reserve yet another frame. This should fail.
  frames[4] = pool.ReserveVideoFrame(kFormat, kSize);
  ASSERT_FALSE(frames[4]);
}

// Returns true iff each plane of the given |frame| is filled with
// |values[plane]|.
bool PlanesAreFilledWithValues(const VideoFrame& frame, const uint8_t* values) {
  static_assert(VideoFrame::kUPlane == (VideoFrame::kYPlane + 1) &&
                    VideoFrame::kVPlane == (VideoFrame::kUPlane + 1),
                "enum values changed, will break code below");
  for (int plane = VideoFrame::kYPlane; plane <= VideoFrame::kVPlane; ++plane) {
    const uint8_t expected_value = values[plane - VideoFrame::kYPlane];
    for (int y = 0; y < frame.rows(plane); ++y) {
      const uint8_t* row = frame.visible_data(plane) + y * frame.stride(plane);
      for (int x = 0; x < frame.row_bytes(plane); ++x) {
        EXPECT_EQ(expected_value, row[x])
            << "at row " << y << " in plane " << plane;
        if (expected_value != row[x])
          return false;
      }
    }
  }
  return true;
}

TEST(InterprocessFramePoolTest, ResurrectsDeliveredFramesOnly) {
  InterprocessFramePool pool(2);

  // Reserve a frame, populate it, but release it before delivery.
  scoped_refptr<media::VideoFrame> frame =
      pool.ReserveVideoFrame(kFormat, kSize);
  ASSERT_TRUE(frame);
  media::FillYUV(frame.get(), 0x11, 0x22, 0x33);
  frame = nullptr;  // Returns frame to pool.

  // The pool should fail to resurrect the last frame because it was never
  // delivered.
  frame = pool.ResurrectLastVideoFrame(kFormat, kSize);
  ASSERT_FALSE(frame);

  // Reserve a frame and populate it with different color values; only this
  // time, signal that it will be delivered before releasing it.
  frame = pool.ReserveVideoFrame(kFormat, kSize);
  ASSERT_TRUE(frame);
  const uint8_t kValues[3] = {0x44, 0x55, 0x66};
  media::FillYUV(frame.get(), kValues[0], kValues[1], kValues[2]);
  {
    auto buffer_and_size = pool.CloneHandleForDelivery(frame.get());
    ExpectValidBufferForDelivery(buffer_and_size);
  }
  frame = nullptr;  // Returns frame to pool.

  // Confirm that the last frame can be resurrected repeatedly.
  for (int i = 0; i < 3; ++i) {
    frame = pool.ResurrectLastVideoFrame(kFormat, kSize);
    ASSERT_TRUE(frame);
    ASSERT_TRUE(PlanesAreFilledWithValues(*frame, kValues));
    frame = nullptr;  // Returns frame to pool.
  }

  // A frame that is being delivered cannot be resurrected.
  for (int i = 0; i < 2; ++i) {
    if (i == 0) {  // Test this for a resurrected frame.
      frame = pool.ResurrectLastVideoFrame(kFormat, kSize);
      ASSERT_TRUE(frame);
      ASSERT_TRUE(PlanesAreFilledWithValues(*frame, kValues));
    } else {  // Test this for a normal frame.
      frame = pool.ReserveVideoFrame(kFormat, kSize);
      ASSERT_TRUE(frame);
      media::FillYUV(frame.get(), 0x77, 0x88, 0x99);
    }
    {
      auto buffer_and_size = pool.CloneHandleForDelivery(frame.get());
      ExpectValidBufferForDelivery(buffer_and_size);
    }
    scoped_refptr<media::VideoFrame> should_be_null =
        pool.ResurrectLastVideoFrame(kFormat, kSize);
    ASSERT_FALSE(should_be_null);
    frame = nullptr;  // Returns frame to pool.
  }

  // Finally, reserve a frame, populate it, and don't deliver it. Expect that,
  // still, undelivered frames cannot be resurrected.
  frame = pool.ReserveVideoFrame(kFormat, kSize);
  ASSERT_TRUE(frame);
  media::FillYUV(frame.get(), 0xaa, 0xbb, 0xcc);
  frame = nullptr;  // Returns frame to pool.
  frame = pool.ResurrectLastVideoFrame(kFormat, kSize);
  ASSERT_FALSE(frame);
}

TEST(InterprocessFramePoolTest, ReportsCorrectUtilization) {
  InterprocessFramePool pool(2);
  ASSERT_EQ(0.0f, pool.GetUtilization());

  // Run through a typical sequence twice: Once for normal frame reservation,
  // and the second time for a resurrected frame.
  for (int i = 0; i < 2; ++i) {
    // Reserve the frame and expect 1/2 the pool to be utilized.
    scoped_refptr<media::VideoFrame> frame =
        (i == 0) ? pool.ReserveVideoFrame(kFormat, kSize)
                 : pool.ResurrectLastVideoFrame(kFormat, kSize);
    ASSERT_TRUE(frame);
    ASSERT_EQ(0.5f, pool.GetUtilization());

    // Signal that the frame will be delivered. This should not change the
    // utilization.
    {
      auto buffer_and_size = pool.CloneHandleForDelivery(frame.get());
      ExpectValidBufferForDelivery(buffer_and_size);
    }
    ASSERT_EQ(0.5f, pool.GetUtilization());

    // Finally, release the frame to indicate it has been delivered and is no
    // longer in-use by downstream consumers. This should cause the utilization
    // to go back down to zero.
    frame = nullptr;
    ASSERT_EQ(0.0f, pool.GetUtilization());
  }
}

}  // namespace
}  // namespace viz
