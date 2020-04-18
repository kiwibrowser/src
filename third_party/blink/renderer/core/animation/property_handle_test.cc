// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/property_handle.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/svg_names.h"

namespace blink {

using SVGNames::amplitudeAttr;
using SVGNames::exponentAttr;

class PropertyHandleTest : public testing::Test {};

TEST_F(PropertyHandleTest, Equality) {
  AtomicString name_a = "--a";
  AtomicString name_b = "--b";

  EXPECT_TRUE(PropertyHandle(GetCSSPropertyOpacity()) ==
              PropertyHandle(GetCSSPropertyOpacity()));
  EXPECT_FALSE(PropertyHandle(GetCSSPropertyOpacity()) !=
               PropertyHandle(GetCSSPropertyOpacity()));
  EXPECT_FALSE(PropertyHandle(GetCSSPropertyOpacity()) ==
               PropertyHandle(GetCSSPropertyTransform()));
  EXPECT_TRUE(PropertyHandle(GetCSSPropertyOpacity()) !=
              PropertyHandle(GetCSSPropertyTransform()));
  EXPECT_FALSE(PropertyHandle(GetCSSPropertyOpacity()) ==
               PropertyHandle(name_a));
  EXPECT_TRUE(PropertyHandle(GetCSSPropertyOpacity()) !=
              PropertyHandle(name_a));
  EXPECT_FALSE(PropertyHandle(GetCSSPropertyOpacity()) ==
               PropertyHandle(amplitudeAttr));
  EXPECT_TRUE(PropertyHandle(GetCSSPropertyOpacity()) !=
              PropertyHandle(amplitudeAttr));

  EXPECT_FALSE(PropertyHandle(name_a) ==
               PropertyHandle(GetCSSPropertyOpacity()));
  EXPECT_TRUE(PropertyHandle(name_a) !=
              PropertyHandle(GetCSSPropertyOpacity()));
  EXPECT_FALSE(PropertyHandle(name_a) ==
               PropertyHandle(GetCSSPropertyTransform()));
  EXPECT_TRUE(PropertyHandle(name_a) !=
              PropertyHandle(GetCSSPropertyTransform()));
  EXPECT_TRUE(PropertyHandle(name_a) == PropertyHandle(name_a));
  EXPECT_FALSE(PropertyHandle(name_a) != PropertyHandle(name_a));
  EXPECT_FALSE(PropertyHandle(name_a) == PropertyHandle(name_b));
  EXPECT_TRUE(PropertyHandle(name_a) != PropertyHandle(name_b));
  EXPECT_FALSE(PropertyHandle(name_a) == PropertyHandle(amplitudeAttr));
  EXPECT_TRUE(PropertyHandle(name_a) != PropertyHandle(amplitudeAttr));

  EXPECT_FALSE(PropertyHandle(amplitudeAttr) ==
               PropertyHandle(GetCSSPropertyOpacity()));
  EXPECT_TRUE(PropertyHandle(amplitudeAttr) !=
              PropertyHandle(GetCSSPropertyOpacity()));
  EXPECT_FALSE(PropertyHandle(amplitudeAttr) == PropertyHandle(name_a));
  EXPECT_TRUE(PropertyHandle(amplitudeAttr) != PropertyHandle(name_a));
  EXPECT_TRUE(PropertyHandle(amplitudeAttr) == PropertyHandle(amplitudeAttr));
  EXPECT_FALSE(PropertyHandle(amplitudeAttr) != PropertyHandle(amplitudeAttr));
  EXPECT_FALSE(PropertyHandle(amplitudeAttr) == PropertyHandle(exponentAttr));
  EXPECT_TRUE(PropertyHandle(amplitudeAttr) != PropertyHandle(exponentAttr));
}

TEST_F(PropertyHandleTest, Hash) {
  AtomicString name_a = "--a";
  AtomicString name_b = "--b";

  EXPECT_TRUE(PropertyHandle(GetCSSPropertyOpacity()).GetHash() ==
              PropertyHandle(GetCSSPropertyOpacity()).GetHash());
  EXPECT_FALSE(PropertyHandle(GetCSSPropertyOpacity()).GetHash() ==
               PropertyHandle(name_a).GetHash());
  EXPECT_FALSE(PropertyHandle(GetCSSPropertyOpacity()).GetHash() ==
               PropertyHandle(GetCSSPropertyTransform()).GetHash());
  EXPECT_FALSE(PropertyHandle(GetCSSPropertyOpacity()).GetHash() ==
               PropertyHandle(amplitudeAttr).GetHash());

  EXPECT_FALSE(PropertyHandle(name_a).GetHash() ==
               PropertyHandle(GetCSSPropertyOpacity()).GetHash());
  EXPECT_TRUE(PropertyHandle(name_a).GetHash() ==
              PropertyHandle(name_a).GetHash());
  EXPECT_FALSE(PropertyHandle(name_a).GetHash() ==
               PropertyHandle(name_b).GetHash());
  EXPECT_FALSE(PropertyHandle(name_a).GetHash() ==
               PropertyHandle(exponentAttr).GetHash());

  EXPECT_FALSE(PropertyHandle(amplitudeAttr).GetHash() ==
               PropertyHandle(GetCSSPropertyOpacity()).GetHash());
  EXPECT_FALSE(PropertyHandle(amplitudeAttr).GetHash() ==
               PropertyHandle(name_a).GetHash());
  EXPECT_TRUE(PropertyHandle(amplitudeAttr).GetHash() ==
              PropertyHandle(amplitudeAttr).GetHash());
  EXPECT_FALSE(PropertyHandle(amplitudeAttr).GetHash() ==
               PropertyHandle(exponentAttr).GetHash());
}

TEST_F(PropertyHandleTest, Accessors) {
  AtomicString name = "--x";

  EXPECT_TRUE(PropertyHandle(GetCSSPropertyOpacity()).IsCSSProperty());
  EXPECT_TRUE(PropertyHandle(name).IsCSSProperty());
  EXPECT_FALSE(PropertyHandle(amplitudeAttr).IsCSSProperty());

  EXPECT_FALSE(PropertyHandle(GetCSSPropertyOpacity()).IsSVGAttribute());
  EXPECT_FALSE(PropertyHandle(name).IsSVGAttribute());
  EXPECT_TRUE(PropertyHandle(amplitudeAttr).IsSVGAttribute());

  EXPECT_FALSE(PropertyHandle(GetCSSPropertyOpacity()).IsCSSCustomProperty());
  EXPECT_TRUE(PropertyHandle(name).IsCSSCustomProperty());
  EXPECT_FALSE(PropertyHandle(amplitudeAttr).IsCSSCustomProperty());

  EXPECT_EQ(
      PropertyHandle(GetCSSPropertyOpacity()).GetCSSProperty().PropertyID(),
      CSSPropertyOpacity);
  EXPECT_EQ(PropertyHandle(name).GetCSSProperty().PropertyID(),
            CSSPropertyVariable);
  EXPECT_EQ(PropertyHandle(name).CustomPropertyName(), name);
  EXPECT_EQ(PropertyHandle(amplitudeAttr).SvgAttribute(), amplitudeAttr);
}

}  // namespace blink
