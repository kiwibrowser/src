// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/whitespace_attacher.h"

#include <gtest/gtest.h>
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/core/dom/node_computed_style.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/dom/shadow_root_init.h"
#include "third_party/blink/renderer/core/layout/layout_text.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"

namespace blink {

class WhitespaceAttacherTest : public PageTestBase {};

TEST_F(WhitespaceAttacherTest, WhitespaceAfterReattachedBlock) {
  GetDocument().body()->SetInnerHTMLFromString("<div id=block></div> ");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Element* div = GetDocument().getElementById("block");
  Text* text = ToText(div->nextSibling());
  EXPECT_FALSE(text->GetLayoutObject());

  GetDocument().Lifecycle().AdvanceTo(DocumentLifecycle::kInStyleRecalc);

  // Force LayoutText to see that the reattach works.
  text->SetLayoutObject(
      text->CreateTextLayoutObject(GetDocument().body()->ComputedStyleRef()));

  WhitespaceAttacher attacher;
  attacher.DidVisitText(text);
  attacher.DidReattachElement(div, div->GetLayoutObject());
  EXPECT_FALSE(text->GetLayoutObject());
}

TEST_F(WhitespaceAttacherTest, WhitespaceAfterReattachedInline) {
  GetDocument().body()->SetInnerHTMLFromString("<span id=inline></span> ");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Element* span = GetDocument().getElementById("inline");
  Text* text = ToText(span->nextSibling());
  EXPECT_TRUE(text->GetLayoutObject());

  GetDocument().Lifecycle().AdvanceTo(DocumentLifecycle::kInStyleRecalc);

  // Clear LayoutText to see that the reattach works.
  text->SetLayoutObject(nullptr);

  WhitespaceAttacher attacher;
  attacher.DidVisitText(text);
  attacher.DidReattachElement(span, span->GetLayoutObject());
  EXPECT_TRUE(text->GetLayoutObject());
}

TEST_F(WhitespaceAttacherTest, WhitespaceAfterReattachedWhitespace) {
  GetDocument().body()->SetInnerHTMLFromString(
      "<span id=inline></span> <!-- --> ");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Element* span = GetDocument().getElementById("inline");
  Text* first_whitespace = ToText(span->nextSibling());
  Text* second_whitespace =
      ToText(first_whitespace->nextSibling()->nextSibling());
  EXPECT_TRUE(first_whitespace->GetLayoutObject());
  EXPECT_FALSE(second_whitespace->GetLayoutObject());

  GetDocument().Lifecycle().AdvanceTo(DocumentLifecycle::kInStyleRecalc);

  // Force LayoutText on the second whitespace to see that the reattach works.
  second_whitespace->SetLayoutObject(second_whitespace->CreateTextLayoutObject(
      GetDocument().body()->ComputedStyleRef()));

  WhitespaceAttacher attacher;
  attacher.DidVisitText(second_whitespace);
  EXPECT_TRUE(second_whitespace->GetLayoutObject());

  attacher.DidReattachText(first_whitespace);
  EXPECT_TRUE(first_whitespace->GetLayoutObject());
  EXPECT_FALSE(second_whitespace->GetLayoutObject());
}

TEST_F(WhitespaceAttacherTest, VisitBlockAfterReattachedWhitespace) {
  GetDocument().body()->SetInnerHTMLFromString("<div id=block></div> ");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Element* div = GetDocument().getElementById("block");
  Text* text = ToText(div->nextSibling());
  EXPECT_FALSE(text->GetLayoutObject());

  GetDocument().Lifecycle().AdvanceTo(DocumentLifecycle::kInStyleRecalc);

  WhitespaceAttacher attacher;
  attacher.DidReattachText(text);
  EXPECT_FALSE(text->GetLayoutObject());

  attacher.DidVisitElement(div);
  EXPECT_FALSE(text->GetLayoutObject());
}

TEST_F(WhitespaceAttacherTest, VisitInlineAfterReattachedWhitespace) {
  GetDocument().body()->SetInnerHTMLFromString("<span id=inline></span> ");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Element* span = GetDocument().getElementById("inline");
  Text* text = ToText(span->nextSibling());
  EXPECT_TRUE(text->GetLayoutObject());

  GetDocument().Lifecycle().AdvanceTo(DocumentLifecycle::kInStyleRecalc);

  // Clear LayoutText to see that the reattach works.
  text->SetLayoutObject(nullptr);

  WhitespaceAttacher attacher;
  attacher.DidReattachText(text);
  EXPECT_FALSE(text->GetLayoutObject());

  attacher.DidVisitElement(span);
  EXPECT_TRUE(text->GetLayoutObject());
}

TEST_F(WhitespaceAttacherTest, VisitTextAfterReattachedWhitespace) {
  GetDocument().body()->SetInnerHTMLFromString("Text<!-- --> ");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Text* text = ToText(GetDocument().body()->firstChild());
  Text* whitespace = ToText(text->nextSibling()->nextSibling());
  EXPECT_TRUE(text->GetLayoutObject());
  EXPECT_TRUE(whitespace->GetLayoutObject());

  GetDocument().Lifecycle().AdvanceTo(DocumentLifecycle::kInStyleRecalc);

  // Clear LayoutText to see that the reattach works.
  whitespace->SetLayoutObject(nullptr);

  WhitespaceAttacher attacher;
  attacher.DidReattachText(whitespace);
  EXPECT_FALSE(whitespace->GetLayoutObject());

  attacher.DidVisitText(text);
  EXPECT_TRUE(text->GetLayoutObject());
  EXPECT_TRUE(whitespace->GetLayoutObject());
}

TEST_F(WhitespaceAttacherTest, ReattachWhitespaceInsideBlockExitingScope) {
  GetDocument().body()->SetInnerHTMLFromString("<div id=block> </div>");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Element* div = GetDocument().getElementById("block");
  Text* text = ToText(div->firstChild());
  EXPECT_FALSE(text->GetLayoutObject());

  GetDocument().Lifecycle().AdvanceTo(DocumentLifecycle::kInStyleRecalc);

  {
    WhitespaceAttacher attacher;
    attacher.DidReattachText(text);
    EXPECT_FALSE(text->GetLayoutObject());

    // Force LayoutText to see that the reattach works.
    text->SetLayoutObject(
        text->CreateTextLayoutObject(div->ComputedStyleRef()));
  }
  EXPECT_FALSE(text->GetLayoutObject());
}

TEST_F(WhitespaceAttacherTest, ReattachWhitespaceInsideInlineExitingScope) {
  GetDocument().body()->SetInnerHTMLFromString("<span id=inline> </span>");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Element* span = GetDocument().getElementById("inline");
  Text* text = ToText(span->firstChild());
  EXPECT_TRUE(text->GetLayoutObject());

  GetDocument().Lifecycle().AdvanceTo(DocumentLifecycle::kInStyleRecalc);

  // Clear LayoutText to see that the reattach works.
  text->SetLayoutObject(nullptr);

  {
    WhitespaceAttacher attacher;
    attacher.DidReattachText(text);
    EXPECT_FALSE(text->GetLayoutObject());
  }
  EXPECT_TRUE(text->GetLayoutObject());
}

TEST_F(WhitespaceAttacherTest, SlottedWhitespaceAfterReattachedBlock) {
  GetDocument().body()->SetInnerHTMLFromString("<div id=host> </div>");
  Element* host = GetDocument().getElementById("host");
  ASSERT_TRUE(host);

  ShadowRoot& shadow_root =
      host->AttachShadowRootInternal(ShadowRootType::kOpen);
  shadow_root.SetInnerHTMLFromString("<div id=block></div><slot></slot>");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Element* div = shadow_root.getElementById("block");
  Text* text = ToText(host->firstChild());
  EXPECT_FALSE(text->GetLayoutObject());

  GetDocument().Lifecycle().AdvanceTo(DocumentLifecycle::kInStyleRecalc);

  // Force LayoutText to see that the reattach works.
  text->SetLayoutObject(text->CreateTextLayoutObject(host->ComputedStyleRef()));

  WhitespaceAttacher attacher;
  attacher.DidVisitText(text);
  EXPECT_TRUE(text->GetLayoutObject());

  attacher.DidReattachElement(div, div->GetLayoutObject());
  EXPECT_FALSE(text->GetLayoutObject());
}

TEST_F(WhitespaceAttacherTest, SlottedWhitespaceAfterReattachedInline) {
  GetDocument().body()->SetInnerHTMLFromString("<div id=host> </div>");
  Element* host = GetDocument().getElementById("host");
  ASSERT_TRUE(host);

  ShadowRoot& shadow_root =
      host->AttachShadowRootInternal(ShadowRootType::kOpen);
  shadow_root.SetInnerHTMLFromString("<span id=inline></span><slot></slot>");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Element* span = shadow_root.getElementById("inline");
  Text* text = ToText(host->firstChild());
  EXPECT_TRUE(text->GetLayoutObject());

  GetDocument().Lifecycle().AdvanceTo(DocumentLifecycle::kInStyleRecalc);

  // Clear LayoutText to see that the reattach works.
  text->SetLayoutObject(nullptr);

  WhitespaceAttacher attacher;
  attacher.DidVisitText(text);
  EXPECT_FALSE(text->GetLayoutObject());

  attacher.DidReattachElement(span, span->GetLayoutObject());
  EXPECT_TRUE(text->GetLayoutObject());
}

TEST_F(WhitespaceAttacherTest,
       WhitespaceInDisplayContentsAfterReattachedBlock) {
  GetDocument().body()->SetInnerHTMLFromString(
      "<div id=block></div><span style='display:contents'> </span>");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Element* div = GetDocument().getElementById("block");
  Element* contents = ToElement(div->nextSibling());
  Text* text = ToText(contents->firstChild());
  EXPECT_FALSE(contents->GetLayoutObject());
  EXPECT_FALSE(text->GetLayoutObject());

  GetDocument().Lifecycle().AdvanceTo(DocumentLifecycle::kInStyleRecalc);

  // Force LayoutText to see that the reattach works.
  text->SetLayoutObject(
      text->CreateTextLayoutObject(contents->ComputedStyleRef()));

  WhitespaceAttacher attacher;
  attacher.DidVisitElement(contents);
  EXPECT_TRUE(text->GetLayoutObject());

  attacher.DidReattachElement(div, div->GetLayoutObject());
  EXPECT_FALSE(text->GetLayoutObject());
}

TEST_F(WhitespaceAttacherTest,
       WhitespaceInDisplayContentsAfterReattachedInline) {
  GetDocument().body()->SetInnerHTMLFromString(
      "<span id=inline></span><span style='display:contents'> </span>");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Element* span = GetDocument().getElementById("inline");
  Element* contents = ToElement(span->nextSibling());
  Text* text = ToText(contents->firstChild());
  EXPECT_FALSE(contents->GetLayoutObject());
  EXPECT_TRUE(text->GetLayoutObject());

  GetDocument().Lifecycle().AdvanceTo(DocumentLifecycle::kInStyleRecalc);

  // Clear LayoutText to see that the reattach works.
  text->SetLayoutObject(nullptr);

  WhitespaceAttacher attacher;
  attacher.DidVisitElement(contents);
  EXPECT_FALSE(text->GetLayoutObject());

  attacher.DidReattachElement(span, span->GetLayoutObject());
  EXPECT_TRUE(text->GetLayoutObject());
}

TEST_F(WhitespaceAttacherTest,
       WhitespaceAfterEmptyDisplayContentsAfterReattachedBlock) {
  GetDocument().body()->SetInnerHTMLFromString(
      "<div id=block></div><span style='display:contents'></span> ");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Element* div = GetDocument().getElementById("block");
  Element* contents = ToElement(div->nextSibling());
  Text* text = ToText(contents->nextSibling());
  EXPECT_FALSE(contents->GetLayoutObject());
  EXPECT_FALSE(text->GetLayoutObject());

  GetDocument().Lifecycle().AdvanceTo(DocumentLifecycle::kInStyleRecalc);

  // Force LayoutText to see that the reattach works.
  text->SetLayoutObject(
      text->CreateTextLayoutObject(contents->ComputedStyleRef()));

  WhitespaceAttacher attacher;
  attacher.DidVisitText(text);
  attacher.DidVisitElement(contents);
  EXPECT_TRUE(text->GetLayoutObject());

  attacher.DidReattachElement(div, div->GetLayoutObject());
  EXPECT_FALSE(text->GetLayoutObject());
}

TEST_F(WhitespaceAttacherTest,
       WhitespaceAfterDisplayContentsWithDisplayNoneChildAfterReattachedBlock) {
  GetDocument().body()->SetInnerHTMLFromString(
      "<div id=block></div><span style='display:contents'>"
      "<span style='display:none'></span></span> ");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Element* div = GetDocument().getElementById("block");
  Element* contents = ToElement(div->nextSibling());
  Text* text = ToText(contents->nextSibling());
  EXPECT_FALSE(contents->GetLayoutObject());
  EXPECT_FALSE(text->GetLayoutObject());

  GetDocument().Lifecycle().AdvanceTo(DocumentLifecycle::kInStyleRecalc);

  // Force LayoutText to see that the reattach works.
  text->SetLayoutObject(
      text->CreateTextLayoutObject(contents->ComputedStyleRef()));

  WhitespaceAttacher attacher;
  attacher.DidVisitText(text);
  attacher.DidVisitElement(contents);
  EXPECT_TRUE(text->GetLayoutObject());

  attacher.DidReattachElement(div, div->GetLayoutObject());
  EXPECT_FALSE(text->GetLayoutObject());
}

TEST_F(WhitespaceAttacherTest, WhitespaceDeepInsideDisplayContents) {
  GetDocument().body()->SetInnerHTMLFromString(
      "<span id=inline></span><span style='display:contents'>"
      "<span style='display:none'></span>"
      "<span id=inner style='display:contents'> </span></span>");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Element* span = GetDocument().getElementById("inline");
  Element* contents = ToElement(span->nextSibling());
  Text* text = ToText(GetDocument().getElementById("inner")->firstChild());
  EXPECT_TRUE(text->GetLayoutObject());

  GetDocument().Lifecycle().AdvanceTo(DocumentLifecycle::kInStyleRecalc);

  // Clear LayoutText to see that the reattach works.
  text->SetLayoutObject(nullptr);

  WhitespaceAttacher attacher;
  attacher.DidVisitElement(contents);
  EXPECT_FALSE(text->GetLayoutObject());

  attacher.DidReattachElement(span, span->GetLayoutObject());
  EXPECT_TRUE(text->GetLayoutObject());
}

TEST_F(WhitespaceAttacherTest, MultipleDisplayContents) {
  GetDocument().body()->SetInnerHTMLFromString(
      "<span id=inline></span>"
      "<span style='display:contents'></span>"
      "<span style='display:contents'></span>"
      "<span style='display:contents'> </span>");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Element* span = GetDocument().getElementById("inline");
  Element* first_contents = ToElement(span->nextSibling());
  Element* second_contents = ToElement(first_contents->nextSibling());
  Element* last_contents = ToElement(second_contents->nextSibling());
  Text* text = ToText(last_contents->firstChild());
  EXPECT_TRUE(text->GetLayoutObject());

  GetDocument().Lifecycle().AdvanceTo(DocumentLifecycle::kInStyleRecalc);

  // Clear LayoutText to see that the reattach works.
  text->SetLayoutObject(nullptr);

  WhitespaceAttacher attacher;
  attacher.DidVisitElement(last_contents);
  attacher.DidVisitElement(second_contents);
  attacher.DidVisitElement(first_contents);
  EXPECT_FALSE(text->GetLayoutObject());

  attacher.DidReattachElement(span, span->GetLayoutObject());
  EXPECT_TRUE(text->GetLayoutObject());
}

TEST_F(WhitespaceAttacherTest, SlottedWhitespaceInsideDisplayContents) {
  GetDocument().body()->SetInnerHTMLFromString("<div id=host> </div>");
  Element* host = GetDocument().getElementById("host");
  ASSERT_TRUE(host);

  ShadowRoot& shadow_root =
      host->AttachShadowRootInternal(ShadowRootType::kOpen);
  shadow_root.SetInnerHTMLFromString(
      "<span id=inline></span>"
      "<div style='display:contents'><slot></slot></div>");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Element* span = shadow_root.getElementById("inline");
  Element* contents = ToElement(span->nextSibling());
  Text* text = ToText(host->firstChild());
  EXPECT_TRUE(text->GetLayoutObject());

  GetDocument().Lifecycle().AdvanceTo(DocumentLifecycle::kInStyleRecalc);

  // Clear LayoutText to see that the reattach works.
  text->SetLayoutObject(nullptr);

  WhitespaceAttacher attacher;
  attacher.DidVisitElement(contents);
  EXPECT_FALSE(text->GetLayoutObject());

  attacher.DidReattachElement(span, span->GetLayoutObject());
  EXPECT_TRUE(text->GetLayoutObject());
}

TEST_F(WhitespaceAttacherTest, RemoveInlineBeforeSpace) {
  GetDocument().body()->SetInnerHTMLFromString("<span id=inline></span> ");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Element* span = GetDocument().getElementById("inline");
  ASSERT_TRUE(span);
  EXPECT_TRUE(span->GetLayoutObject());

  Node* text = span->nextSibling();
  ASSERT_TRUE(text);
  EXPECT_TRUE(text->IsTextNode());
  EXPECT_TRUE(text->GetLayoutObject());

  span->remove();
  GetDocument().View()->UpdateAllLifecyclePhases();

  EXPECT_FALSE(text->previousSibling());
  EXPECT_TRUE(text->IsTextNode());
  EXPECT_FALSE(text->nextSibling());
  EXPECT_FALSE(text->GetLayoutObject());
}

TEST_F(WhitespaceAttacherTest, RemoveInlineBeforeOutOfFlowBeforeSpace) {
  GetDocument().body()->SetInnerHTMLFromString(
      "<span id=inline></span><div id=float style='float:right'></div> ");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Element* span = GetDocument().getElementById("inline");
  ASSERT_TRUE(span);
  EXPECT_TRUE(span->GetLayoutObject());

  Element* floated = GetDocument().getElementById("float");
  ASSERT_TRUE(floated);
  EXPECT_TRUE(floated->GetLayoutObject());

  Node* text = floated->nextSibling();
  ASSERT_TRUE(text);
  EXPECT_TRUE(text->IsTextNode());
  EXPECT_TRUE(text->GetLayoutObject());

  span->remove();
  GetDocument().View()->UpdateAllLifecyclePhases();

  EXPECT_TRUE(text->IsTextNode());
  EXPECT_FALSE(text->nextSibling());
  EXPECT_FALSE(text->GetLayoutObject());
}

TEST_F(WhitespaceAttacherTest, RemoveSpaceBeforeSpace) {
  GetDocument().body()->SetInnerHTMLFromString("<span> <!-- --> </span>");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Node* span = GetDocument().body()->firstChild();
  ASSERT_TRUE(span);

  Node* space1 = span->firstChild();
  ASSERT_TRUE(space1);
  EXPECT_TRUE(space1->IsTextNode());
  EXPECT_TRUE(space1->GetLayoutObject());

  Node* space2 = span->lastChild();
  ASSERT_TRUE(space2);
  EXPECT_TRUE(space2->IsTextNode());
  EXPECT_FALSE(space2->GetLayoutObject());

  space1->remove();
  GetDocument().View()->UpdateAllLifecyclePhases();

  EXPECT_TRUE(space2->GetLayoutObject());
}

TEST_F(WhitespaceAttacherTest, RemoveInlineBeforeDisplayContentsWithSpace) {
  GetDocument().body()->SetInnerHTMLFromString(
      "<style>div { display: contents }</style>"
      "<div><span id=inline></span></div>"
      "<div><div><div id=innerdiv> </div></div></div>text");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Node* span = GetDocument().getElementById("inline");
  ASSERT_TRUE(span);

  Node* space = GetDocument().getElementById("innerdiv")->firstChild();
  ASSERT_TRUE(space);
  EXPECT_TRUE(space->IsTextNode());
  EXPECT_TRUE(space->GetLayoutObject());

  span->remove();
  GetDocument().View()->UpdateAllLifecyclePhases();

  EXPECT_FALSE(space->GetLayoutObject());
}

TEST_F(WhitespaceAttacherTest, RemoveBlockBeforeSpace) {
  GetDocument().body()->SetInnerHTMLFromString(
      "A<div id=block></div> <span>B</span>");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Node* div = GetDocument().getElementById("block");
  ASSERT_TRUE(div);

  Node* space = div->nextSibling();
  ASSERT_TRUE(space);
  EXPECT_TRUE(space->IsTextNode());
  EXPECT_FALSE(space->GetLayoutObject());

  div->remove();
  GetDocument().View()->UpdateAllLifecyclePhases();

  EXPECT_TRUE(space->GetLayoutObject());
}

}  // namespace blink
