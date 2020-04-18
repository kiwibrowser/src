// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/opentype/font_settings.h"

#include "base/memory/scoped_refptr.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

namespace {

template <typename T, typename U>
scoped_refptr<T> MakeSettings(std::initializer_list<U> items) {
  scoped_refptr<T> settings = T::Create();
  for (auto item = items.begin(); item != items.end(); ++item) {
    settings->Append(*item);
  }
  return settings;
}

}  // namespace

TEST(FontSettingsTest, HashTest) {
  scoped_refptr<FontVariationSettings> one_axis_a =
      MakeSettings<FontVariationSettings, FontVariationAxis>(
          {FontVariationAxis{"a   ", 0}});
  scoped_refptr<FontVariationSettings> one_axis_b =
      MakeSettings<FontVariationSettings, FontVariationAxis>(
          {FontVariationAxis{"b   ", 0}});
  scoped_refptr<FontVariationSettings> two_axes =
      MakeSettings<FontVariationSettings, FontVariationAxis>(
          {FontVariationAxis{"a   ", 0}, FontVariationAxis{"b   ", 0}});
  scoped_refptr<FontVariationSettings> two_axes_different_value =
      MakeSettings<FontVariationSettings, FontVariationAxis>(
          {FontVariationAxis{"a   ", 0}, FontVariationAxis{"b   ", 1}});

  scoped_refptr<FontVariationSettings> empty_variation_settings =
      FontVariationSettings::Create();

  CHECK_NE(one_axis_a->GetHash(), one_axis_b->GetHash());
  CHECK_NE(one_axis_a->GetHash(), two_axes->GetHash());
  CHECK_NE(one_axis_a->GetHash(), two_axes_different_value->GetHash());
  CHECK_NE(empty_variation_settings->GetHash(), one_axis_a->GetHash());
  CHECK_EQ(empty_variation_settings->GetHash(), 0u);
};

TEST(FontSettingsTest, ToString) {
  {
    scoped_refptr<FontVariationSettings> settings =
        MakeSettings<FontVariationSettings, FontVariationAxis>(
            {FontVariationAxis{"a", 42}, FontVariationAxis{"b", 8118}});
    EXPECT_EQ("a=42,b=8118", settings->ToString());
  }
  {
    scoped_refptr<FontFeatureSettings> settings =
        MakeSettings<FontFeatureSettings, FontFeature>(
            {FontFeature{"a", 42}, FontFeature{"b", 8118}});
    EXPECT_EQ("a=42,b=8118", settings->ToString());
  }
}

}  // namespace blink
