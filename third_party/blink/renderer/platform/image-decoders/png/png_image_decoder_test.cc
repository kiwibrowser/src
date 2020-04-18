// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/image-decoders/png/png_image_decoder.h"

#include <memory>
#include "png.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/image-decoders/image_decoder_test_helpers.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

// /LayoutTests/images/resources/png-animated-idat-part-of-animation.png
// is modified in multiple tests to simulate erroneous PNGs. As a reference,
// the table below shows how the file is structured.
//
// Offset | 8     33    95    133   172   210   241   279   314   352   422
// -------------------------------------------------------------------------
// Chunk  | IHDR  acTL  fcTL  IDAT  fcTL  fdAT  fcTL  fdAT  fcTL  fdAT  IEND
//
// In between the acTL and fcTL there are two other chunks, PLTE and tRNS, but
// those are not specifically used in this test suite. The same holds for a
// tEXT chunk in between the last fdAT and IEND.
//
// In the current behavior of PNG image decoders, the 4 frames are detected when
// respectively 141, 249, 322 and 430 bytes are received. The first frame should
// be detected when the IDAT has been received, and non-first frames when the
// next fcTL or IEND chunk has been received. Note that all offsets are +8,
// because a chunk is identified by byte 4-7.

namespace blink {

namespace {

std::unique_ptr<ImageDecoder> CreatePNGDecoder(
    ImageDecoder::AlphaOption alpha_option) {
  return std::make_unique<PNGImageDecoder>(
      alpha_option, ColorBehavior::TransformToSRGB(),
      ImageDecoder::kNoDecodedImageByteLimit);
}

std::unique_ptr<ImageDecoder> CreatePNGDecoder() {
  return CreatePNGDecoder(ImageDecoder::kAlphaNotPremultiplied);
}

std::unique_ptr<ImageDecoder> CreatePNGDecoderWithPngData(
    const char* png_file) {
  auto decoder = CreatePNGDecoder();
  scoped_refptr<SharedBuffer> data = ReadFile(png_file);
  EXPECT_FALSE(data->IsEmpty());
  decoder->SetData(data.get(), true);
  return decoder;
}

void TestSize(const char* png_file, IntSize expected_size) {
  auto decoder = CreatePNGDecoderWithPngData(png_file);
  EXPECT_TRUE(decoder->IsSizeAvailable());
  EXPECT_EQ(expected_size, decoder->Size());
}

// Test whether querying for the size of the image works if we present the
// data byte by byte.
void TestSizeByteByByte(const char* png_file,
                        size_t bytes_needed_to_decode_size,
                        IntSize expected_size) {
  auto decoder = CreatePNGDecoder();
  scoped_refptr<SharedBuffer> data = ReadFile(png_file);
  ASSERT_FALSE(data->IsEmpty());
  ASSERT_LT(bytes_needed_to_decode_size, data->size());

  const char* source = data->Data();
  scoped_refptr<SharedBuffer> partial_data = SharedBuffer::Create();
  for (size_t length = 1; length <= bytes_needed_to_decode_size; length++) {
    partial_data->Append(source++, 1u);
    decoder->SetData(partial_data.get(), false);

    if (length < bytes_needed_to_decode_size) {
      EXPECT_FALSE(decoder->IsSizeAvailable());
      EXPECT_TRUE(decoder->Size().IsEmpty());
      EXPECT_FALSE(decoder->Failed());
    } else {
      EXPECT_TRUE(decoder->IsSizeAvailable());
      EXPECT_EQ(expected_size, decoder->Size());
    }
  }
  EXPECT_FALSE(decoder->Failed());
}

void WriteUint32(uint32_t val, png_byte* data) {
  data[0] = val >> 24;
  data[1] = val >> 16;
  data[2] = val >> 8;
  data[3] = val;
}

void TestRepetitionCount(const char* png_file, int expected_repetition_count) {
  auto decoder = CreatePNGDecoderWithPngData(png_file);
  // Decoding the frame count sets the number of repetitions as well.
  decoder->FrameCount();
  EXPECT_FALSE(decoder->Failed());
  EXPECT_EQ(expected_repetition_count, decoder->RepetitionCount());
}

struct PublicFrameInfo {
  TimeDelta duration;
  IntRect frame_rect;
  ImageFrame::AlphaBlendSource alpha_blend;
  ImageFrame::DisposalMethod disposal_method;
};

// This is the frame data for the following PNG image:
// /LayoutTests/images/resources/png-animated-idat-part-of-animation.png
static PublicFrameInfo g_png_animated_frame_info[] = {
    {TimeDelta::FromMilliseconds(500),
     {IntPoint(0, 0), IntSize(5, 5)},
     ImageFrame::kBlendAtopBgcolor,
     ImageFrame::kDisposeKeep},
    {TimeDelta::FromMilliseconds(900),
     {IntPoint(1, 1), IntSize(3, 1)},
     ImageFrame::kBlendAtopBgcolor,
     ImageFrame::kDisposeOverwriteBgcolor},
    {TimeDelta::FromMilliseconds(2000),
     {IntPoint(1, 2), IntSize(3, 2)},
     ImageFrame::kBlendAtopPreviousFrame,
     ImageFrame::kDisposeKeep},
    {TimeDelta::FromMilliseconds(1500),
     {IntPoint(1, 2), IntSize(3, 1)},
     ImageFrame::kBlendAtopBgcolor,
     ImageFrame::kDisposeKeep},
};

void CompareFrameWithExpectation(const PublicFrameInfo& expected,
                                 ImageDecoder* decoder,
                                 size_t index) {
  EXPECT_EQ(expected.duration, decoder->FrameDurationAtIndex(index));

  const auto* frame = decoder->DecodeFrameBufferAtIndex(index);
  ASSERT_TRUE(frame);

  EXPECT_EQ(expected.duration, frame->Duration());
  EXPECT_EQ(expected.frame_rect, frame->OriginalFrameRect());
  EXPECT_EQ(expected.disposal_method, frame->GetDisposalMethod());
  EXPECT_EQ(expected.alpha_blend, frame->GetAlphaBlendSource());
}

// This function removes |length| bytes at |offset|, and then calls FrameCount.
// It assumes the missing bytes should result in a failed decode because the
// parser jumps |length| bytes too far in the next chunk.
void TestMissingDataBreaksDecoding(const char* png_file,
                                   size_t offset,
                                   size_t length) {
  auto decoder = CreatePNGDecoder();
  scoped_refptr<SharedBuffer> data = ReadFile(png_file);
  ASSERT_FALSE(data->IsEmpty());

  scoped_refptr<SharedBuffer> invalid_data =
      SharedBuffer::Create(data->Data(), offset);
  invalid_data->Append(data->Data() + offset + length,
                       data->size() - offset - length);
  ASSERT_EQ(data->size() - length, invalid_data->size());

  decoder->SetData(invalid_data, true);
  decoder->FrameCount();
  EXPECT_TRUE(decoder->Failed());
}

// Verify that a decoder with a parse error converts to a static image.
static void ExpectStatic(ImageDecoder* decoder) {
  EXPECT_EQ(1u, decoder->FrameCount());
  EXPECT_FALSE(decoder->Failed());

  ImageFrame* frame = decoder->DecodeFrameBufferAtIndex(0);
  ASSERT_NE(nullptr, frame);
  EXPECT_EQ(ImageFrame::kFrameComplete, frame->GetStatus());
  EXPECT_FALSE(decoder->Failed());
  EXPECT_EQ(kAnimationNone, decoder->RepetitionCount());
}

// Decode up to the indicated fcTL offset and then provide an fcTL with the
// wrong chunk size (20 instead of 26).
void TestInvalidFctlSize(const char* png_file,
                         size_t offset_fctl,
                         size_t expected_frame_count,
                         bool should_fail) {
  scoped_refptr<SharedBuffer> data = ReadFile(png_file);
  ASSERT_FALSE(data->IsEmpty());

  auto decoder = CreatePNGDecoder();
  scoped_refptr<SharedBuffer> invalid_data =
      SharedBuffer::Create(data->Data(), offset_fctl);

  // Test if this gives the correct frame count, before the fcTL is parsed.
  decoder->SetData(invalid_data, false);
  EXPECT_EQ(expected_frame_count, decoder->FrameCount());
  ASSERT_FALSE(decoder->Failed());

  // Append the wrong size to the data stream
  png_byte size_chunk[4];
  WriteUint32(20, size_chunk);
  invalid_data->Append(reinterpret_cast<char*>(size_chunk), 4u);

  // Skip the size in the original data, but provide a truncated fcTL,
  // which is 4B of tag, 20B of data and 4B of CRC, totalling 28B.
  invalid_data->Append(data->Data() + offset_fctl + 4, 28u);
  // Append the rest of the data
  const size_t offset_post_fctl = offset_fctl + 38;
  invalid_data->Append(data->Data() + offset_post_fctl,
                       data->size() - offset_post_fctl);

  decoder->SetData(invalid_data, false);
  if (should_fail) {
    EXPECT_EQ(expected_frame_count, decoder->FrameCount());
    EXPECT_EQ(true, decoder->Failed());
  } else {
    ExpectStatic(decoder.get());
  }
}

// Verify that the decoder can successfully decode the first frame when
// initially only half of the frame data is received, resulting in a partially
// decoded image, and then the rest of the image data is received. Verify that
// the bitmap hashes of the two stages are different. Also verify that the final
// bitmap hash is equivalent to the hash when all data is provided at once.
//
// This verifies that the decoder correctly keeps track of where it stopped
// decoding when the image was not yet fully received.
void TestProgressiveDecodingContinuesAfterFullData(
    const char* png_file,
    size_t offset_mid_first_frame) {
  scoped_refptr<SharedBuffer> full_data = ReadFile(png_file);
  ASSERT_FALSE(full_data->IsEmpty());

  auto decoder_upfront = CreatePNGDecoder();
  decoder_upfront->SetData(full_data.get(), true);
  EXPECT_GE(decoder_upfront->FrameCount(), 1u);
  const ImageFrame* const frame_upfront =
      decoder_upfront->DecodeFrameBufferAtIndex(0);
  ASSERT_EQ(ImageFrame::kFrameComplete, frame_upfront->GetStatus());
  const unsigned hash_upfront = HashBitmap(frame_upfront->Bitmap());

  auto decoder = CreatePNGDecoder();
  scoped_refptr<SharedBuffer> partial_data =
      SharedBuffer::Create(full_data->Data(), offset_mid_first_frame);
  decoder->SetData(partial_data, false);

  EXPECT_EQ(1u, decoder->FrameCount());
  const ImageFrame* frame = decoder->DecodeFrameBufferAtIndex(0);
  EXPECT_EQ(frame->GetStatus(), ImageFrame::kFramePartial);
  const unsigned hash_partial = HashBitmap(frame->Bitmap());

  decoder->SetData(full_data.get(), true);
  frame = decoder->DecodeFrameBufferAtIndex(0);
  EXPECT_EQ(frame->GetStatus(), ImageFrame::kFrameComplete);
  const unsigned hash_full = HashBitmap(frame->Bitmap());

  EXPECT_FALSE(decoder->Failed());
  EXPECT_NE(hash_full, hash_partial);
  EXPECT_EQ(hash_full, hash_upfront);
}

}  // Anonymous namespace

// Animated PNG Tests

TEST(AnimatedPNGTests, sizeTest) {
  TestSize(
      "/images/resources/"
      "png-animated-idat-part-of-animation.png",
      IntSize(5, 5));
  TestSize(
      "/images/resources/"
      "png-animated-idat-not-part-of-animation.png",
      IntSize(227, 35));
}

TEST(AnimatedPNGTests, repetitionCountTest) {
  TestRepetitionCount(
      "/images/resources/"
      "png-animated-idat-part-of-animation.png",
      6u);
  // This is an "animated" image with only one frame, that is, the IDAT is
  // ignored and there is one fdAT frame. so it should be considered
  // non-animated.
  TestRepetitionCount(
      "/images/resources/"
      "png-animated-idat-not-part-of-animation.png",
      kAnimationNone);
}

// Test if the decoded metdata corresponds to the defined expectations
TEST(AnimatedPNGTests, MetaDataTest) {
  const char* png_file =
      "/images/resources/"
      "png-animated-idat-part-of-animation.png";
  constexpr size_t kExpectedFrameCount = 4;

  auto decoder = CreatePNGDecoderWithPngData(png_file);
  ASSERT_EQ(kExpectedFrameCount, decoder->FrameCount());
  for (size_t i = 0; i < kExpectedFrameCount; i++) {
    CompareFrameWithExpectation(g_png_animated_frame_info[i], decoder.get(), i);
  }
}

TEST(AnimatedPNGTests, EmptyFrame) {
  const char* png_file = "/images/resources/empty-frame.png";
  auto decoder = CreatePNGDecoderWithPngData(png_file);
  // Frame 0 is empty. Ensure that decoding frame 1 (which depends on frame 0)
  // fails (rather than crashing).
  EXPECT_EQ(2u, decoder->FrameCount());
  EXPECT_FALSE(decoder->Failed());

  ImageFrame* frame = decoder->DecodeFrameBufferAtIndex(1);
  EXPECT_TRUE(decoder->Failed());
  ASSERT_NE(nullptr, frame);
  EXPECT_EQ(ImageFrame::kFrameEmpty, frame->GetStatus());
}

TEST(AnimatedPNGTests, ByteByByteSizeAvailable) {
  TestSizeByteByByte(
      "/images/resources/"
      "png-animated-idat-part-of-animation.png",
      141u, IntSize(5, 5));
  TestSizeByteByByte(
      "/images/resources/"
      "png-animated-idat-not-part-of-animation.png",
      79u, IntSize(227, 35));
}

TEST(AnimatedPNGTests, ByteByByteMetaData) {
  const char* png_file =
      "/images/resources/"
      "png-animated-idat-part-of-animation.png";
  constexpr size_t kExpectedFrameCount = 4;

  // These are the byte offsets where each frame should have been parsed.
  // It boils down to the offset of the first fcTL / IEND after the last
  // frame data chunk, plus 8 bytes for recognition. The exception on this is
  // the first frame, which is reported when its first framedata is seen.
  size_t frame_offsets[kExpectedFrameCount] = {141, 249, 322, 430};

  auto decoder = CreatePNGDecoder();
  scoped_refptr<SharedBuffer> data = ReadFile(png_file);
  ASSERT_FALSE(data->IsEmpty());
  size_t frames_parsed = 0;

  const char* source = data->Data();
  scoped_refptr<SharedBuffer> partial_data = SharedBuffer::Create();
  for (size_t length = 1; length <= frame_offsets[kExpectedFrameCount - 1];
       length++) {
    partial_data->Append(source++, 1u);
    decoder->SetData(partial_data.get(), false);
    EXPECT_FALSE(decoder->Failed());
    if (length < frame_offsets[frames_parsed]) {
      EXPECT_EQ(frames_parsed, decoder->FrameCount());
    } else {
      ASSERT_EQ(frames_parsed + 1, decoder->FrameCount());
      CompareFrameWithExpectation(g_png_animated_frame_info[frames_parsed],
                                  decoder.get(), frames_parsed);
      frames_parsed++;
    }
  }
  EXPECT_EQ(kExpectedFrameCount, decoder->FrameCount());
  EXPECT_FALSE(decoder->Failed());
}

TEST(AnimatedPNGTests, TestRandomFrameDecode) {
  TestRandomFrameDecode(&CreatePNGDecoder,
                        "/images/resources/"
                        "png-animated-idat-part-of-animation.png",
                        2u);
}

TEST(AnimatedPNGTests, TestDecodeAfterReallocation) {
  TestDecodeAfterReallocatingData(&CreatePNGDecoder,
                                  "/images/resources/"
                                  "png-animated-idat-part-of-animation.png");
}

TEST(AnimatedPNGTests, ProgressiveDecode) {
  TestProgressiveDecoding(&CreatePNGDecoder,
                          "/images/resources/"
                          "png-animated-idat-part-of-animation.png",
                          13u);
}

TEST(AnimatedPNGTests, ParseAndDecodeByteByByte) {
  TestByteByByteDecode(&CreatePNGDecoder,
                       "/images/resources/"
                       "png-animated-idat-part-of-animation.png",
                       4u, 6u);
}

TEST(AnimatedPNGTests, FailureDuringParsing) {
  // Test the first fcTL in the stream. Because no frame data has been set at
  // this point, the expected frame count is zero. 95 bytes is just before the
  // first fcTL chunk, at which the first frame is detected. This is before the
  // IDAT, so it should be treated as a static image.
  TestInvalidFctlSize(
      "/images/resources/"
      "png-animated-idat-part-of-animation.png",
      95u, 0u, false);

  // Test for the third fcTL in the stream. This should see 1 frame before the
  // fcTL, and then fail when parsing it.
  TestInvalidFctlSize(
      "/images/resources/"
      "png-animated-idat-part-of-animation.png",
      241u, 1u, true);
}

TEST(AnimatedPNGTests, ActlErrors) {
  const char* png_file =
      "/images/resources/"
      "png-animated-idat-part-of-animation.png";
  scoped_refptr<SharedBuffer> data = ReadFile(png_file);
  ASSERT_FALSE(data->IsEmpty());

  const size_t kOffsetActl = 33u;
  const size_t kAcTLSize = 20u;
  {
    // Remove the acTL chunk from the stream. This results in a static image.
    scoped_refptr<SharedBuffer> no_actl_data =
        SharedBuffer::Create(data->Data(), kOffsetActl);
    no_actl_data->Append(data->Data() + kOffsetActl + kAcTLSize,
                         data->size() - kOffsetActl - kAcTLSize);

    auto decoder = CreatePNGDecoder();
    decoder->SetData(no_actl_data, true);
    EXPECT_EQ(1u, decoder->FrameCount());
    EXPECT_FALSE(decoder->Failed());
    EXPECT_EQ(kAnimationNone, decoder->RepetitionCount());
  }

  // Store the acTL for more tests.
  char ac_tl[kAcTLSize];
  memcpy(ac_tl, data->Data() + kOffsetActl, kAcTLSize);

  // Insert an extra acTL at a couple of different offsets.
  // Prior to the IDAT, this should result in a static image. After, this
  // should fail.
  const struct {
    size_t offset;
    bool should_fail;
  } kGRecs[] = {{8u, false},
                {kOffsetActl, false},
                {133u, false},
                {172u, true},
                {422u, true}};
  for (const auto& rec : kGRecs) {
    const size_t offset = rec.offset;
    scoped_refptr<SharedBuffer> extra_actl_data =
        SharedBuffer::Create(data->Data(), offset);
    extra_actl_data->Append(ac_tl, kAcTLSize);
    extra_actl_data->Append(data->Data() + offset, data->size() - offset);
    auto decoder = CreatePNGDecoder();
    decoder->SetData(extra_actl_data, true);
    EXPECT_EQ(rec.should_fail ? 0u : 1u, decoder->FrameCount());
    EXPECT_EQ(rec.should_fail, decoder->Failed());
  }

  // An acTL after IDAT is ignored.
  png_file =
      "/images/resources/"
      "cHRM_color_spin.png";
  {
    scoped_refptr<SharedBuffer> data2 = ReadFile(png_file);
    ASSERT_FALSE(data2->IsEmpty());
    const size_t kPostIDATOffset = 30971u;
    for (size_t times = 0; times < 2; times++) {
      scoped_refptr<SharedBuffer> extra_actl_data =
          SharedBuffer::Create(data2->Data(), kPostIDATOffset);
      for (size_t i = 0; i < times; i++)
        extra_actl_data->Append(ac_tl, kAcTLSize);
      extra_actl_data->Append(data2->Data() + kPostIDATOffset,
                              data2->size() - kPostIDATOffset);

      auto decoder = CreatePNGDecoder();
      decoder->SetData(extra_actl_data, true);
      EXPECT_EQ(1u, decoder->FrameCount());
      EXPECT_FALSE(decoder->Failed());
      EXPECT_EQ(kAnimationNone, decoder->RepetitionCount());
      EXPECT_NE(nullptr, decoder->DecodeFrameBufferAtIndex(0));
      EXPECT_FALSE(decoder->Failed());
    }
  }
}

TEST(AnimatedPNGTests, fdatBeforeIdat) {
  const char* png_file =
      "/images/resources/"
      "png-animated-idat-not-part-of-animation.png";
  scoped_refptr<SharedBuffer> data = ReadFile(png_file);
  ASSERT_FALSE(data->IsEmpty());

  // Insert fcTL and fdAT prior to the IDAT
  const size_t kIdatOffset = 71u;
  scoped_refptr<SharedBuffer> modified_data =
      SharedBuffer::Create(data->Data(), kIdatOffset);
  // Copy fcTL and fdAT
  const size_t kFctlPlusFdatSize = 38u + 1566u;
  modified_data->Append(data->Data() + 2519u, kFctlPlusFdatSize);
  // Copy IDAT
  modified_data->Append(data->Data() + kIdatOffset, 2448u);
  // Copy the remaining
  modified_data->Append(data->Data() + 4123u, 39u + 12u);
  // Data has just been rearranged.
  ASSERT_EQ(data->size(), modified_data->size());

  {
    // This broken APNG will be treated as a static png.
    auto decoder = CreatePNGDecoder();
    decoder->SetData(modified_data.get(), true);
    ExpectStatic(decoder.get());
  }

  {
    // Remove the acTL from the modified image. It now has fdAT before
    // IDAT, but no acTL, so fdAT should be ignored.
    const size_t kOffsetActl = 33u;
    const size_t kAcTLSize = 20u;
    scoped_refptr<SharedBuffer> modified_data2 =
        SharedBuffer::Create(modified_data->Data(), kOffsetActl);
    modified_data2->Append(modified_data->Data() + kOffsetActl + kAcTLSize,
                           modified_data->size() - kOffsetActl - kAcTLSize);
    auto decoder = CreatePNGDecoder();
    decoder->SetData(modified_data2.get(), true);
    ExpectStatic(decoder.get());

    // Likewise, if an acTL follows the fdAT, it is ignored.
    const size_t kInsertionOffset = kIdatOffset + kFctlPlusFdatSize - kAcTLSize;
    scoped_refptr<SharedBuffer> modified_data3 =
        SharedBuffer::Create(modified_data2->Data(), kInsertionOffset);
    modified_data3->Append(data->Data() + kOffsetActl, kAcTLSize);
    modified_data3->Append(modified_data2->Data() + kInsertionOffset,
                           modified_data2->size() - kInsertionOffset);
    decoder = CreatePNGDecoder();
    decoder->SetData(modified_data3.get(), true);
    ExpectStatic(decoder.get());
  }
}

TEST(AnimatedPNGTests, IdatSizeMismatch) {
  // The default image must fill the image
  const char* png_file =
      "/images/resources/"
      "png-animated-idat-part-of-animation.png";
  scoped_refptr<SharedBuffer> data = ReadFile(png_file);
  ASSERT_FALSE(data->IsEmpty());

  const size_t kFctlOffset = 95u;
  scoped_refptr<SharedBuffer> modified_data =
      SharedBuffer::Create(data->Data(), kFctlOffset);
  const size_t kFctlSize = 38u;
  png_byte fctl[kFctlSize];
  memcpy(fctl, data->Data() + kFctlOffset, kFctlSize);
  // Set the height to a smaller value, so it does not fill the image.
  WriteUint32(3, fctl + 16);
  // Correct the crc
  WriteUint32(3210324191, fctl + 34);
  modified_data->Append((const char*)fctl, kFctlSize);
  const size_t kAfterFctl = kFctlOffset + kFctlSize;
  modified_data->Append(data->Data() + kAfterFctl, data->size() - kAfterFctl);

  auto decoder = CreatePNGDecoder();
  decoder->SetData(modified_data.get(), true);
  ExpectStatic(decoder.get());
}

// Originally, the third frame has an offset of (1,2) and a size of (3,2). By
// changing the offset to (4,4), the frame rect is no longer within the image
// size of 5x5. This results in a failure.
TEST(AnimatedPNGTests, VerifyFrameOutsideImageSizeFails) {
  const char* png_file =
      "/images/resources/"
      "png-animated-idat-part-of-animation.png";
  scoped_refptr<SharedBuffer> data = ReadFile(png_file);
  auto decoder = CreatePNGDecoder();
  ASSERT_FALSE(data->IsEmpty());

  const size_t kOffsetThirdFctl = 241;
  scoped_refptr<SharedBuffer> modified_data =
      SharedBuffer::Create(data->Data(), kOffsetThirdFctl);
  const size_t kFctlSize = 38u;
  png_byte fctl[kFctlSize];
  memcpy(fctl, data->Data() + kOffsetThirdFctl, kFctlSize);
  // Modify offset and crc.
  WriteUint32(4, fctl + 20u);
  WriteUint32(4, fctl + 24u);
  WriteUint32(3700322018, fctl + 34u);

  modified_data->Append(const_cast<const char*>(reinterpret_cast<char*>(fctl)),
                        kFctlSize);
  modified_data->Append(data->Data() + kOffsetThirdFctl + kFctlSize,
                        data->size() - kOffsetThirdFctl - kFctlSize);

  decoder->SetData(modified_data, true);

  IntSize expected_size(5, 5);
  EXPECT_TRUE(decoder->IsSizeAvailable());
  EXPECT_EQ(expected_size, decoder->Size());

  const size_t kExpectedFrameCount = 0;
  EXPECT_EQ(kExpectedFrameCount, decoder->FrameCount());
  EXPECT_TRUE(decoder->Failed());
}

TEST(AnimatedPNGTests, ProgressiveDecodingContinuesAfterFullData) {
  // 160u is a randomly chosen offset in the IDAT chunk of the first frame.
  TestProgressiveDecodingContinuesAfterFullData(
      "/images/resources/"
      "png-animated-idat-part-of-animation.png",
      160u);
}

TEST(AnimatedPNGTests, RandomDecodeAfterClearFrameBufferCache) {
  TestRandomDecodeAfterClearFrameBufferCache(
      &CreatePNGDecoder,
      "/images/resources/"
      "png-animated-idat-part-of-animation.png",
      2u);
}

TEST(AnimatedPNGTests, VerifyAlphaBlending) {
  TestAlphaBlending(&CreatePNGDecoder,
                    "/images/resources/"
                    "png-animated-idat-part-of-animation.png");
}

// This tests if the frame count gets set correctly when parsing FrameCount
// fails in one of the parsing queries.
//
// First, enough data is provided such that two frames should be registered.
// The decoder should at this point not be in the failed status.
//
// Then, we provide the rest of the data except for the last IEND chunk, but
// tell the decoder that this is all the data we have.  The frame count should
// be three, since one extra frame should be discovered. The fourth frame
// should *not* be registered since the reader should not be able to determine
// where the frame ends. The decoder should *not* be in the failed state since
// there are three frames which can be shown.
// Attempting to decode the third frame should fail, since the file is
// truncated.
TEST(AnimatedPNGTests, FailureMissingIendChunk) {
  scoped_refptr<SharedBuffer> full_data = ReadFile(
      "/images/resources/"
      "png-animated-idat-part-of-animation.png");
  ASSERT_FALSE(full_data->IsEmpty());
  auto decoder = CreatePNGDecoder();

  const size_t kOffsetTwoFrames = 249;
  const size_t kExpectedFramesAfter249Bytes = 2;
  scoped_refptr<SharedBuffer> temp_data =
      SharedBuffer::Create(full_data->Data(), kOffsetTwoFrames);
  decoder->SetData(temp_data.get(), false);
  EXPECT_EQ(kExpectedFramesAfter249Bytes, decoder->FrameCount());
  EXPECT_FALSE(decoder->Failed());

  // Provide the rest of the data except for the last IEND chunk.
  const size_t kExpectedFramesAfterAllExcept12Bytes = 3;
  temp_data = SharedBuffer::Create(full_data->Data(), full_data->size() - 12);
  decoder->SetData(temp_data.get(), true);
  ASSERT_EQ(kExpectedFramesAfterAllExcept12Bytes, decoder->FrameCount());

  for (size_t i = 0; i < kExpectedFramesAfterAllExcept12Bytes; i++) {
    EXPECT_FALSE(decoder->Failed());
    decoder->DecodeFrameBufferAtIndex(i);
  }

  EXPECT_TRUE(decoder->Failed());
}

// Verify that a malformatted PNG, where the IEND appears before any frame data
// (IDAT), invalidates the decoder.
TEST(AnimatedPNGTests, VerifyIENDBeforeIDATInvalidatesDecoder) {
  scoped_refptr<SharedBuffer> full_data = ReadFile(
      "/images/resources/"
      "png-animated-idat-part-of-animation.png");
  ASSERT_FALSE(full_data->IsEmpty());
  auto decoder = CreatePNGDecoder();

  const size_t kOffsetIDAT = 133;
  scoped_refptr<SharedBuffer> data =
      SharedBuffer::Create(full_data->Data(), kOffsetIDAT);
  data->Append(full_data->Data() + full_data->size() - 12u, 12u);
  data->Append(full_data->Data() + kOffsetIDAT,
               full_data->size() - kOffsetIDAT);
  decoder->SetData(data.get(), true);

  const size_t kExpectedFrameCount = 0u;
  EXPECT_EQ(kExpectedFrameCount, decoder->FrameCount());
  EXPECT_TRUE(decoder->Failed());
}

// All IDAT chunks must be before all fdAT chunks
TEST(AnimatedPNGTests, MixedDataChunks) {
  const char* png_file =
      "/images/resources/"
      "png-animated-idat-part-of-animation.png";
  scoped_refptr<SharedBuffer> full_data = ReadFile(png_file);
  ASSERT_FALSE(full_data->IsEmpty());

  // Add an extra fdAT after the first IDAT, skipping fcTL.
  const size_t kPostIDAT = 172u;
  scoped_refptr<SharedBuffer> data =
      SharedBuffer::Create(full_data->Data(), kPostIDAT);
  const size_t kFcTLSize = 38u;
  const size_t kFdATSize = 31u;
  png_byte fd_at[kFdATSize];
  memcpy(fd_at, full_data->Data() + kPostIDAT + kFcTLSize, kFdATSize);
  // Modify the sequence number
  WriteUint32(1u, fd_at + 8);
  data->Append((const char*)fd_at, kFdATSize);
  const size_t kIENDOffset = 422u;
  data->Append(full_data->Data() + kIENDOffset,
               full_data->size() - kIENDOffset);
  auto decoder = CreatePNGDecoder();
  decoder->SetData(data.get(), true);
  decoder->FrameCount();
  EXPECT_TRUE(decoder->Failed());

  // Insert an IDAT after an fdAT.
  const size_t kPostfdAT = kPostIDAT + kFcTLSize + kFdATSize;
  data = SharedBuffer::Create(full_data->Data(), kPostfdAT);
  const size_t kIDATOffset = 133u;
  data->Append(full_data->Data() + kIDATOffset, kPostIDAT - kIDATOffset);
  // Append the rest.
  data->Append(full_data->Data() + kPostIDAT, full_data->size() - kPostIDAT);
  decoder = CreatePNGDecoder();
  decoder->SetData(data.get(), true);
  decoder->FrameCount();
  EXPECT_TRUE(decoder->Failed());
}

// Verify that erroneous values for the disposal method and alpha blending
// cause the decoder to fail.
TEST(AnimatedPNGTests, VerifyInvalidDisposalAndBlending) {
  const char* png_file =
      "/images/resources/"
      "png-animated-idat-part-of-animation.png";
  scoped_refptr<SharedBuffer> full_data = ReadFile(png_file);
  ASSERT_FALSE(full_data->IsEmpty());
  auto decoder = CreatePNGDecoder();

  // The disposal byte in the frame control chunk is the 24th byte, alpha
  // blending the 25th. |kOffsetDisposalOp| is 241 bytes to get to the third
  // fctl chunk, 8 bytes to skip the length and tag bytes, and 24 bytes to get
  // to the disposal op.
  //
  // Write invalid values to the disposal and alpha blending byte, correct the
  // crc and append the rest of the buffer.
  const size_t kOffsetDisposalOp = 241 + 8 + 24;
  scoped_refptr<SharedBuffer> data =
      SharedBuffer::Create(full_data->Data(), kOffsetDisposalOp);
  png_byte disposal_and_blending[6u];
  disposal_and_blending[0] = 7;
  disposal_and_blending[1] = 9;
  WriteUint32(2408835439u, disposal_and_blending + 2u);
  data->Append(reinterpret_cast<char*>(disposal_and_blending), 6u);
  data->Append(full_data->Data() + kOffsetDisposalOp + 6u,
               full_data->size() - kOffsetDisposalOp - 6u);

  decoder->SetData(data.get(), true);
  decoder->FrameCount();
  ASSERT_TRUE(decoder->Failed());
}

// This test verifies that the following situation does not invalidate the
// decoder:
// - Frame 0 is decoded progressively, but there's not enough data to fully
//   decode it.
// - The rest of the image data is received.
// - Frame X, with X > 0, and X does not depend on frame 0, is decoded.
// - Frame 0 is decoded.
// This is a tricky case since the decoder resets the png struct for each frame,
// and this test verifies that it does not break the decoding of frame 0, even
// though it already started in the first call.
TEST(AnimatedPNGTests, VerifySuccessfulFirstFrameDecodeAfterLaterFrame) {
  const char* png_file =
      "/images/resources/"
      "png-animated-three-independent-frames.png";
  auto decoder = CreatePNGDecoder();
  scoped_refptr<SharedBuffer> full_data = ReadFile(png_file);
  ASSERT_FALSE(full_data->IsEmpty());

  // 160u is a randomly chosen offset in the IDAT chunk of the first frame.
  const size_t kMiddleFirstFrame = 160u;
  scoped_refptr<SharedBuffer> data =
      SharedBuffer::Create(full_data->Data(), kMiddleFirstFrame);
  decoder->SetData(data.get(), false);

  ASSERT_EQ(1u, decoder->FrameCount());
  ASSERT_EQ(ImageFrame::kFramePartial,
            decoder->DecodeFrameBufferAtIndex(0)->GetStatus());

  decoder->SetData(full_data.get(), true);
  ASSERT_EQ(3u, decoder->FrameCount());
  ASSERT_EQ(ImageFrame::kFrameComplete,
            decoder->DecodeFrameBufferAtIndex(1)->GetStatus());
  // The point is that this call does not decode frame 0, which it won't do if
  // it does not have it as its required previous frame.
  ASSERT_EQ(kNotFound,
            decoder->DecodeFrameBufferAtIndex(1)->RequiredPreviousFrameIndex());

  EXPECT_EQ(ImageFrame::kFrameComplete,
            decoder->DecodeFrameBufferAtIndex(0)->GetStatus());
  EXPECT_FALSE(decoder->Failed());
}

// If the decoder attempts to decode a non-first frame which is subset and
// independent, it needs to discard its png_struct so it can use a modified
// IHDR. Test this by comparing a decode of frame 1 after frame 0 to a decode
// of frame 1 without decoding frame 0.
TEST(AnimatedPNGTests, DecodeFromIndependentFrame) {
  const char* png_file =
      "/images/resources/"
      "png-animated-idat-part-of-animation.png";
  scoped_refptr<SharedBuffer> original_data = ReadFile(png_file);
  ASSERT_FALSE(original_data->IsEmpty());

  // This file almost fits the bill. Modify it to dispose frame 0, making
  // frame 1 independent.
  const size_t kDisposeOffset = 127u;
  auto data = SharedBuffer::Create(original_data->Data(), kDisposeOffset);
  // 1 Corresponds to APNG_DISPOSE_OP_BACKGROUND
  const char kOne = '\001';
  data->Append(&kOne, 1u);
  // No need to modify the blend op
  data->Append(original_data->Data() + kDisposeOffset + 1, 1u);
  // Modify the CRC
  png_byte crc[4];
  WriteUint32(2226670956, crc);
  data->Append(reinterpret_cast<const char*>(crc), 4u);
  data->Append(original_data->Data() + data->size(),
               original_data->size() - data->size());
  ASSERT_EQ(original_data->size(), data->size());

  auto decoder = CreatePNGDecoder();
  decoder->SetData(data.get(), true);

  ASSERT_EQ(4u, decoder->FrameCount());
  ASSERT_FALSE(decoder->Failed());

  auto* frame = decoder->DecodeFrameBufferAtIndex(0);
  ASSERT_TRUE(frame);
  ASSERT_EQ(ImageFrame::kDisposeOverwriteBgcolor, frame->GetDisposalMethod());

  frame = decoder->DecodeFrameBufferAtIndex(1);
  ASSERT_TRUE(frame);
  ASSERT_FALSE(decoder->Failed());
  ASSERT_NE(IntRect({}, decoder->Size()), frame->OriginalFrameRect());
  ASSERT_EQ(kNotFound, frame->RequiredPreviousFrameIndex());

  const auto hash = HashBitmap(frame->Bitmap());

  // Now decode starting from frame 1.
  decoder = CreatePNGDecoder();
  decoder->SetData(data.get(), true);

  frame = decoder->DecodeFrameBufferAtIndex(1);
  ASSERT_TRUE(frame);
  EXPECT_EQ(hash, HashBitmap(frame->Bitmap()));
}

// If the first frame is subset from IHDR (only allowed if the first frame is
// not the default image), the decoder has to destroy the png_struct it used
// for parsing so it can use a modified IHDR.
TEST(AnimatedPNGTests, SubsetFromIHDR) {
  const char* png_file =
      "/images/resources/"
      "png-animated-idat-not-part-of-animation.png";
  scoped_refptr<SharedBuffer> original_data = ReadFile(png_file);
  ASSERT_FALSE(original_data->IsEmpty());

  const size_t kFcTLOffset = 2519u;
  auto data = SharedBuffer::Create(original_data->Data(), kFcTLOffset);

  const size_t kFcTLSize = 38u;
  png_byte fc_tl[kFcTLSize];
  memcpy(fc_tl, original_data->Data() + kFcTLOffset, kFcTLSize);
  // Modify to have a subset frame (yOffset 1, height 34 out of 35).
  WriteUint32(34, fc_tl + 16u);
  WriteUint32(1, fc_tl + 24u);
  WriteUint32(3972842751, fc_tl + 34u);
  data->Append(reinterpret_cast<const char*>(fc_tl), kFcTLSize);

  // Append the rest of the data.
  // Note: If PNGImageDecoder changes to reject an image with too many
  // rows, the fdAT data will need to be modified as well.
  data->Append(original_data->Data() + kFcTLOffset + kFcTLSize,
               original_data->size() - data->size());
  ASSERT_EQ(original_data->size(), data->size());

  // This will test both byte by byte and using the full data, and compare.
  TestByteByByteDecode(CreatePNGDecoder, data.get(), 1, kAnimationNone);
}

TEST(AnimatedPNGTests, Offset) {
  const char* png_file = "/images/resources/apng18.png";
  scoped_refptr<SharedBuffer> original_data = ReadFile(png_file);
  ASSERT_FALSE(original_data->IsEmpty());

  Vector<unsigned> baseline_hashes;
  CreateDecodingBaseline(CreatePNGDecoder, original_data.get(),
                         &baseline_hashes);
  constexpr size_t kExpectedFrameCount = 13;
  ASSERT_EQ(kExpectedFrameCount, baseline_hashes.size());

  constexpr size_t kOffset = 37;
  char buffer[kOffset] = {};

  auto data = SharedBuffer::Create(buffer, kOffset);
  data->Append(*original_data.get());

  // Use the same defaults as CreatePNGDecoder, except use the (arbitrary)
  // non-zero offset.
  auto decoder = std::make_unique<PNGImageDecoder>(
      ImageDecoder::kAlphaNotPremultiplied, ColorBehavior::TransformToSRGB(),
      ImageDecoder::kNoDecodedImageByteLimit, kOffset);
  decoder->SetData(data, true);
  ASSERT_EQ(kExpectedFrameCount, decoder->FrameCount());

  for (size_t i = 0; i < kExpectedFrameCount; ++i) {
    auto* frame = decoder->DecodeFrameBufferAtIndex(i);
    EXPECT_EQ(baseline_hashes[i], HashBitmap(frame->Bitmap()));
  }
}

TEST(AnimatedPNGTests, ExtraChunksBeforeIHDR) {
  const char* png_file = "/images/resources/apng18.png";
  scoped_refptr<SharedBuffer> original_data = ReadFile(png_file);
  ASSERT_FALSE(original_data->IsEmpty());

  Vector<unsigned> baseline_hashes;
  CreateDecodingBaseline(CreatePNGDecoder, original_data.get(),
                         &baseline_hashes);
  constexpr size_t kExpectedFrameCount = 13;
  ASSERT_EQ(kExpectedFrameCount, baseline_hashes.size());

  constexpr size_t kPngSignatureSize = 8;
  auto data = SharedBuffer::Create(original_data->Data(), kPngSignatureSize);

  // Arbitrary chunk of data.
  constexpr size_t kExtraChunkSize = 13;
  constexpr png_byte kExtraChunk[kExtraChunkSize] = {
      0, 0, 0, 1, 't', 'R', 'c', 'N', 68, 82, 0, 87, 10};
  data->Append(reinterpret_cast<const char*>(kExtraChunk), kExtraChunkSize);

  // Append the rest of the data from the original.
  data->Append(original_data->Data() + kPngSignatureSize,
               original_data->size() - kPngSignatureSize);
  ASSERT_EQ(original_data->size() + kExtraChunkSize, data->size());

  auto decoder = CreatePNGDecoder();
  decoder->SetData(data, true);
  ASSERT_EQ(kExpectedFrameCount, decoder->FrameCount());

  for (size_t i = 0; i < kExpectedFrameCount; ++i) {
    auto* frame = decoder->DecodeFrameBufferAtIndex(i);
    EXPECT_EQ(baseline_hashes[i], HashBitmap(frame->Bitmap()));
  }
}

// Static PNG tests

TEST(StaticPNGTests, repetitionCountTest) {
  TestRepetitionCount("/images/resources/png-simple.png", kAnimationNone);
}

TEST(StaticPNGTests, sizeTest) {
  TestSize("/images/resources/png-simple.png", IntSize(111, 29));
}

TEST(StaticPNGTests, MetaDataTest) {
  const size_t kExpectedFrameCount = 1;
  const TimeDelta kExpectedDuration;
  auto decoder =
      CreatePNGDecoderWithPngData("/images/resources/png-simple.png");
  EXPECT_EQ(kExpectedFrameCount, decoder->FrameCount());
  EXPECT_EQ(kExpectedDuration, decoder->FrameDurationAtIndex(0));
}

TEST(StaticPNGTests, InvalidIHDRChunk) {
  TestMissingDataBreaksDecoding("/images/resources/png-simple.png", 20u, 2u);
}

TEST(StaticPNGTests, ProgressiveDecoding) {
  TestProgressiveDecoding(&CreatePNGDecoder, "/images/resources/png-simple.png",
                          11u);
}

TEST(StaticPNGTests, ProgressiveDecodingContinuesAfterFullData) {
  TestProgressiveDecodingContinuesAfterFullData(
      "/images/resources/png-simple.png", 1000u);
}

TEST(PNGTests, VerifyFrameCompleteBehavior) {
  struct {
    const char* name;
    size_t expected_frame_count;
    size_t offset_in_first_frame;
  } g_recs[] = {
      {"/images/resources/"
       "png-animated-three-independent-frames.png",
       3u, 150u},
      {"/images/resources/"
       "png-animated-idat-part-of-animation.png",
       4u, 160u},

      {"/images/resources/png-simple.png", 1u, 700u},
      {"/images/resources/lenna.png", 1u, 40000u},
  };
  for (const auto& rec : g_recs) {
    scoped_refptr<SharedBuffer> full_data = ReadFile(rec.name);
    ASSERT_TRUE(full_data.get());

    // Create with enough data for part of the first frame.
    auto decoder = CreatePNGDecoder();
    auto data =
        SharedBuffer::Create(full_data->Data(), rec.offset_in_first_frame);
    decoder->SetData(data.get(), false);

    EXPECT_FALSE(decoder->FrameIsReceivedAtIndex(0));

    // Parsing the size is not enough to mark the frame as complete.
    EXPECT_TRUE(decoder->IsSizeAvailable());
    EXPECT_FALSE(decoder->FrameIsReceivedAtIndex(0));

    const auto partial_frame_count = decoder->FrameCount();
    EXPECT_EQ(1u, partial_frame_count);

    // Frame is not complete, even after decoding partially.
    EXPECT_FALSE(decoder->FrameIsReceivedAtIndex(0));
    auto* frame = decoder->DecodeFrameBufferAtIndex(0);
    ASSERT_TRUE(frame);
    EXPECT_NE(ImageFrame::kFrameComplete, frame->GetStatus());
    EXPECT_FALSE(decoder->FrameIsReceivedAtIndex(0));

    decoder->SetData(full_data.get(), true);

    // With full data, parsing the size still does not mark a frame as
    // complete for animated images.
    EXPECT_TRUE(decoder->IsSizeAvailable());
    if (rec.expected_frame_count > 1)
      EXPECT_FALSE(decoder->FrameIsReceivedAtIndex(0));
    else
      EXPECT_TRUE(decoder->FrameIsReceivedAtIndex(0));

    const auto frame_count = decoder->FrameCount();
    ASSERT_EQ(rec.expected_frame_count, frame_count);

    // After parsing (the full file), all frames are complete.
    for (size_t i = 0; i < frame_count; ++i)
      EXPECT_TRUE(decoder->FrameIsReceivedAtIndex(i));

    frame = decoder->DecodeFrameBufferAtIndex(0);
    ASSERT_TRUE(frame);
    EXPECT_EQ(ImageFrame::kFrameComplete, frame->GetStatus());
    EXPECT_TRUE(decoder->FrameIsReceivedAtIndex(0));
  }
}

TEST(PNGTests, sizeMayOverflow) {
  auto decoder =
      CreatePNGDecoderWithPngData("/images/resources/crbug702934.png");
  EXPECT_FALSE(decoder->IsSizeAvailable());
  EXPECT_TRUE(decoder->Failed());
}

TEST(PNGTests, truncated) {
  auto decoder =
      CreatePNGDecoderWithPngData("/images/resources/crbug807324.png");

  // An update to libpng (without using the libpng-provided workaround) resulted
  // in truncating this image. It has no transparency, so no pixel should be
  // transparent.
  auto* frame = decoder->DecodeFrameBufferAtIndex(0);
  auto size = decoder->Size();
  for (int i = 0; i < size.Width();  ++i)
  for (int j = 0; j < size.Height(); ++j) {
    ASSERT_NE(SK_ColorTRANSPARENT, *frame->GetAddr(i, j));
  }
}

TEST(PNGTests, crbug827754) {
  const char* png_file = "/images/resources/crbug827754.png";
  scoped_refptr<SharedBuffer> data = ReadFile(png_file);
  ASSERT_TRUE(data);

  auto decoder = CreatePNGDecoder();
  decoder->SetData(data.get(), true);
  auto* frame = decoder->DecodeFrameBufferAtIndex(0);
  ASSERT_TRUE(frame);
  ASSERT_FALSE(decoder->Failed());
}

};  // namespace blink
