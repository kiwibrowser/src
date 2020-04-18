// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mus/property_converter.h"

#include <stdint.h>
#include <string>

#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "services/ui/public/cpp/property_type_converters.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/test/aura_test_base.h"
#include "ui/aura/window.h"
#include "ui/base/class_property.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/skia_util.h"

// See aura_constants.cc for bool, int32_t, int64_t, std::string, gfx::Rect,
// base::string16, uint32_t (via SkColor), and gfx::ImageSkia.
DEFINE_UI_CLASS_PROPERTY_TYPE(uint8_t)
DEFINE_UI_CLASS_PROPERTY_TYPE(uint16_t)
DEFINE_UI_CLASS_PROPERTY_TYPE(uint64_t)
DEFINE_UI_CLASS_PROPERTY_TYPE(int8_t)
DEFINE_UI_CLASS_PROPERTY_TYPE(int16_t)

namespace aura {

namespace {

DEFINE_UI_CLASS_PROPERTY_KEY(bool, kTestPropertyKey0, false);
DEFINE_UI_CLASS_PROPERTY_KEY(bool, kTestPropertyKey1, true);
DEFINE_UI_CLASS_PROPERTY_KEY(uint8_t, kTestPropertyKey2, UINT8_MAX / 3);
DEFINE_UI_CLASS_PROPERTY_KEY(uint16_t, kTestPropertyKey3, UINT16_MAX / 3);
DEFINE_UI_CLASS_PROPERTY_KEY(uint32_t, kTestPropertyKey4, UINT32_MAX);
DEFINE_UI_CLASS_PROPERTY_KEY(uint64_t, kTestPropertyKey5, UINT64_MAX);
DEFINE_UI_CLASS_PROPERTY_KEY(int8_t, kTestPropertyKey6, 0);
DEFINE_UI_CLASS_PROPERTY_KEY(int16_t, kTestPropertyKey7, 1);
DEFINE_UI_CLASS_PROPERTY_KEY(int32_t, kTestPropertyKey8, -1);
DEFINE_UI_CLASS_PROPERTY_KEY(int64_t, kTestPropertyKey9, 777);

DEFINE_OWNED_UI_CLASS_PROPERTY_KEY(gfx::ImageSkia,
                                   kTestImageSkiaPropertyKey,
                                   nullptr);
DEFINE_OWNED_UI_CLASS_PROPERTY_KEY(gfx::Rect, kTestRectPropertyKey, nullptr);
DEFINE_OWNED_UI_CLASS_PROPERTY_KEY(gfx::Size, kTestSizePropertyKey, nullptr);
DEFINE_OWNED_UI_CLASS_PROPERTY_KEY(
     std::string, kTestStringPropertyKey, nullptr);
DEFINE_OWNED_UI_CLASS_PROPERTY_KEY(base::string16, kTestString16PropertyKey,
                                 nullptr);

const char kTestPropertyServerKey0[] = "test-property-server0";
const char kTestPropertyServerKey1[] = "test-property-server1";
const char kTestPropertyServerKey2[] = "test-property-server2";
const char kTestPropertyServerKey3[] = "test-property-server3";
const char kTestPropertyServerKey4[] = "test-property-server4";
const char kTestPropertyServerKey5[] = "test-property-server5";
const char kTestPropertyServerKey6[] = "test-property-server6";
const char kTestPropertyServerKey7[] = "test-property-server7";
const char kTestPropertyServerKey8[] = "test-property-server8";
const char kTestPropertyServerKey9[] = "test-property-server9";

const char kTestImageSkiaPropertyServerKey[] = "test-imageskia-property-server";
const char kTestRectPropertyServerKey[] = "test-rect-property-server";
const char kTestSizePropertyServerKey[] = "test-size-property-server";
const char kTestStringPropertyServerKey[] = "test-string-property-server";
const char kTestString16PropertyServerKey[] = "test-string16-property-server";

// Test registration, naming and value conversion for primitive property types.
template <typename T>
void TestPrimitiveProperty(PropertyConverter* property_converter,
                           Window* window,
                           const WindowProperty<T>* key,
                           const char* transport_name,
                           T value_1,
                           T value_2) {
  property_converter->RegisterPrimitiveProperty(
      key, transport_name, PropertyConverter::CreateAcceptAnyValueCallback());
  EXPECT_EQ(transport_name,
            property_converter->GetTransportNameForPropertyKey(key));
  EXPECT_TRUE(property_converter->IsTransportNameRegistered(transport_name));

  window->SetProperty(key, value_1);
  EXPECT_EQ(value_1, window->GetProperty(key));

  std::string transport_name_out;
  std::unique_ptr<std::vector<uint8_t>> transport_value_out;
  EXPECT_TRUE(property_converter->ConvertPropertyForTransport(
      window, key, &transport_name_out, &transport_value_out));
  EXPECT_EQ(transport_name, transport_name_out);
  const int64_t storage_value_1 = static_cast<int64_t>(value_1);
  std::vector<uint8_t> transport_value1 =
      mojo::ConvertTo<std::vector<uint8_t>>(storage_value_1);
  EXPECT_EQ(transport_value1, *transport_value_out.get());

  int64_t decoded_value_1 = 0;
  EXPECT_TRUE(property_converter->GetPropertyValueFromTransportValue(
      transport_name, *transport_value_out, &decoded_value_1));
  EXPECT_EQ(value_1, static_cast<T>(decoded_value_1));

  const int64_t storage_value_2 = static_cast<int64_t>(value_2);
  std::vector<uint8_t> transport_value2 =
      mojo::ConvertTo<std::vector<uint8_t>>(storage_value_2);
  property_converter->SetPropertyFromTransportValue(window, transport_name,
                                                    &transport_value2);
  EXPECT_EQ(value_2, window->GetProperty(key));
  int64_t decoded_value_2 = 0;
  EXPECT_TRUE(property_converter->GetPropertyValueFromTransportValue(
      transport_name, transport_value2, &decoded_value_2));
  EXPECT_EQ(value_2, static_cast<T>(decoded_value_2));
}

bool OnlyAllowNegativeNumbers(int64_t number) {
  return number < 0;
}

}  // namespace

using PropertyConverterTest = test::AuraTestBase;

// Verifies property setting behavior for a std::string* property.
TEST_F(PropertyConverterTest, PrimitiveProperties) {
  PropertyConverter property_converter;
  std::unique_ptr<Window> window(CreateNormalWindow(1, root_window(), nullptr));

  EXPECT_FALSE(
      property_converter.IsTransportNameRegistered(kTestPropertyServerKey0));

  const bool value_0a = true, value_0b = false;
  TestPrimitiveProperty(&property_converter, window.get(), kTestPropertyKey0,
                        kTestPropertyServerKey0, value_0a, value_0b);

  const bool value_1a = true, value_1b = false;
  TestPrimitiveProperty(&property_converter, window.get(), kTestPropertyKey1,
                        kTestPropertyServerKey1, value_1a, value_1b);

  const uint8_t value_2a = UINT8_MAX / 2, value_2b = UINT8_MAX / 3;
  TestPrimitiveProperty(&property_converter, window.get(), kTestPropertyKey2,
                        kTestPropertyServerKey2, value_2a, value_2b);

  const uint16_t value_3a = UINT16_MAX / 3, value_3b = UINT16_MAX / 4;
  TestPrimitiveProperty(&property_converter, window.get(), kTestPropertyKey3,
                        kTestPropertyServerKey3, value_3a, value_3b);

  const uint32_t value_4a = UINT32_MAX / 4, value_4b = UINT32_MAX / 5;
  TestPrimitiveProperty(&property_converter, window.get(), kTestPropertyKey4,
                        kTestPropertyServerKey4, value_4a, value_4b);

  const uint64_t value_5a = UINT64_MAX / 5, value_5b = UINT64_MAX / 6;
  TestPrimitiveProperty(&property_converter, window.get(), kTestPropertyKey5,
                        kTestPropertyServerKey5, value_5a, value_5b);

  const int8_t value_6a = INT8_MIN / 2, value_6b = INT8_MIN / 3;
  TestPrimitiveProperty(&property_converter, window.get(), kTestPropertyKey6,
                        kTestPropertyServerKey6, value_6a, value_6b);

  const int16_t value_7a = INT16_MIN / 3, value_7b = INT16_MIN / 4;
  TestPrimitiveProperty(&property_converter, window.get(), kTestPropertyKey7,
                        kTestPropertyServerKey7, value_7a, value_7b);

  const int32_t value_8a = INT32_MIN / 4, value_8b = INT32_MIN / 5;
  TestPrimitiveProperty(&property_converter, window.get(), kTestPropertyKey8,
                        kTestPropertyServerKey8, value_8a, value_8b);

  const int64_t value_9a = INT64_MIN / 5, value_9b = INT64_MIN / 6;
  TestPrimitiveProperty(&property_converter, window.get(), kTestPropertyKey9,
                        kTestPropertyServerKey9, value_9a, value_9b);
}

TEST_F(PropertyConverterTest, TestPrimitiveVerifier) {
  std::unique_ptr<Window> window(CreateNormalWindow(1, root_window(), nullptr));

  PropertyConverter property_converter;
  property_converter.RegisterPrimitiveProperty(
      kTestPropertyKey8, kTestPropertyServerKey8,
      base::Bind(&OnlyAllowNegativeNumbers));

  // Test that we reject invalid TransportValues during
  // GetPropertyValueFromTransportValue().
  int64_t int_value = 5;
  std::vector<uint8_t> transport =
      mojo::ConvertTo<std::vector<uint8_t>>(int_value);
  EXPECT_FALSE(property_converter.GetPropertyValueFromTransportValue(
      kTestPropertyServerKey8, transport, &int_value));

  // Test that we reject invalid TransportValues during
  // SetPropertyFromTransportValue().
  EXPECT_EQ(-1, window->GetProperty(kTestPropertyKey8));
  property_converter.SetPropertyFromTransportValue(
      window.get(), kTestPropertyServerKey8, &transport);
  EXPECT_EQ(-1, window->GetProperty(kTestPropertyKey8));
}

// Verifies property setting behavior for a gfx::ImageSkia* property.
TEST_F(PropertyConverterTest, ImageSkiaProperty) {
  PropertyConverter property_converter;
  property_converter.RegisterImageSkiaProperty(kTestImageSkiaPropertyKey,
                                               kTestImageSkiaPropertyServerKey);
  EXPECT_EQ(kTestImageSkiaPropertyServerKey,
            property_converter.GetTransportNameForPropertyKey(
                kTestImageSkiaPropertyKey));
  EXPECT_TRUE(property_converter.IsTransportNameRegistered(
      kTestImageSkiaPropertyServerKey));

  SkBitmap bitmap_1;
  bitmap_1.allocN32Pixels(16, 32);
  bitmap_1.eraseARGB(255, 11, 22, 33);
  gfx::ImageSkia value_1 = gfx::ImageSkia::CreateFrom1xBitmap(bitmap_1);
  std::unique_ptr<Window> window(CreateNormalWindow(1, root_window(), nullptr));
  window->SetProperty(kTestImageSkiaPropertyKey, new gfx::ImageSkia(value_1));
  gfx::ImageSkia* image_out_1 = window->GetProperty(kTestImageSkiaPropertyKey);
  EXPECT_TRUE(gfx::BitmapsAreEqual(bitmap_1, *image_out_1->bitmap()));

  std::string transport_name_out;
  std::unique_ptr<std::vector<uint8_t>> transport_value_out;
  EXPECT_TRUE(property_converter.ConvertPropertyForTransport(
      window.get(), kTestImageSkiaPropertyKey, &transport_name_out,
      &transport_value_out));
  EXPECT_EQ(kTestImageSkiaPropertyServerKey, transport_name_out);
  EXPECT_EQ(mojo::ConvertTo<std::vector<uint8_t>>(bitmap_1),
            *transport_value_out.get());

  SkBitmap bitmap_2;
  bitmap_2.allocN32Pixels(16, 16);
  bitmap_2.eraseARGB(255, 33, 22, 11);
  EXPECT_FALSE(gfx::BitmapsAreEqual(bitmap_1, bitmap_2));
  gfx::ImageSkia value_2 = gfx::ImageSkia::CreateFrom1xBitmap(bitmap_2);
  std::vector<uint8_t> transport_value =
      mojo::ConvertTo<std::vector<uint8_t>>(bitmap_2);
  property_converter.SetPropertyFromTransportValue(
      window.get(), kTestImageSkiaPropertyServerKey, &transport_value);
  gfx::ImageSkia* image_out_2 = window->GetProperty(kTestImageSkiaPropertyKey);
  EXPECT_TRUE(gfx::BitmapsAreEqual(bitmap_2, *image_out_2->bitmap()));
}

// Verifies property setting behavior for a gfx::Rect* property.
TEST_F(PropertyConverterTest, RectProperty) {
  PropertyConverter property_converter;
  property_converter.RegisterRectProperty(kTestRectPropertyKey,
                                          kTestRectPropertyServerKey);
  EXPECT_EQ(
      kTestRectPropertyServerKey,
      property_converter.GetTransportNameForPropertyKey(kTestRectPropertyKey));
  EXPECT_TRUE(
      property_converter.IsTransportNameRegistered(kTestRectPropertyServerKey));

  gfx::Rect value_1(1, 2, 3, 4);
  std::unique_ptr<Window> window(CreateNormalWindow(1, root_window(), nullptr));
  window->SetProperty(kTestRectPropertyKey, new gfx::Rect(value_1));
  EXPECT_EQ(value_1, *window->GetProperty(kTestRectPropertyKey));

  std::string transport_name_out;
  std::unique_ptr<std::vector<uint8_t>> transport_value_out;
  EXPECT_TRUE(property_converter.ConvertPropertyForTransport(
      window.get(), kTestRectPropertyKey, &transport_name_out,
      &transport_value_out));
  EXPECT_EQ(kTestRectPropertyServerKey, transport_name_out);
  EXPECT_EQ(mojo::ConvertTo<std::vector<uint8_t>>(value_1),
            *transport_value_out.get());

  gfx::Rect value_2(1, 3, 5, 7);
  std::vector<uint8_t> transport_value =
      mojo::ConvertTo<std::vector<uint8_t>>(value_2);
  property_converter.SetPropertyFromTransportValue(
      window.get(), kTestRectPropertyServerKey, &transport_value);
  EXPECT_EQ(value_2, *window->GetProperty(kTestRectPropertyKey));
}

// Verifies property setting behavior for a gfx::Size* property.
TEST_F(PropertyConverterTest, SizeProperty) {
  PropertyConverter property_converter;
  property_converter.RegisterSizeProperty(kTestSizePropertyKey,
                                          kTestSizePropertyServerKey);
  EXPECT_EQ(
      kTestSizePropertyServerKey,
      property_converter.GetTransportNameForPropertyKey(kTestSizePropertyKey));
  EXPECT_TRUE(
      property_converter.IsTransportNameRegistered(kTestSizePropertyServerKey));

  gfx::Size value_1(1, 2);
  std::unique_ptr<Window> window(CreateNormalWindow(1, root_window(), nullptr));
  window->SetProperty(kTestSizePropertyKey, new gfx::Size(value_1));
  EXPECT_EQ(value_1, *window->GetProperty(kTestSizePropertyKey));

  std::string transport_name_out;
  std::unique_ptr<std::vector<uint8_t>> transport_value_out;
  EXPECT_TRUE(property_converter.ConvertPropertyForTransport(
      window.get(), kTestSizePropertyKey, &transport_name_out,
      &transport_value_out));
  EXPECT_EQ(kTestSizePropertyServerKey, transport_name_out);
  EXPECT_EQ(mojo::ConvertTo<std::vector<uint8_t>>(value_1),
            *transport_value_out.get());

  gfx::Size value_2(1, 3);
  std::vector<uint8_t> transport_value =
      mojo::ConvertTo<std::vector<uint8_t>>(value_2);
  property_converter.SetPropertyFromTransportValue(
      window.get(), kTestSizePropertyServerKey, &transport_value);
  EXPECT_EQ(value_2, *window->GetProperty(kTestSizePropertyKey));
}

// Verifies property setting behavior for a std::string* property.
TEST_F(PropertyConverterTest, StringProperty) {
  PropertyConverter property_converter;
  property_converter.RegisterStringProperty(kTestStringPropertyKey,
                                            kTestStringPropertyServerKey);
  EXPECT_EQ(kTestStringPropertyServerKey,
            property_converter.GetTransportNameForPropertyKey(
                kTestStringPropertyKey));
  EXPECT_TRUE(property_converter.IsTransportNameRegistered(
      kTestStringPropertyServerKey));

  std::string value_1 = "test value";
  std::unique_ptr<Window> window(CreateNormalWindow(1, root_window(), nullptr));
  window->SetProperty(kTestStringPropertyKey, new std::string(value_1));
  EXPECT_EQ(value_1, *window->GetProperty(kTestStringPropertyKey));

  std::string transport_name_out;
  std::unique_ptr<std::vector<uint8_t>> transport_value_out;
  EXPECT_TRUE(property_converter.ConvertPropertyForTransport(
      window.get(), kTestStringPropertyKey, &transport_name_out,
      &transport_value_out));
  EXPECT_EQ(kTestStringPropertyServerKey, transport_name_out);
  EXPECT_EQ(mojo::ConvertTo<std::vector<uint8_t>>(value_1),
            *transport_value_out.get());

  std::string value_2 = "another test value";
  std::vector<uint8_t> transport_value =
      mojo::ConvertTo<std::vector<uint8_t>>(value_2);
  property_converter.SetPropertyFromTransportValue(
      window.get(), kTestStringPropertyServerKey, &transport_value);
  EXPECT_EQ(value_2, *window->GetProperty(kTestStringPropertyKey));
}

// Verifies property setting behavior for a base::string16* property.
TEST_F(PropertyConverterTest, String16Property) {
  PropertyConverter property_converter;
  property_converter.RegisterString16Property(kTestString16PropertyKey,
                                              kTestString16PropertyServerKey);
  EXPECT_EQ(kTestString16PropertyServerKey,
            property_converter.GetTransportNameForPropertyKey(
                kTestString16PropertyKey));
  EXPECT_TRUE(property_converter.IsTransportNameRegistered(
      kTestString16PropertyServerKey));

  base::string16 value_1 = base::ASCIIToUTF16("test value");
  std::unique_ptr<Window> window(CreateNormalWindow(1, root_window(), nullptr));
  window->SetProperty(kTestString16PropertyKey, new base::string16(value_1));
  EXPECT_EQ(value_1, *window->GetProperty(kTestString16PropertyKey));

  std::string transport_name_out;
  std::unique_ptr<std::vector<uint8_t>> transport_value_out;
  EXPECT_TRUE(property_converter.ConvertPropertyForTransport(
      window.get(), kTestString16PropertyKey, &transport_name_out,
      &transport_value_out));
  EXPECT_EQ(kTestString16PropertyServerKey, transport_name_out);
  EXPECT_EQ(mojo::ConvertTo<std::vector<uint8_t>>(value_1),
            *transport_value_out.get());

  base::string16 value_2 = base::ASCIIToUTF16("another test value");
  std::vector<uint8_t> transport_value =
      mojo::ConvertTo<std::vector<uint8_t>>(value_2);
  property_converter.SetPropertyFromTransportValue(
      window.get(), kTestString16PropertyServerKey, &transport_value);
  EXPECT_EQ(value_2, *window->GetProperty(kTestString16PropertyKey));
}

}  // namespace aura
