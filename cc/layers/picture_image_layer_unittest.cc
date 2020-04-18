// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layers/picture_image_layer.h"

#include "cc/animation/animation_host.h"
#include "cc/paint/paint_image_builder.h"
#include "cc/test/fake_layer_tree_host.h"
#include "cc/test/skia_common.h"
#include "cc/test/test_task_graph_runner.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkSurface.h"

namespace cc {
namespace {

TEST(PictureImageLayerTest, PaintContentsToDisplayList) {
  scoped_refptr<PictureImageLayer> layer = PictureImageLayer::Create();
  FakeLayerTreeHostClient client;
  TestTaskGraphRunner task_graph_runner;
  auto animation_host = AnimationHost::CreateForTesting(ThreadInstance::MAIN);
  std::unique_ptr<FakeLayerTreeHost> host = FakeLayerTreeHost::Create(
      &client, &task_graph_runner, animation_host.get());
  layer->SetLayerTreeHost(host.get());
  gfx::Rect layer_rect(200, 200);

  unsigned char image_pixels[4 * 200 * 200] = {0};
  SkImageInfo info =
      SkImageInfo::MakeN32Premul(layer_rect.width(), layer_rect.height());
  sk_sp<SkSurface> image_surface =
      SkSurface::MakeRasterDirect(info, image_pixels, info.minRowBytes());
  SkCanvas* image_canvas = image_surface->getCanvas();
  image_canvas->clear(SK_ColorRED);
  SkPaint blue_paint;
  blue_paint.setColor(SK_ColorBLUE);
  image_canvas->drawRect(SkRect::MakeWH(100, 100), blue_paint);
  image_canvas->drawRect(SkRect::MakeLTRB(100, 100, 200, 200), blue_paint);

  layer->SetImage(PaintImageBuilder::WithDefault()
                      .set_id(PaintImage::GetNextId())
                      .set_image(image_surface->makeImageSnapshot(),
                                 PaintImage::GetNextContentId())
                      .TakePaintImage(),
                  SkMatrix::I(), false);
  layer->SetBounds(gfx::Size(layer_rect.width(), layer_rect.height()));

  scoped_refptr<DisplayItemList> display_list =
      layer->PaintContentsToDisplayList(
          ContentLayerClient::PAINTING_BEHAVIOR_NORMAL);
  unsigned char actual_pixels[4 * 200 * 200] = {0};
  DrawDisplayList(actual_pixels, layer_rect, display_list);

  EXPECT_EQ(0, memcmp(actual_pixels, image_pixels, 4 * 200 * 200));

  layer->SetLayerTreeHost(nullptr);
}

}  // namespace
}  // namespace cc
