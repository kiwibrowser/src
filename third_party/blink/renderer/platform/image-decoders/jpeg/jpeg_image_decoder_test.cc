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

#include "third_party/blink/renderer/platform/image-decoders/jpeg/jpeg_image_decoder.h"

#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_data.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/renderer/platform/image-decoders/image_animation.h"
#include "third_party/blink/renderer/platform/image-decoders/image_decoder_test_helpers.h"
#include "third_party/blink/renderer/platform/shared_buffer.h"
#include "third_party/blink/renderer/platform/wtf/typed_arrays/array_buffer.h"

namespace blink {

static const size_t kLargeEnoughSize = 1000 * 1000;

namespace {

std::unique_ptr<ImageDecoder> CreateJPEGDecoder(size_t max_decoded_bytes) {
  return std::make_unique<JPEGImageDecoder>(
      ImageDecoder::kAlphaNotPremultiplied, ColorBehavior::TransformToSRGB(),
      max_decoded_bytes);
}

std::unique_ptr<ImageDecoder> CreateJPEGDecoder() {
  return CreateJPEGDecoder(ImageDecoder::kNoDecodedImageByteLimit);
}

}  // anonymous namespace

void Downsample(size_t max_decoded_bytes,
                unsigned* output_width,
                unsigned* output_height,
                const char* image_file_path) {
  scoped_refptr<SharedBuffer> data = ReadFile(image_file_path);
  ASSERT_TRUE(data);

  std::unique_ptr<ImageDecoder> decoder = CreateJPEGDecoder(max_decoded_bytes);
  decoder->SetData(data.get(), true);

  ImageFrame* frame = decoder->DecodeFrameBufferAtIndex(0);
  ASSERT_TRUE(frame);
  *output_width = frame->Bitmap().width();
  *output_height = frame->Bitmap().height();
  EXPECT_EQ(IntSize(*output_width, *output_height), decoder->DecodedSize());
}

void ReadYUV(size_t max_decoded_bytes,
             unsigned* output_y_width,
             unsigned* output_y_height,
             unsigned* output_uv_width,
             unsigned* output_uv_height,
             const char* image_file_path) {
  scoped_refptr<SharedBuffer> data = ReadFile(image_file_path);
  ASSERT_TRUE(data);

  std::unique_ptr<ImageDecoder> decoder = CreateJPEGDecoder(max_decoded_bytes);
  decoder->SetData(data.get(), true);

  // Setting a dummy ImagePlanes object signals to the decoder that we want to
  // do YUV decoding.
  std::unique_ptr<ImagePlanes> dummy_image_planes =
      std::make_unique<ImagePlanes>();
  decoder->SetImagePlanes(std::move(dummy_image_planes));

  bool size_is_available = decoder->IsSizeAvailable();
  ASSERT_TRUE(size_is_available);

  IntSize size = decoder->DecodedSize();
  IntSize y_size = decoder->DecodedYUVSize(0);
  IntSize u_size = decoder->DecodedYUVSize(1);
  IntSize v_size = decoder->DecodedYUVSize(2);

  ASSERT_TRUE(size.Width() == y_size.Width());
  ASSERT_TRUE(size.Height() == y_size.Height());
  ASSERT_TRUE(u_size.Width() == v_size.Width());
  ASSERT_TRUE(u_size.Height() == v_size.Height());

  *output_y_width = y_size.Width();
  *output_y_height = y_size.Height();
  *output_uv_width = u_size.Width();
  *output_uv_height = u_size.Height();

  size_t row_bytes[3];
  row_bytes[0] = decoder->DecodedYUVWidthBytes(0);
  row_bytes[1] = decoder->DecodedYUVWidthBytes(1);
  row_bytes[2] = decoder->DecodedYUVWidthBytes(2);

  scoped_refptr<ArrayBuffer> buffer(ArrayBuffer::Create(
      row_bytes[0] * y_size.Height() + row_bytes[1] * u_size.Height() +
          row_bytes[2] * v_size.Height(),
      1));
  void* planes[3];
  planes[0] = buffer->Data();
  planes[1] = ((char*)planes[0]) + row_bytes[0] * y_size.Height();
  planes[2] = ((char*)planes[1]) + row_bytes[1] * u_size.Height();

  std::unique_ptr<ImagePlanes> image_planes =
      std::make_unique<ImagePlanes>(planes, row_bytes);
  decoder->SetImagePlanes(std::move(image_planes));

  ASSERT_TRUE(decoder->DecodeToYUV());
}

// Tests failure on a too big image.
TEST(JPEGImageDecoderTest, tooBig) {
  std::unique_ptr<ImageDecoder> decoder = CreateJPEGDecoder(100);
  EXPECT_FALSE(decoder->SetSize(10000, 10000));
  EXPECT_TRUE(decoder->Failed());
}

// Tests that the JPEG decoder can downsample image whose width and height are
// multiples of 8, to ensure we compute the correct DecodedSize and pass correct
// parameters to libjpeg to output the image with the expected size.
TEST(JPEGImageDecoderTest, downsampleImageSizeMultipleOf8) {
  const char* jpeg_file = "/images/resources/lenna.jpg";  // 256x256
  unsigned output_width, output_height;

  // 1/8 downsample.
  Downsample(40 * 40 * 4, &output_width, &output_height, jpeg_file);
  EXPECT_EQ(32u, output_width);
  EXPECT_EQ(32u, output_height);

  // 2/8 downsample.
  Downsample(70 * 70 * 4, &output_width, &output_height, jpeg_file);
  EXPECT_EQ(64u, output_width);
  EXPECT_EQ(64u, output_height);

  // 3/8 downsample.
  Downsample(100 * 100 * 4, &output_width, &output_height, jpeg_file);
  EXPECT_EQ(96u, output_width);
  EXPECT_EQ(96u, output_height);

  // 4/8 downsample.
  Downsample(130 * 130 * 4, &output_width, &output_height, jpeg_file);
  EXPECT_EQ(128u, output_width);
  EXPECT_EQ(128u, output_height);

  // 5/8 downsample.
  Downsample(170 * 170 * 4, &output_width, &output_height, jpeg_file);
  EXPECT_EQ(160u, output_width);
  EXPECT_EQ(160u, output_height);

  // 6/8 downsample.
  Downsample(200 * 200 * 4, &output_width, &output_height, jpeg_file);
  EXPECT_EQ(192u, output_width);
  EXPECT_EQ(192u, output_height);

  // 7/8 downsample.
  Downsample(230 * 230 * 4, &output_width, &output_height, jpeg_file);
  EXPECT_EQ(224u, output_width);
  EXPECT_EQ(224u, output_height);
}

// Tests that JPEG decoder can downsample image whose width and height are not
// multiple of 8. Ensures that we round using the same algorithm as libjpeg.
TEST(JPEGImageDecoderTest, downsampleImageSizeNotMultipleOf8) {
  const char* jpeg_file = "/images/resources/icc-v2-gbr.jpg";  // 275x207
  unsigned output_width, output_height;

  // 1/8 downsample.
  Downsample(40 * 40 * 4, &output_width, &output_height, jpeg_file);
  EXPECT_EQ(35u, output_width);
  EXPECT_EQ(26u, output_height);

  // 2/8 downsample.
  Downsample(70 * 70 * 4, &output_width, &output_height, jpeg_file);
  EXPECT_EQ(69u, output_width);
  EXPECT_EQ(52u, output_height);

  // 3/8 downsample.
  Downsample(100 * 100 * 4, &output_width, &output_height, jpeg_file);
  EXPECT_EQ(104u, output_width);
  EXPECT_EQ(78u, output_height);

  // 4/8 downsample.
  Downsample(130 * 130 * 4, &output_width, &output_height, jpeg_file);
  EXPECT_EQ(138u, output_width);
  EXPECT_EQ(104u, output_height);

  // 5/8 downsample.
  Downsample(170 * 170 * 4, &output_width, &output_height, jpeg_file);
  EXPECT_EQ(172u, output_width);
  EXPECT_EQ(130u, output_height);

  // 6/8 downsample.
  Downsample(200 * 200 * 4, &output_width, &output_height, jpeg_file);
  EXPECT_EQ(207u, output_width);
  EXPECT_EQ(156u, output_height);

  // 7/8 downsample.
  Downsample(230 * 230 * 4, &output_width, &output_height, jpeg_file);
  EXPECT_EQ(241u, output_width);
  EXPECT_EQ(182u, output_height);
}

// Tests that upsampling is not allowed.
TEST(JPEGImageDecoderTest, upsample) {
  const char* jpeg_file = "/images/resources/lenna.jpg";  // 256x256
  unsigned output_width, output_height;
  Downsample(kLargeEnoughSize, &output_width, &output_height, jpeg_file);
  EXPECT_EQ(256u, output_width);
  EXPECT_EQ(256u, output_height);
}

TEST(JPEGImageDecoderTest, yuv) {
  const char* jpeg_file = "/images/resources/lenna.jpg";  // 256x256, YUV 4:2:0
  unsigned output_y_width, output_y_height, output_uv_width, output_uv_height;
  ReadYUV(kLargeEnoughSize, &output_y_width, &output_y_height, &output_uv_width,
          &output_uv_height, jpeg_file);
  EXPECT_EQ(256u, output_y_width);
  EXPECT_EQ(256u, output_y_height);
  EXPECT_EQ(128u, output_uv_width);
  EXPECT_EQ(128u, output_uv_height);

  const char* jpeg_file_image_size_not_multiple_of8 =
      "/images/resources/cropped_mandrill.jpg";  // 439x154
  ReadYUV(kLargeEnoughSize, &output_y_width, &output_y_height, &output_uv_width,
          &output_uv_height, jpeg_file_image_size_not_multiple_of8);
  EXPECT_EQ(439u, output_y_width);
  EXPECT_EQ(154u, output_y_height);
  EXPECT_EQ(220u, output_uv_width);
  EXPECT_EQ(77u, output_uv_height);

  // Make sure we revert to RGBA decoding when we're about to downscale,
  // which can occur on memory-constrained android devices.
  scoped_refptr<SharedBuffer> data = ReadFile(jpeg_file);
  ASSERT_TRUE(data);

  std::unique_ptr<ImageDecoder> decoder = CreateJPEGDecoder(230 * 230 * 4);
  decoder->SetData(data.get(), true);

  std::unique_ptr<ImagePlanes> image_planes = std::make_unique<ImagePlanes>();
  decoder->SetImagePlanes(std::move(image_planes));
  ASSERT_TRUE(decoder->IsSizeAvailable());
  ASSERT_FALSE(decoder->CanDecodeToYUV());
}

TEST(JPEGImageDecoderTest,
     byteByByteBaselineJPEGWithColorProfileAndRestartMarkers) {
  TestByteByByteDecode(&CreateJPEGDecoder,
                       "/images/resources/"
                       "small-square-with-colorspin-profile.jpg",
                       1u, kAnimationNone);
}

TEST(JPEGImageDecoderTest, byteByByteProgressiveJPEG) {
  TestByteByByteDecode(&CreateJPEGDecoder, "/images/resources/bug106024.jpg",
                       1u, kAnimationNone);
}

TEST(JPEGImageDecoderTest, byteByByteRGBJPEGWithAdobeMarkers) {
  TestByteByByteDecode(&CreateJPEGDecoder,
                       "/images/resources/rgb-jpeg-with-adobe-marker-only.jpg",
                       1u, kAnimationNone);
}

// This test verifies that calling SharedBuffer::MergeSegmentsIntoBuffer() does
// not break JPEG decoding at a critical point: in between a call to decode the
// size (when JPEGImageDecoder stops while it may still have input data to
// read) and a call to do a full decode.
TEST(JPEGImageDecoderTest, mergeBuffer) {
  const char* jpeg_file = "/images/resources/lenna.jpg";
  TestMergeBuffer(&CreateJPEGDecoder, jpeg_file);
}

// This tests decoding a JPEG with many progressive scans.  Decoding should
// fail, but not hang (crbug.com/642462).
TEST(JPEGImageDecoderTest, manyProgressiveScans) {
  scoped_refptr<SharedBuffer> test_data =
      ReadFile(kDecodersTestingDir, "many-progressive-scans.jpg");
  ASSERT_TRUE(test_data.get());

  std::unique_ptr<ImageDecoder> test_decoder = CreateJPEGDecoder();
  test_decoder->SetData(test_data.get(), true);
  EXPECT_EQ(1u, test_decoder->FrameCount());
  ASSERT_TRUE(test_decoder->DecodeFrameBufferAtIndex(0));
  EXPECT_TRUE(test_decoder->Failed());
}

TEST(JPEGImageDecoderTest, SupportedSizesSquare) {
  const char* jpeg_file = "/images/resources/lenna.jpg";  // 256x256
  scoped_refptr<SharedBuffer> data = ReadFile(jpeg_file);
  ASSERT_TRUE(data);

  std::unique_ptr<ImageDecoder> decoder =
      CreateJPEGDecoder(std::numeric_limits<int>::max());
  decoder->SetData(data.get(), true);
  // This will decode the size and needs to be called to avoid DCHECKs
  ASSERT_TRUE(decoder->IsSizeAvailable());
  std::vector<SkISize> expected_sizes = {
      SkISize::Make(32, 32),   SkISize::Make(64, 64),   SkISize::Make(96, 96),
      SkISize::Make(128, 128), SkISize::Make(160, 160), SkISize::Make(192, 192),
      SkISize::Make(224, 224), SkISize::Make(256, 256)};
  auto sizes = decoder->GetSupportedDecodeSizes();
  ASSERT_EQ(expected_sizes.size(), sizes.size());
  for (size_t i = 0; i < sizes.size(); ++i) {
    EXPECT_TRUE(expected_sizes[i] == sizes[i])
        << "Expected " << expected_sizes[i].width() << "x"
        << expected_sizes[i].height() << ". Got " << sizes[i].width() << "x"
        << sizes[i].height();
  }
}

TEST(JPEGImageDecoderTest, SupportedSizesRectangle) {
  const char* jpeg_file = "/images/resources/icc-v2-gbr.jpg";  // 275x207

  scoped_refptr<SharedBuffer> data = ReadFile(jpeg_file);
  ASSERT_TRUE(data);

  std::unique_ptr<ImageDecoder> decoder =
      CreateJPEGDecoder(std::numeric_limits<int>::max());
  decoder->SetData(data.get(), true);
  // This will decode the size and needs to be called to avoid DCHECKs
  ASSERT_TRUE(decoder->IsSizeAvailable());
  std::vector<SkISize> expected_sizes = {
      SkISize::Make(35, 26),   SkISize::Make(69, 52),   SkISize::Make(104, 78),
      SkISize::Make(138, 104), SkISize::Make(172, 130), SkISize::Make(207, 156),
      SkISize::Make(241, 182), SkISize::Make(275, 207)};

  auto sizes = decoder->GetSupportedDecodeSizes();
  ASSERT_EQ(expected_sizes.size(), sizes.size());
  for (size_t i = 0; i < sizes.size(); ++i) {
    EXPECT_TRUE(expected_sizes[i] == sizes[i])
        << "Expected " << expected_sizes[i].width() << "x"
        << expected_sizes[i].height() << ". Got " << sizes[i].width() << "x"
        << sizes[i].height();
  }
}

TEST(JPEGImageDecoderTest, SupportedSizesTruncatedIfMemoryBound) {
  const char* jpeg_file = "/images/resources/lenna.jpg";  // 256x256
  scoped_refptr<SharedBuffer> data = ReadFile(jpeg_file);
  ASSERT_TRUE(data);

  // Limit the memory so that 128 would be the largest size possible.
  std::unique_ptr<ImageDecoder> decoder = CreateJPEGDecoder(130 * 130 * 4);
  decoder->SetData(data.get(), true);
  // This will decode the size and needs to be called to avoid DCHECKs
  ASSERT_TRUE(decoder->IsSizeAvailable());
  std::vector<SkISize> expected_sizes = {
      SkISize::Make(32, 32), SkISize::Make(64, 64), SkISize::Make(96, 96),
      SkISize::Make(128, 128)};
  auto sizes = decoder->GetSupportedDecodeSizes();
  ASSERT_EQ(expected_sizes.size(), sizes.size());
  for (size_t i = 0; i < sizes.size(); ++i) {
    EXPECT_TRUE(expected_sizes[i] == sizes[i])
        << "Expected " << expected_sizes[i].width() << "x"
        << expected_sizes[i].height() << ". Got " << sizes[i].width() << "x"
        << sizes[i].height();
  }
}

}  // namespace blink
