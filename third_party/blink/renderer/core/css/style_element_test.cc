// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/style_element.h"

#include <memory>
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/css/style_sheet_contents.h"
#include "third_party/blink/renderer/core/dom/comment.h"
#include "third_party/blink/renderer/core/html/html_style_element.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"

namespace blink {

TEST(StyleElementTest, CreateSheetUsesCache) {
  std::unique_ptr<DummyPageHolder> dummy_page_holder =
      DummyPageHolder::Create(IntSize(800, 600));
  Document& document = dummy_page_holder->GetDocument();

  document.documentElement()->SetInnerHTMLFromString(
      "<style id=style>a { top: 0; }</style>");

  HTMLStyleElement& style_element =
      ToHTMLStyleElement(*document.getElementById("style"));
  StyleSheetContents* sheet = style_element.sheet()->Contents();

  Comment* comment = document.createComment("hello!");
  style_element.AppendChild(comment);
  EXPECT_EQ(style_element.sheet()->Contents(), sheet);

  style_element.RemoveChild(comment);
  EXPECT_EQ(style_element.sheet()->Contents(), sheet);
}

}  // namespace blink
