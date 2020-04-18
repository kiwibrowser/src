// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/public/cpp/property_type_converters.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/skia_util.h"

namespace mojo {
namespace {

// Tests round-trip serializing and deserializing an SkBitmap.
TEST(PropertyTypeConvertersTest, SkBitmapRoundTrip) {
  SkBitmap bitmap;
  bitmap.allocN32Pixels(16, 32);
  bitmap.eraseARGB(255, 11, 22, 33);
  EXPECT_FALSE(bitmap.isNull());
  auto bytes = TypeConverter<std::vector<uint8_t>, SkBitmap>::Convert(bitmap);
  SkBitmap out = TypeConverter<SkBitmap, std::vector<uint8_t>>::Convert(bytes);
  EXPECT_TRUE(gfx::BitmapsAreEqual(bitmap, out));
}

TEST(PropertyTypeConvertersTest, Rectangle) {
  const gfx::Rect rect(1, 2, 3, 4);
  const auto encoded = mojo::ConvertTo<std::vector<uint8_t>>(rect);
  const gfx::Rect decoded = mojo::ConvertTo<gfx::Rect>(encoded);
  EXPECT_EQ(rect, decoded);

  // Verify a vector with an invalid size results in an empty rect.
  EXPECT_EQ(gfx::Rect(), mojo::ConvertTo<gfx::Rect>(std::vector<uint8_t>()));
}

TEST(PropertyTypeConvertersTest, Size) {
  const gfx::Size size(121, 987);
  const auto encoded = mojo::ConvertTo<std::vector<uint8_t>>(size);
  const gfx::Size decoded = mojo::ConvertTo<gfx::Size>(encoded);
  EXPECT_EQ(size, decoded);

  // Verify a vector with an invalid size results in an empty size.
  EXPECT_EQ(gfx::Size(), mojo::ConvertTo<gfx::Size>(std::vector<uint8_t>()));
}

TEST(PropertyTypeConvertersTest, Int32) {
  const int32_t value = 0xFEDCBA90;
  const auto encoded = mojo::ConvertTo<std::vector<uint8_t>>(value);
  const int32_t decoded = mojo::ConvertTo<int32_t>(encoded);
  EXPECT_EQ(value, decoded);

  // Verify a vector with an invalid size results in 0.
  EXPECT_EQ(0, mojo::ConvertTo<int32_t>(std::vector<uint8_t>()));
  EXPECT_EQ(0, mojo::ConvertTo<int32_t>(std::vector<uint8_t>(1, 1)));
  EXPECT_EQ(0, mojo::ConvertTo<int32_t>(std::vector<uint8_t>(2, 1)));
  EXPECT_EQ(0, mojo::ConvertTo<int32_t>(std::vector<uint8_t>(5, 1)));
}

TEST(PropertyTypeConvertersTest, Int64) {
  const int64_t value = 0xFEDCBA0123456789;
  const auto encoded = mojo::ConvertTo<std::vector<uint8_t>>(value);
  const int64_t decoded = mojo::ConvertTo<int64_t>(encoded);
  EXPECT_EQ(value, decoded);

  // Verify a vector with an invalid size results in 0.
  EXPECT_EQ(0, mojo::ConvertTo<int64_t>(std::vector<uint8_t>()));
  EXPECT_EQ(0, mojo::ConvertTo<int64_t>(std::vector<uint8_t>(1, 1)));
  EXPECT_EQ(0, mojo::ConvertTo<int64_t>(std::vector<uint8_t>(2, 1)));
  EXPECT_EQ(0, mojo::ConvertTo<int64_t>(std::vector<uint8_t>(10, 1)));
}

}  // namespace
}  // namespace mojo
