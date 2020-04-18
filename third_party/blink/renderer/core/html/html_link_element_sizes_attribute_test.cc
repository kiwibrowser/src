// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/html_link_element.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_token_list.h"
#include "third_party/blink/renderer/core/html_names.h"

namespace blink {

class HTMLLinkElementSizesAttributeTest : public testing::Test {};

TEST(HTMLLinkElementSizesAttributeTest,
     setSizesPropertyValue_updatesAttribute) {
  Document* document = Document::CreateForTest();
  auto* link = HTMLLinkElement::Create(*document, CreateElementFlags());
  DOMTokenList* sizes = link->sizes();
  EXPECT_EQ(g_null_atom, sizes->value());
  sizes->setValue("   a b  c ");
  EXPECT_EQ("   a b  c ", link->getAttribute(HTMLNames::sizesAttr));
  EXPECT_EQ("   a b  c ", sizes->value());
}

TEST(HTMLLinkElementSizesAttributeTest,
     setSizesAttribute_updatesSizesPropertyValue) {
  Document* document = Document::CreateForTest();
  HTMLLinkElement* link =
      HTMLLinkElement::Create(*document, CreateElementFlags());
  DOMTokenList* sizes = link->sizes();
  EXPECT_EQ(g_null_atom, sizes->value());
  link->setAttribute(HTMLNames::sizesAttr, "y  x ");
  EXPECT_EQ("y  x ", sizes->value());
}

}  // namespace blink
