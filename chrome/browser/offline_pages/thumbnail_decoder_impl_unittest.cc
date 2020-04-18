// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/thumbnail_decoder_impl.h"

#include "base/test/bind_test_util.h"
#include "base/test/mock_callback.h"
#include "components/image_fetcher/core/mock_image_decoder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_unittest_util.h"

namespace offline_pages {
namespace {
using testing::_;
using testing::Invoke;

const char kImageData[] = "abc123";

class ThumbnailDecoderImplTest : public testing::Test {
 public:
  void SetUp() override {
    auto decoder = std::make_unique<image_fetcher::MockImageDecoder>();
    image_decoder = decoder.get();
    thumbnail_decoder =
        std::make_unique<ThumbnailDecoderImpl>(std::move(decoder));
  }
  image_fetcher::MockImageDecoder* image_decoder;
  std::unique_ptr<ThumbnailDecoderImpl> thumbnail_decoder;
};

TEST_F(ThumbnailDecoderImplTest, Success) {
  const int kImageWidth = 4;
  const int kImageHeight = 2;
  const gfx::Image kDecodedImage =
      gfx::test::CreateImage(kImageWidth, kImageHeight);
  EXPECT_CALL(*image_decoder, DecodeImage(kImageData, _, _))
      .WillOnce(testing::WithArg<2>(
          Invoke([&](const image_fetcher::ImageDecodedCallback& callback) {
            callback.Run(kDecodedImage);
          })));
  base::MockCallback<ThumbnailDecoder::DecodeComplete> complete_callback;
  EXPECT_CALL(complete_callback, Run(_))
      .WillOnce(Invoke([&](const gfx::Image& image) {
        EXPECT_EQ(kImageHeight, image.Width());
        EXPECT_EQ(kImageHeight, image.Height());
      }));
  thumbnail_decoder->DecodeAndCropThumbnail(kImageData,
                                            complete_callback.Get());
}

TEST_F(ThumbnailDecoderImplTest, DecodeFail) {
  const gfx::Image kDecodedImage = gfx::Image();
  EXPECT_CALL(*image_decoder, DecodeImage(kImageData, _, _))
      .WillOnce(testing::WithArg<2>(
          Invoke([&](const image_fetcher::ImageDecodedCallback& callback) {
            callback.Run(kDecodedImage);
          })));
  base::MockCallback<ThumbnailDecoder::DecodeComplete> complete_callback;
  EXPECT_CALL(complete_callback, Run(_))
      .WillOnce(Invoke(
          [](const gfx::Image& image) { EXPECT_TRUE(image.IsEmpty()); }));
  thumbnail_decoder->DecodeAndCropThumbnail(kImageData,
                                            complete_callback.Get());
}

}  // namespace
}  // namespace offline_pages
