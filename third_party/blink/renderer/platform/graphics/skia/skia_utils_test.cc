// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/skia/skia_utils.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class SkiaUtilsTest : public testing::Test {};

// Tests converting a SkColorSpace to a gfx::ColorSpace
TEST_F(SkiaUtilsTest, SkColorSpaceToGfxColorSpace) {
  std::vector<sk_sp<SkColorSpace>> skia_color_spaces;

  SkColorSpace::RenderTargetGamma gammas[] = {
      SkColorSpace::kLinear_RenderTargetGamma,
      SkColorSpace::kSRGB_RenderTargetGamma};

  SkColorSpace::Gamut gamuts[] = {
      SkColorSpace::kSRGB_Gamut, SkColorSpace::kAdobeRGB_Gamut,
      SkColorSpace::kDCIP3_D65_Gamut, SkColorSpace::kRec2020_Gamut,
  };

  skia_color_spaces.push_back((SkColorSpace::MakeSRGB())->makeColorSpin());

  for (unsigned gamma_itr = 0; gamma_itr < 2; gamma_itr++) {
    for (unsigned gamut_itr = 0; gamut_itr < 4; gamut_itr++) {
      skia_color_spaces.push_back(
          SkColorSpace::MakeRGB(gammas[gamma_itr], gamuts[gamut_itr]));
    }
  }

  std::vector<gfx::ColorSpace> gfx_color_spaces;
  for (unsigned i = 0; i < skia_color_spaces.size(); i++) {
    gfx::ColorSpace color_space =
        SkColorSpaceToGfxColorSpace(skia_color_spaces[i]);
    ASSERT_TRUE(SkColorSpace::Equals(color_space.ToSkColorSpace().get(),
                                     skia_color_spaces[i].get()));
  }
}

}  // namespace blink
