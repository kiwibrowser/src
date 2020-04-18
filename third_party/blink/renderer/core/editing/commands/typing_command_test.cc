// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/commands/typing_command.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/position.h"
#include "third_party/blink/renderer/core/editing/selection_template.h"
#include "third_party/blink/renderer/core/editing/testing/editing_test_base.h"
#include "third_party/blink/renderer/core/editing/visible_selection.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"

#include <memory>

namespace blink {

class TypingCommandTest : public EditingTestBase {};

// This is a regression test for https://crbug.com/585048
TEST_F(TypingCommandTest, insertLineBreakWithIllFormedHTML) {
  SetBodyContent("<div contenteditable></div>");

  // <input><form></form></input>
  Element* input1 = GetDocument().CreateRawElement(HTMLNames::inputTag);
  Element* form = GetDocument().CreateRawElement(HTMLNames::formTag);
  input1->AppendChild(form);

  // <tr><input><header></header></input><rbc></rbc></tr>
  Element* tr = GetDocument().CreateRawElement(HTMLNames::trTag);
  Element* input2 = GetDocument().CreateRawElement(HTMLNames::inputTag);
  Element* header = GetDocument().CreateRawElement(HTMLNames::headerTag);
  Element* rbc = GetDocument().CreateElementForBinding("rbc");
  input2->AppendChild(header);
  tr->AppendChild(input2);
  tr->AppendChild(rbc);

  Element* div = GetDocument().QuerySelector("div");
  div->AppendChild(input1);
  div->AppendChild(tr);

  LocalFrame* frame = GetDocument().GetFrame();
  frame->Selection().SetSelectionAndEndTyping(SelectionInDOMTree::Builder()
                                                  .Collapse(Position(form, 0))
                                                  .Extend(Position(header, 0))
                                                  .Build());

  // Inserting line break should not crash or hit assertion.
  TypingCommand::InsertLineBreak(GetDocument());
}

// http://crbug.com/767599
TEST_F(TypingCommandTest,
       DontCrashWhenReplaceSelectionCommandLeavesBadSelection) {
  Selection().SetSelectionAndEndTyping(
      SetSelectionTextToBody("<div contenteditable>^<h1>H1</h1>ello|</div>"));

  // This call shouldn't crash.
  TypingCommand::InsertText(
      GetDocument(), " ", 0,
      TypingCommand::TextCompositionType::kTextCompositionUpdate, true);
  EXPECT_EQ("<div contenteditable>^<h1></h1>|</div>",
            GetSelectionTextFromBody());
}

// crbug.com/794397
TEST_F(TypingCommandTest, ForwardDeleteInvalidatesSelection) {
  GetDocument().setDesignMode("on");
  Selection().SetSelectionAndEndTyping(SetSelectionTextToBody(
      "<blockquote>^"
      "<q>"
      "<table contenteditable=\"false\"><colgroup width=\"-1\">\n</table>|"
      "</q>"
      "</blockquote>"
      "<q>\n<svg></svg></q>"));

  EditingState editing_state;
  TypingCommand::ForwardDeleteKeyPressed(GetDocument(), &editing_state);

  EXPECT_EQ(
      "<blockquote>|</blockquote>"
      "<q>"
      "<table contenteditable=\"false\">"
      "<colgroup width=\"-1\"></colgroup>"
      "</table>\n"
      "<svg></svg>"
      "</q>",
      GetSelectionTextFromBody());
}

}  // namespace blink
