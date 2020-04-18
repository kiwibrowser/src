/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "third_party/blink/renderer/platform/graphics/image.h"

#include <memory>

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/testing/fake_graphics_layer.h"
#include "third_party/blink/renderer/platform/testing/fake_graphics_layer_client.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkSurface.h"

namespace blink {

namespace {

class TestImage : public Image {
 public:
  static scoped_refptr<TestImage> Create(const IntSize& size, bool opaque) {
    return base::AdoptRef(new TestImage(size, opaque));
  }

  bool CurrentFrameKnownToBeOpaque() override { return image_->isOpaque(); }

  IntSize Size() const override { return size_; }

  void DestroyDecodedData() override {
    // Image pure virtual stub.
  }

  void Draw(PaintCanvas*,
            const PaintFlags&,
            const FloatRect&,
            const FloatRect&,
            RespectImageOrientationEnum,
            ImageClampingMode,
            ImageDecodingMode) override {
    // Image pure virtual stub.
  }

  PaintImage PaintImageForCurrentFrame() override {
    return CreatePaintImageBuilder()
        .set_image(image_, PaintImage::GetNextContentId())
        .TakePaintImage();
  }

 private:
  TestImage(IntSize size, bool opaque) : Image(nullptr), size_(size) {
    sk_sp<SkSurface> surface = CreateSkSurface(size, opaque);
    if (!surface)
      return;

    surface->getCanvas()->clear(SK_ColorTRANSPARENT);
    image_ = surface->makeImageSnapshot();
  }

  static sk_sp<SkSurface> CreateSkSurface(IntSize size, bool opaque) {
    return SkSurface::MakeRaster(SkImageInfo::MakeN32(
        size.Width(), size.Height(),
        opaque ? kOpaque_SkAlphaType : kPremul_SkAlphaType));
  }

  IntSize size_;
  sk_sp<SkImage> image_;
};

}  // anonymous namespace

TEST(ImageLayerChromiumTest, imageLayerContentReset) {
  FakeGraphicsLayerClient client;
  std::unique_ptr<FakeGraphicsLayer> graphics_layer =
      std::make_unique<FakeGraphicsLayer>(client);
  ASSERT_TRUE(graphics_layer.get());

  ASSERT_FALSE(graphics_layer->HasContentsLayer());
  ASSERT_FALSE(graphics_layer->ContentsLayer());

  bool opaque = false;
  scoped_refptr<Image> image = TestImage::Create(IntSize(100, 100), opaque);
  ASSERT_TRUE(image.get());

  graphics_layer->SetContentsToImage(image.get(), Image::kUnspecifiedDecode);
  ASSERT_TRUE(graphics_layer->HasContentsLayer());
  ASSERT_TRUE(graphics_layer->ContentsLayer());

  graphics_layer->SetContentsToImage(nullptr, Image::kUnspecifiedDecode);
  ASSERT_FALSE(graphics_layer->HasContentsLayer());
  ASSERT_FALSE(graphics_layer->ContentsLayer());
}

TEST(ImageLayerChromiumTest, opaqueImages) {
  FakeGraphicsLayerClient client;
  std::unique_ptr<FakeGraphicsLayer> graphics_layer =
      std::make_unique<FakeGraphicsLayer>(client);
  ASSERT_TRUE(graphics_layer.get());

  bool opaque = true;
  scoped_refptr<Image> opaque_image =
      TestImage::Create(IntSize(100, 100), opaque);
  ASSERT_TRUE(opaque_image.get());
  scoped_refptr<Image> non_opaque_image =
      TestImage::Create(IntSize(100, 100), !opaque);
  ASSERT_TRUE(non_opaque_image.get());

  ASSERT_FALSE(graphics_layer->ContentsLayer());

  graphics_layer->SetContentsToImage(opaque_image.get(),
                                     Image::kUnspecifiedDecode);
  ASSERT_TRUE(graphics_layer->ContentsLayer()->contents_opaque());

  graphics_layer->SetContentsToImage(non_opaque_image.get(),
                                     Image::kUnspecifiedDecode);
  ASSERT_FALSE(graphics_layer->ContentsLayer()->contents_opaque());
}

}  // namespace blink
