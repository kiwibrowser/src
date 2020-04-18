// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/layout_text_control.h"

#include "build/build_config.h"
#include "third_party/blink/renderer/core/html/forms/html_input_element.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"

namespace blink {

namespace {

class LayoutTextControlTest : public RenderingTest {
 protected:
  HTMLInputElement* GetHTMLInputElementById(const char* id) {
    return ToHTMLInputElement(GetDocument().getElementById(id));
  }
  // Return the LayoutText from inside an HTMLInputElement's user agent shadow
  // tree.
  LayoutText* GetInnerLayoutText(HTMLInputElement* input) {
    return ToLayoutText(
        input->InnerEditorElement()->GetLayoutObject()->SlowFirstChild());
  }
};

TEST_F(LayoutTextControlTest,
       ChangingPseudoSelectionStyleShouldInvalidateSelection) {
  SetBodyInnerHTML(R"HTML(
    <style>
      input::selection { background-color: blue; }
      .pseudoSelection::selection { background-color: green; }
    </style>
    <input id="input" type="text" value="AAAAAAAAAAAA">
  )HTML");

  auto* inputElement = GetHTMLInputElementById("input");
  inputElement->focus();
  inputElement->SetSelectionRange(1, 3);
  GetDocument().View()->UpdateAllLifecyclePhases();

  auto* selectedText = GetInnerLayoutText(inputElement);
  EXPECT_FALSE(selectedText->ShouldInvalidateSelection());

  inputElement->setAttribute(HTMLNames::classAttr, "pseudoSelection");
  GetDocument().View()->UpdateLifecycleToLayoutClean();
  EXPECT_TRUE(selectedText->ShouldInvalidateSelection());

  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_FALSE(selectedText->ShouldInvalidateSelection());
}

TEST_F(LayoutTextControlTest,
       AddingPseudoSelectionStyleShouldInvalidateSelection) {
  SetBodyInnerHTML(R"HTML(
    <style>
      .pseudoSelection::selection { background-color: green; }
    </style>
    <input id="input" type="text" value="AAAAAAAAAAAA">
  )HTML");

  auto* inputElement = GetHTMLInputElementById("input");
  inputElement->focus();
  inputElement->SetSelectionRange(1, 3);
  GetDocument().View()->UpdateAllLifecyclePhases();

  auto* selectedText = GetInnerLayoutText(inputElement);
  EXPECT_FALSE(selectedText->ShouldInvalidateSelection());

  inputElement->setAttribute(HTMLNames::classAttr, "pseudoSelection");
  GetDocument().View()->UpdateLifecycleToLayoutClean();
  EXPECT_TRUE(selectedText->ShouldInvalidateSelection());

  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_FALSE(selectedText->ShouldInvalidateSelection());
}

TEST_F(LayoutTextControlTest,
       RemovingPseudoSelectionStyleShouldInvalidateSelection) {
  SetBodyInnerHTML(R"HTML(
    <style>
      .pseudoSelection::selection { background-color: green; }
    </style>
    <input id="input" type="text" class="pseudoSelection" value="AAAAAAAAAAAA">
  )HTML");

  auto* inputElement = GetHTMLInputElementById("input");
  inputElement->focus();
  inputElement->SetSelectionRange(1, 3);
  GetDocument().View()->UpdateAllLifecyclePhases();

  auto* selectedText = GetInnerLayoutText(inputElement);
  EXPECT_FALSE(selectedText->ShouldInvalidateSelection());

  inputElement->removeAttribute(HTMLNames::classAttr);
  GetDocument().View()->UpdateLifecycleToLayoutClean();
  EXPECT_TRUE(selectedText->ShouldInvalidateSelection());

  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_FALSE(selectedText->ShouldInvalidateSelection());
}

}  // anonymous namespace

}  // namespace blink
