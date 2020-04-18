/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/image-decoders/image_decoder.h"

#include <memory>
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/image-decoders/image_frame.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class TestImageDecoder : public ImageDecoder {
 public:
  TestImageDecoder()
      : ImageDecoder(kAlphaNotPremultiplied,
                     ColorBehavior::TransformToSRGB(),
                     kNoDecodedImageByteLimit) {}

  String FilenameExtension() const override { return ""; }

  Vector<ImageFrame, 1>& FrameBufferCache() { return frame_buffer_cache_; }

  void ResetRequiredPreviousFrames(bool known_opaque = false) {
    for (size_t i = 0; i < frame_buffer_cache_.size(); ++i)
      frame_buffer_cache_[i].SetRequiredPreviousFrameIndex(
          FindRequiredPreviousFrame(i, known_opaque));
  }

  void InitFrames(size_t num_frames,
                  unsigned width = 100,
                  unsigned height = 100) {
    SetSize(width, height);
    frame_buffer_cache_.resize(num_frames);
    for (size_t i = 0; i < num_frames; ++i)
      frame_buffer_cache_[i].SetOriginalFrameRect(IntRect(0, 0, width, height));
  }

 private:
  void DecodeSize() override {}
  void Decode(size_t index) override {}
};

TEST(ImageDecoderTest, sizeCalculationMayOverflow) {
  std::unique_ptr<TestImageDecoder> decoder(
      std::make_unique<TestImageDecoder>());
  EXPECT_FALSE(decoder->SetSize(1 << 29, 1));
  EXPECT_FALSE(decoder->SetSize(1, 1 << 29));
  EXPECT_FALSE(decoder->SetSize(1 << 15, 1 << 15));
  EXPECT_TRUE(decoder->SetSize(1 << 28, 1));
  EXPECT_TRUE(decoder->SetSize(1, 1 << 28));
  EXPECT_TRUE(decoder->SetSize(1 << 14, 1 << 14));
}

TEST(ImageDecoderTest, requiredPreviousFrameIndex) {
  std::unique_ptr<TestImageDecoder> decoder(
      std::make_unique<TestImageDecoder>());
  decoder->InitFrames(6);
  Vector<ImageFrame, 1>& frame_buffers = decoder->FrameBufferCache();

  frame_buffers[1].SetDisposalMethod(ImageFrame::kDisposeKeep);
  frame_buffers[2].SetDisposalMethod(ImageFrame::kDisposeOverwritePrevious);
  frame_buffers[3].SetDisposalMethod(ImageFrame::kDisposeOverwritePrevious);
  frame_buffers[4].SetDisposalMethod(ImageFrame::kDisposeKeep);

  decoder->ResetRequiredPreviousFrames();

  // The first frame doesn't require any previous frame.
  EXPECT_EQ(kNotFound, frame_buffers[0].RequiredPreviousFrameIndex());
  // The previous DisposeNotSpecified frame is required.
  EXPECT_EQ(0u, frame_buffers[1].RequiredPreviousFrameIndex());
  // DisposeKeep is treated as DisposeNotSpecified.
  EXPECT_EQ(1u, frame_buffers[2].RequiredPreviousFrameIndex());
  // Previous DisposeOverwritePrevious frames are skipped.
  EXPECT_EQ(1u, frame_buffers[3].RequiredPreviousFrameIndex());
  EXPECT_EQ(1u, frame_buffers[4].RequiredPreviousFrameIndex());
  EXPECT_EQ(4u, frame_buffers[5].RequiredPreviousFrameIndex());
}

TEST(ImageDecoderTest, requiredPreviousFrameIndexDisposeOverwriteBgcolor) {
  std::unique_ptr<TestImageDecoder> decoder(
      std::make_unique<TestImageDecoder>());
  decoder->InitFrames(3);
  Vector<ImageFrame, 1>& frame_buffers = decoder->FrameBufferCache();

  // Fully covering DisposeOverwriteBgcolor previous frame resets the starting
  // state.
  frame_buffers[1].SetDisposalMethod(ImageFrame::kDisposeOverwriteBgcolor);
  decoder->ResetRequiredPreviousFrames();
  EXPECT_EQ(kNotFound, frame_buffers[2].RequiredPreviousFrameIndex());

  // Partially covering DisposeOverwriteBgcolor previous frame is required by
  // this frame.
  frame_buffers[1].SetOriginalFrameRect(IntRect(50, 50, 50, 50));
  decoder->ResetRequiredPreviousFrames();
  EXPECT_EQ(1u, frame_buffers[2].RequiredPreviousFrameIndex());
}

TEST(ImageDecoderTest, requiredPreviousFrameIndexForFrame1) {
  std::unique_ptr<TestImageDecoder> decoder(
      std::make_unique<TestImageDecoder>());
  decoder->InitFrames(2);
  Vector<ImageFrame, 1>& frame_buffers = decoder->FrameBufferCache();

  decoder->ResetRequiredPreviousFrames();
  EXPECT_EQ(0u, frame_buffers[1].RequiredPreviousFrameIndex());

  // The first frame with DisposeOverwritePrevious or DisposeOverwriteBgcolor
  // resets the starting state.
  frame_buffers[0].SetDisposalMethod(ImageFrame::kDisposeOverwritePrevious);
  decoder->ResetRequiredPreviousFrames();
  EXPECT_EQ(kNotFound, frame_buffers[1].RequiredPreviousFrameIndex());
  frame_buffers[0].SetDisposalMethod(ImageFrame::kDisposeOverwriteBgcolor);
  decoder->ResetRequiredPreviousFrames();
  EXPECT_EQ(kNotFound, frame_buffers[1].RequiredPreviousFrameIndex());

  // ... even if it partially covers.
  frame_buffers[0].SetOriginalFrameRect(IntRect(50, 50, 50, 50));

  frame_buffers[0].SetDisposalMethod(ImageFrame::kDisposeOverwritePrevious);
  decoder->ResetRequiredPreviousFrames();
  EXPECT_EQ(kNotFound, frame_buffers[1].RequiredPreviousFrameIndex());
  frame_buffers[0].SetDisposalMethod(ImageFrame::kDisposeOverwriteBgcolor);
  decoder->ResetRequiredPreviousFrames();
  EXPECT_EQ(kNotFound, frame_buffers[1].RequiredPreviousFrameIndex());
}

TEST(ImageDecoderTest, requiredPreviousFrameIndexBlendAtopBgcolor) {
  std::unique_ptr<TestImageDecoder> decoder(
      std::make_unique<TestImageDecoder>());
  decoder->InitFrames(3);
  Vector<ImageFrame, 1>& frame_buffers = decoder->FrameBufferCache();

  frame_buffers[1].SetOriginalFrameRect(IntRect(25, 25, 50, 50));
  frame_buffers[2].SetAlphaBlendSource(ImageFrame::kBlendAtopBgcolor);

  // A full frame with 'blending method == BlendAtopBgcolor' doesn't depend on
  // any prior frames.
  for (int dispose_method = ImageFrame::kDisposeNotSpecified;
       dispose_method <= ImageFrame::kDisposeOverwritePrevious;
       ++dispose_method) {
    frame_buffers[1].SetDisposalMethod(
        static_cast<ImageFrame::DisposalMethod>(dispose_method));
    decoder->ResetRequiredPreviousFrames();
    EXPECT_EQ(kNotFound, frame_buffers[2].RequiredPreviousFrameIndex());
  }

  // A non-full frame with 'blending method == BlendAtopBgcolor' does depend on
  // a prior frame.
  frame_buffers[2].SetOriginalFrameRect(IntRect(50, 50, 50, 50));
  for (int dispose_method = ImageFrame::kDisposeNotSpecified;
       dispose_method <= ImageFrame::kDisposeOverwritePrevious;
       ++dispose_method) {
    frame_buffers[1].SetDisposalMethod(
        static_cast<ImageFrame::DisposalMethod>(dispose_method));
    decoder->ResetRequiredPreviousFrames();
    EXPECT_NE(kNotFound, frame_buffers[2].RequiredPreviousFrameIndex());
  }
}

TEST(ImageDecoderTest, requiredPreviousFrameIndexKnownOpaque) {
  std::unique_ptr<TestImageDecoder> decoder(
      std::make_unique<TestImageDecoder>());
  decoder->InitFrames(3);
  Vector<ImageFrame, 1>& frame_buffers = decoder->FrameBufferCache();

  frame_buffers[1].SetOriginalFrameRect(IntRect(25, 25, 50, 50));

  // A full frame that is known to be opaque doesn't depend on any prior frames.
  for (int dispose_method = ImageFrame::kDisposeNotSpecified;
       dispose_method <= ImageFrame::kDisposeOverwritePrevious;
       ++dispose_method) {
    frame_buffers[1].SetDisposalMethod(
        static_cast<ImageFrame::DisposalMethod>(dispose_method));
    decoder->ResetRequiredPreviousFrames(true);
    EXPECT_EQ(kNotFound, frame_buffers[2].RequiredPreviousFrameIndex());
  }

  // A non-full frame that is known to be opaque does depend on a prior frame.
  frame_buffers[2].SetOriginalFrameRect(IntRect(50, 50, 50, 50));
  for (int dispose_method = ImageFrame::kDisposeNotSpecified;
       dispose_method <= ImageFrame::kDisposeOverwritePrevious;
       ++dispose_method) {
    frame_buffers[1].SetDisposalMethod(
        static_cast<ImageFrame::DisposalMethod>(dispose_method));
    decoder->ResetRequiredPreviousFrames(true);
    EXPECT_NE(kNotFound, frame_buffers[2].RequiredPreviousFrameIndex());
  }
}

TEST(ImageDecoderTest, clearCacheExceptFrameDoNothing) {
  std::unique_ptr<TestImageDecoder> decoder(
      std::make_unique<TestImageDecoder>());
  decoder->ClearCacheExceptFrame(0);

  // This should not crash.
  decoder->InitFrames(20);
  decoder->ClearCacheExceptFrame(kNotFound);
}

TEST(ImageDecoderTest, clearCacheExceptFrameAll) {
  const size_t kNumFrames = 10;
  std::unique_ptr<TestImageDecoder> decoder(
      std::make_unique<TestImageDecoder>());
  decoder->InitFrames(kNumFrames);
  Vector<ImageFrame, 1>& frame_buffers = decoder->FrameBufferCache();
  for (size_t i = 0; i < kNumFrames; ++i)
    frame_buffers[i].SetStatus(i % 2 ? ImageFrame::kFramePartial
                                     : ImageFrame::kFrameComplete);

  decoder->ClearCacheExceptFrame(kNotFound);

  for (size_t i = 0; i < kNumFrames; ++i) {
    SCOPED_TRACE(testing::Message() << i);
    EXPECT_EQ(ImageFrame::kFrameEmpty, frame_buffers[i].GetStatus());
  }
}

TEST(ImageDecoderTest, clearCacheExceptFramePreverveClearExceptFrame) {
  const size_t kNumFrames = 10;
  std::unique_ptr<TestImageDecoder> decoder(
      std::make_unique<TestImageDecoder>());
  decoder->InitFrames(kNumFrames);
  Vector<ImageFrame, 1>& frame_buffers = decoder->FrameBufferCache();
  for (size_t i = 0; i < kNumFrames; ++i)
    frame_buffers[i].SetStatus(ImageFrame::kFrameComplete);

  decoder->ResetRequiredPreviousFrames();
  decoder->ClearCacheExceptFrame(5);
  for (size_t i = 0; i < kNumFrames; ++i) {
    SCOPED_TRACE(testing::Message() << i);
    if (i == 5)
      EXPECT_EQ(ImageFrame::kFrameComplete, frame_buffers[i].GetStatus());
    else
      EXPECT_EQ(ImageFrame::kFrameEmpty, frame_buffers[i].GetStatus());
  }
}

}  // namespace blink
