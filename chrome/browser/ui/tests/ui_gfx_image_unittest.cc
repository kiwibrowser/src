// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_unittest_util.h"

#if defined(TOOLKIT_VIEWS)
#include "ui/views/controls/image_view.h"
#include "ui/views/view.h"
#endif

namespace {

#if defined(TOOLKIT_VIEWS)
TEST(UiGfxImageTest, ViewsImageView) {
  gfx::Image image(gfx::test::CreatePlatformImage());

  std::unique_ptr<views::View> container(new views::View());
  container->SetBounds(0, 0, 200, 200);
  container->SetVisible(true);

  std::unique_ptr<views::ImageView> image_view(new views::ImageView());
  image_view->SetImage(*image.ToImageSkia());
  container->AddChildView(image_view.get());
}
#endif

}  // namespace
