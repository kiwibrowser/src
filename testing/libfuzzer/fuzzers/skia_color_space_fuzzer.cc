// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstddef>
#include <cstdint>
#include <random>

#include "base/logging.h"
#include "testing/libfuzzer/fuzzers/color_space_data.h"
#include "third_party/skia/include/core/SkColorSpace.h"
#include "third_party/skia/include/core/SkColorSpaceXform.h"

static constexpr size_t kPixels = 2048 / 4;

static uint32_t pixels[kPixels * 4];

static void GeneratePixels(size_t hash) {
  static std::uniform_int_distribution<uint32_t> uniform(0u, ~0u);

  std::mt19937_64 random(hash);
  for (size_t i = 0; i < arraysize(pixels); ++i)
    pixels[i] = uniform(random);
}

static sk_sp<SkColorSpace> test;
static sk_sp<SkColorSpace> srgb;

static void ColorTransform(size_t hash, bool input) {
  if (!test.get())
    return;

  auto transform = input ? SkColorSpaceXform::New(test.get(), srgb.get())
                         : SkColorSpaceXform::New(srgb.get(), test.get());
  if (!transform)
    return;

  static uint32_t output[kPixels * 4];

  const auto form1 = SkColorSpaceXform::ColorFormat((hash >> 0) % 7);
  const auto form2 = SkColorSpaceXform::ColorFormat((hash >> 3) % 7);
  const auto alpha = SkAlphaType(hash >> 6 & 3);

  transform->apply(form1, output, form2, pixels, kPixels, alpha);
}

static sk_sp<SkColorSpace> SelectProfile(size_t hash) {
  static sk_sp<SkColorSpace> profiles[8] = {
      SkColorSpace::MakeSRGB(),
      SkColorSpace::MakeICC(kSRGBData, arraysize(kSRGBData)),
      SkColorSpace::MakeICC(kSRGBPara, arraysize(kSRGBPara)),
      SkColorSpace::MakeICC(kAdobeData, arraysize(kAdobeData)),
      SkColorSpace::MakeSRGBLinear(),
      SkColorSpace::MakeRGB(
          SkColorSpace::RenderTargetGamma::kLinear_RenderTargetGamma,
          SkColorSpace::Gamut::kAdobeRGB_Gamut),
      SkColorSpace::MakeRGB(
          SkColorSpace::RenderTargetGamma::kSRGB_RenderTargetGamma,
          SkColorSpace::Gamut::kDCIP3_D65_Gamut),
      SkColorSpace::MakeSRGB(),
  };

  return profiles[hash & 7];
}

inline size_t Hash(const char* data, size_t size, size_t hash = ~0) {
  for (size_t i = 0; i < size; ++i)
    hash = hash * 131 + *data++;
  return hash;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  constexpr size_t kSizeLimit = 4 * 1024 * 1024;
  if (size < 128 || size > kSizeLimit)
    return 0;

  test = SkColorSpace::MakeICC(data, size);
  if (!test.get())
    return 0;

  const size_t hash = Hash(reinterpret_cast<const char*>(data), size);
  srgb = SelectProfile(hash);
  GeneratePixels(hash);

  ColorTransform(hash, true);
  ColorTransform(hash, false);

  test.reset();
  srgb.reset();
  return 0;
}
