// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/commands/apply_style_command.h"

#include "third_party/blink/renderer/core/css/css_property_value_set.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/editing/editing_style.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/selection_template.h"
#include "third_party/blink/renderer/core/editing/testing/editing_test_base.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"

namespace blink {

class ApplyStyleCommandTest : public EditingTestBase {};

// This is a regression test for https://crbug.com/675727
TEST_F(ApplyStyleCommandTest, RemoveRedundantBlocksWithStarEditableStyle) {
  // The second <div> below is redundant from Blink's perspective (no siblings
  // && no attributes) and will be removed by
  // |DeleteSelectionCommand::removeRedundantBlocks()|.
  SetBodyContent(
      "<div><div>"
      "<div></div>"
      "<ul>"
      "<li>"
      "<div></div>"
      "<input>"
      "<style> * {-webkit-user-modify: read-write;}</style><div></div>"
      "</li>"
      "</ul></div></div>");

  Element* li = GetDocument().QuerySelector("li");

  LocalFrame* frame = GetDocument().GetFrame();
  frame->Selection().SetSelectionAndEndTyping(
      SelectionInDOMTree::Builder()
          .Collapse(Position(li, PositionAnchorType::kBeforeAnchor))
          .Build());

  MutableCSSPropertyValueSet* style =
      MutableCSSPropertyValueSet::Create(kHTMLQuirksMode);
  style->SetProperty(CSSPropertyTextAlign, "center", /* important */ false,
                     SecureContextMode::kInsecureContext);
  ApplyStyleCommand::Create(GetDocument(), EditingStyle::Create(style),
                            InputEvent::InputType::kFormatJustifyCenter,
                            ApplyStyleCommand::kForceBlockProperties)
      ->Apply();
  // Shouldn't crash.
}

// This is a regression test for https://crbug.com/761280
TEST_F(ApplyStyleCommandTest, JustifyRightDetachesDestination) {
  SetBodyContent(
      "<style>"
      ".CLASS1{visibility:visible;}"
      "*:last-child{visibility:collapse;display:list-item;}"
      "</style>"
      "<input class=CLASS1>"
      "<ruby>"
      "<button class=CLASS1></button>"
      "<button></button>"
      "</ruby");
  Element* body = GetDocument().body();
  // The bug does't reproduce with a contenteditable <div> as container.
  body->setAttribute(HTMLNames::contenteditableAttr, "true");
  GetDocument().UpdateStyleAndLayout();
  Selection().SelectAll();

  MutableCSSPropertyValueSet* style =
      MutableCSSPropertyValueSet::Create(kHTMLQuirksMode);
  style->SetProperty(CSSPropertyTextAlign, "right", /* important */ false,
                     SecureContextMode::kInsecureContext);
  ApplyStyleCommand::Create(GetDocument(), EditingStyle::Create(style),
                            InputEvent::InputType::kFormatJustifyCenter,
                            ApplyStyleCommand::kForceBlockProperties)
      ->Apply();
  // Shouldn't crash.
}

// This is a regression test for https://crbug.com/726992
TEST_F(ApplyStyleCommandTest, FontSizeDeltaWithSpanElement) {
  Selection().SetSelectionAndEndTyping(SetSelectionTextToBody(
      "<div contenteditable>^<div></div>a<span></span>|</div>"));

  MutableCSSPropertyValueSet* style =
      MutableCSSPropertyValueSet::Create(kHTMLQuirksMode);
  style->SetProperty(CSSPropertyWebkitFontSizeDelta, "3", /* important */ false,
                     GetDocument().GetSecureContextMode());
  ApplyStyleCommand::Create(GetDocument(), EditingStyle::Create(style),
                            InputEvent::InputType::kNone)
      ->Apply();
  EXPECT_EQ("<div contenteditable><div></div><span>^a|</span></div>",
            GetSelectionTextFromBody());
}
}  // namespace blink
