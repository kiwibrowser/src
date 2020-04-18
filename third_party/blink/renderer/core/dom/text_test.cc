// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/text.h"

#include "third_party/blink/renderer/core/dom/range.h"
#include "third_party/blink/renderer/core/editing/testing/editing_test_base.h"
#include "third_party/blink/renderer/core/html/html_pre_element.h"
#include "third_party/blink/renderer/core/layout/layout_text.h"

namespace blink {

class TextTest : public EditingTestBase {};

TEST_F(TextTest, SetDataToChangeFirstLetterTextNode) {
  SetBodyContent(
      "<style>pre::first-letter {color:red;}</style><pre "
      "id=sample>a<span>b</span></pre>");

  Node* sample = GetDocument().getElementById("sample");
  Text* text = ToText(sample->firstChild());
  text->setData(" ");
  UpdateAllLifecyclePhases();

  EXPECT_FALSE(text->GetLayoutObject()->IsTextFragment());
}

TEST_F(TextTest, RemoveFirstLetterPseudoElementWhenNoLetter) {
  SetBodyContent("<style>*::first-letter{font:icon;}</style><pre>AB\n</pre>");

  Element* pre = GetDocument().QuerySelector("pre");
  Text* text = ToText(pre->firstChild());

  Range* range = Range::Create(GetDocument(), text, 0, text, 2);
  range->deleteContents(ASSERT_NO_EXCEPTION);
  UpdateAllLifecyclePhases();

  EXPECT_FALSE(text->GetLayoutObject()->IsTextFragment());
}

TEST_F(TextTest, TextLayoutObjectIsNeeded_CannotHaveChildren) {
  SetBodyContent("<img id=image>");
  UpdateAllLifecyclePhases();

  Element* img = GetDocument().getElementById("image");
  ASSERT_TRUE(img);

  const LayoutObject* img_layout = img->GetLayoutObject();
  ASSERT_TRUE(img_layout);
  const ComputedStyle& style = img_layout->StyleRef();

  Text* text = Text::Create(GetDocument(), "dummy");

  Node::AttachContext context;
  EXPECT_FALSE(text->TextLayoutObjectIsNeeded(context, style, *img_layout));

  context.use_previous_in_flow = true;
  EXPECT_FALSE(text->TextLayoutObjectIsNeeded(context, style, *img_layout));
}

TEST_F(TextTest, TextLayoutObjectIsNeeded_EditingText) {
  SetBodyContent("<span id=parent></span>");
  UpdateAllLifecyclePhases();

  Element* parent = GetDocument().getElementById("parent");
  ASSERT_TRUE(parent);

  const LayoutObject* parent_layout = parent->GetLayoutObject();
  ASSERT_TRUE(parent_layout);
  const ComputedStyle& style = parent_layout->StyleRef();

  Text* text_empty = Text::CreateEditingText(GetDocument(), "");
  Text* text_whitespace = Text::CreateEditingText(GetDocument(), " ");
  Text* text = Text::CreateEditingText(GetDocument(), "dummy");

  Node::AttachContext context;
  EXPECT_TRUE(
      text_empty->TextLayoutObjectIsNeeded(context, style, *parent_layout));
  EXPECT_TRUE(text_whitespace->TextLayoutObjectIsNeeded(context, style,
                                                        *parent_layout));
  EXPECT_TRUE(text->TextLayoutObjectIsNeeded(context, style, *parent_layout));

  context.use_previous_in_flow = true;
  EXPECT_TRUE(
      text_empty->TextLayoutObjectIsNeeded(context, style, *parent_layout));
  EXPECT_TRUE(text_whitespace->TextLayoutObjectIsNeeded(context, style,
                                                        *parent_layout));
  EXPECT_TRUE(text->TextLayoutObjectIsNeeded(context, style, *parent_layout));
}

TEST_F(TextTest, TextLayoutObjectIsNeeded_Empty) {
  SetBodyContent("<span id=parent></span>");
  UpdateAllLifecyclePhases();

  Element* parent = GetDocument().getElementById("parent");
  ASSERT_TRUE(parent);

  const LayoutObject* parent_layout = parent->GetLayoutObject();
  ASSERT_TRUE(parent_layout);
  const ComputedStyle& style = parent_layout->StyleRef();

  Text* text = Text::Create(GetDocument(), "");

  Node::AttachContext context;
  EXPECT_FALSE(text->TextLayoutObjectIsNeeded(context, style, *parent_layout));
  context.use_previous_in_flow = true;
  EXPECT_FALSE(text->TextLayoutObjectIsNeeded(context, style, *parent_layout));
}

TEST_F(TextTest, TextLayoutObjectIsNeeded_Whitespace) {
  SetBodyContent(
      "<div id=block></div>Ends with whitespace "
      "<span id=inline></span>Nospace<br id=br>");
  UpdateAllLifecyclePhases();

  LayoutObject* block =
      GetDocument().getElementById("block")->GetLayoutObject();
  LayoutObject* in_line =
      GetDocument().getElementById("inline")->GetLayoutObject();
  LayoutObject* space_at_end =
      GetDocument().getElementById("block")->nextSibling()->GetLayoutObject();
  LayoutObject* no_space =
      GetDocument().getElementById("inline")->nextSibling()->GetLayoutObject();
  LayoutObject* br = GetDocument().getElementById("br")->GetLayoutObject();
  ASSERT_TRUE(block);
  ASSERT_TRUE(in_line);
  ASSERT_TRUE(space_at_end);
  ASSERT_TRUE(no_space);
  ASSERT_TRUE(br);

  Text* whitespace = Text::Create(GetDocument(), "   ");
  Node::AttachContext context;

  EXPECT_FALSE(
      whitespace->TextLayoutObjectIsNeeded(context, block->StyleRef(), *block));
  EXPECT_FALSE(whitespace->TextLayoutObjectIsNeeded(
      context, in_line->StyleRef(), *in_line));

  context.use_previous_in_flow = true;
  EXPECT_FALSE(
      whitespace->TextLayoutObjectIsNeeded(context, block->StyleRef(), *block));
  EXPECT_TRUE(whitespace->TextLayoutObjectIsNeeded(context, in_line->StyleRef(),
                                                   *in_line));

  context.previous_in_flow = in_line;
  EXPECT_TRUE(
      whitespace->TextLayoutObjectIsNeeded(context, block->StyleRef(), *block));
  EXPECT_TRUE(whitespace->TextLayoutObjectIsNeeded(context, in_line->StyleRef(),
                                                   *in_line));

  context.previous_in_flow = space_at_end;
  EXPECT_FALSE(
      whitespace->TextLayoutObjectIsNeeded(context, block->StyleRef(), *block));
  EXPECT_FALSE(whitespace->TextLayoutObjectIsNeeded(
      context, in_line->StyleRef(), *in_line));

  context.previous_in_flow = no_space;
  EXPECT_TRUE(
      whitespace->TextLayoutObjectIsNeeded(context, block->StyleRef(), *block));
  EXPECT_TRUE(whitespace->TextLayoutObjectIsNeeded(context, in_line->StyleRef(),
                                                   *in_line));

  context.previous_in_flow = block;
  EXPECT_FALSE(
      whitespace->TextLayoutObjectIsNeeded(context, block->StyleRef(), *block));
  EXPECT_FALSE(whitespace->TextLayoutObjectIsNeeded(
      context, in_line->StyleRef(), *in_line));

  context.previous_in_flow = br;
  EXPECT_FALSE(
      whitespace->TextLayoutObjectIsNeeded(context, block->StyleRef(), *block));
  EXPECT_FALSE(whitespace->TextLayoutObjectIsNeeded(
      context, in_line->StyleRef(), *in_line));
}

TEST_F(TextTest, TextLayoutObjectIsNeeded_PreserveNewLine) {
  SetBodyContent(R"HTML(
    <div id=pre style='white-space:pre'></div>
    <div id=pre-line style='white-space:pre-line'></div>
    <div id=pre-wrap style='white-space:pre-wrap'></div>
  )HTML");
  UpdateAllLifecyclePhases();

  Text* text = Text::Create(GetDocument(), " ");

  Element* pre = GetDocument().getElementById("pre");
  ASSERT_TRUE(pre);
  const LayoutObject* pre_layout = pre->GetLayoutObject();
  ASSERT_TRUE(pre_layout);
  const ComputedStyle& pre_style = pre_layout->StyleRef();
  EXPECT_TRUE(text->TextLayoutObjectIsNeeded(Node::AttachContext(), pre_style,
                                             *pre_layout));

  Element* pre_line = GetDocument().getElementById("pre-line");
  ASSERT_TRUE(pre_line);
  const LayoutObject* pre_line_layout = pre_line->GetLayoutObject();
  ASSERT_TRUE(pre_line_layout);
  const ComputedStyle& pre_line_style = pre_line_layout->StyleRef();
  EXPECT_TRUE(text->TextLayoutObjectIsNeeded(Node::AttachContext(),
                                             pre_line_style, *pre_line_layout));

  Element* pre_wrap = GetDocument().getElementById("pre-wrap");
  ASSERT_TRUE(pre_wrap);
  const LayoutObject* pre_wrap_layout = pre_wrap->GetLayoutObject();
  ASSERT_TRUE(pre_wrap_layout);
  const ComputedStyle& pre_wrap_style = pre_wrap_layout->StyleRef();
  EXPECT_TRUE(text->TextLayoutObjectIsNeeded(Node::AttachContext(),
                                             pre_wrap_style, *pre_wrap_layout));
}

}  // namespace blink
