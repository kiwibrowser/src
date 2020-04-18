// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/inline/ng_physical_line_box_fragment.h"

#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_fragment_traversal.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_physical_text_fragment.h"
#include "third_party/blink/renderer/core/layout/ng/ng_layout_test.h"
#include "third_party/blink/renderer/core/layout/ng/ng_physical_box_fragment.h"

namespace blink {

class NGPhysicalLineBoxFragmentTest : public NGLayoutTest {
 public:
  NGPhysicalLineBoxFragmentTest() : NGLayoutTest() {}

 protected:
  const NGPhysicalLineBoxFragment* GetLineBox() const {
    const Element* container = GetElementById("root");
    DCHECK(container);
    const LayoutObject* layout_object = container->GetLayoutObject();
    DCHECK(layout_object) << container;
    DCHECK(layout_object->IsLayoutBlockFlow()) << container;
    const NGPhysicalBoxFragment* root_fragment =
        ToLayoutBlockFlow(layout_object)->CurrentFragment();
    DCHECK(root_fragment) << container;

    for (const auto& child :
         NGInlineFragmentTraversal::DescendantsOf(*root_fragment)) {
      if (child.fragment->IsLineBox())
        return ToNGPhysicalLineBoxFragment(child.fragment.get());
    }
    NOTREACHED();
    return nullptr;
  }
};

#define EXPECT_TEXT_FRAGMENT(text, fragment)                                \
  {                                                                         \
    EXPECT_TRUE(fragment);                                                  \
    EXPECT_TRUE(fragment->IsText());                                        \
    EXPECT_EQ(text, ToNGPhysicalTextFragment(fragment)->Text().ToString()); \
  }

#define EXPECT_BOX_FRAGMENT(id, fragment)               \
  {                                                     \
    EXPECT_TRUE(fragment);                              \
    EXPECT_TRUE(fragment->IsBox());                     \
    EXPECT_TRUE(fragment->GetNode());                   \
    EXPECT_EQ(GetElementById(id), fragment->GetNode()); \
  }

TEST_F(NGPhysicalLineBoxFragmentTest, FirstLastLogicalLeafInSimpleText) {
  SetBodyInnerHTML(
      "<div id=root>"
      "<span>foo</span>"
      "<span>bar</span>"
      "</div>");
  EXPECT_TEXT_FRAGMENT("foo", GetLineBox()->FirstLogicalLeaf());
  EXPECT_TEXT_FRAGMENT("bar", GetLineBox()->LastLogicalLeaf());
  EXPECT_TEXT_FRAGMENT("bar", GetLineBox()->LastLogicalLeafIgnoringLineBreak());
}

TEST_F(NGPhysicalLineBoxFragmentTest, FirstLastLogicalLeafInRtlText) {
  SetBodyInnerHTML(
      "<bdo id=root dir=rtl style='display: block'>"
      "<span>foo</span>"
      "<span>bar</span>"
      "</bdo>");
  EXPECT_TEXT_FRAGMENT("foo", GetLineBox()->FirstLogicalLeaf());
  EXPECT_TEXT_FRAGMENT("bar", GetLineBox()->LastLogicalLeaf());
  EXPECT_TEXT_FRAGMENT("bar", GetLineBox()->LastLogicalLeafIgnoringLineBreak());
}

TEST_F(NGPhysicalLineBoxFragmentTest,
       FirstLastLogicalLeafInTextAsDeepDescendants) {
  SetBodyInnerHTML(
      "<style>span {border: 1px solid black}</style>"
      "<div id=root>"
      "<span><span>f</span>oo</span>"
      "<span>ba<span>r</span></span>"
      "</div>");
  EXPECT_TEXT_FRAGMENT("f", GetLineBox()->FirstLogicalLeaf());
  EXPECT_TEXT_FRAGMENT("r", GetLineBox()->LastLogicalLeaf());
  EXPECT_TEXT_FRAGMENT("r", GetLineBox()->LastLogicalLeafIgnoringLineBreak());
}

TEST_F(NGPhysicalLineBoxFragmentTest, FirstLastLogicalLeafWithInlineBlock) {
  SetBodyInnerHTML(
      "<div id=root>"
      "<span id=foo style='display: inline-block'>foo</span>"
      "bar"
      "<span id=baz style='display: inline-block'>baz</span>"
      "</div>");
  EXPECT_BOX_FRAGMENT("foo", GetLineBox()->FirstLogicalLeaf());
  EXPECT_BOX_FRAGMENT("baz", GetLineBox()->LastLogicalLeaf());
  EXPECT_BOX_FRAGMENT("baz", GetLineBox()->LastLogicalLeafIgnoringLineBreak());
}

TEST_F(NGPhysicalLineBoxFragmentTest, FirstLastLogicalLeafWithImages) {
  SetBodyInnerHTML("<div id=root><img id=img1>foo<img id=img2></div>");
  EXPECT_BOX_FRAGMENT("img1", GetLineBox()->FirstLogicalLeaf());
  EXPECT_BOX_FRAGMENT("img2", GetLineBox()->LastLogicalLeaf());
  EXPECT_BOX_FRAGMENT("img2", GetLineBox()->LastLogicalLeafIgnoringLineBreak());
}

TEST_F(NGPhysicalLineBoxFragmentTest, LastLogicalLeafSoftWrap) {
  SetBodyInnerHTML("<div id=root style='width: 2em'>foo bar</div>");
  EXPECT_TEXT_FRAGMENT("foo", GetLineBox()->LastLogicalLeaf());
  EXPECT_TEXT_FRAGMENT("foo", GetLineBox()->LastLogicalLeafIgnoringLineBreak());
}

TEST_F(NGPhysicalLineBoxFragmentTest, LastLogicalLeafHardWrap) {
  SetBodyInnerHTML("<div id=root>foo<br>bar</div>");
  EXPECT_TEXT_FRAGMENT("\n", GetLineBox()->LastLogicalLeaf());
  EXPECT_TEXT_FRAGMENT("foo", GetLineBox()->LastLogicalLeafIgnoringLineBreak());
}

}  // namespace blink
