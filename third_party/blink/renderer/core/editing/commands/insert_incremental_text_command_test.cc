// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/commands/insert_incremental_text_command.h"

#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/selection_template.h"
#include "third_party/blink/renderer/core/editing/testing/editing_test_base.h"

namespace blink {

class InsertIncrementalTextCommandTest : public EditingTestBase {};

// http://crbug.com/706166
TEST_F(InsertIncrementalTextCommandTest, SurrogatePairsReplace) {
  SetBodyContent("<div id=sample contenteditable><a>a</a>b&#x1F63A;</div>");
  Element* const sample = GetDocument().getElementById("sample");
  const String new_text(Vector<UChar>{0xD83D, 0xDE38});  // U+1F638
  Selection().SetSelectionAndEndTyping(
      SelectionInDOMTree::Builder()
          .Collapse(Position(sample->lastChild(), 1))
          .Extend(Position(sample->lastChild(), 3))
          .Build());
  CompositeEditCommand* const command =
      InsertIncrementalTextCommand::Create(GetDocument(), new_text);
  command->Apply();

  EXPECT_EQ(String(Vector<UChar>{'b', 0xD83D, 0xDE38}),
            sample->lastChild()->nodeValue())
      << "Replace 'U+D83D U+DE3A (U+1F63A) with 'U+D83D U+DE38'(U+1F638)";
}

TEST_F(InsertIncrementalTextCommandTest, SurrogatePairsNoReplace) {
  SetBodyContent("<div id=sample contenteditable><a>a</a>b&#x1F63A;</div>");
  Element* const sample = GetDocument().getElementById("sample");
  const String new_text(Vector<UChar>{0xD83D, 0xDE3A});  // U+1F63A
  Selection().SetSelectionAndEndTyping(
      SelectionInDOMTree::Builder()
          .Collapse(Position(sample->lastChild(), 1))
          .Extend(Position(sample->lastChild(), 3))
          .Build());
  CompositeEditCommand* const command =
      InsertIncrementalTextCommand::Create(GetDocument(), new_text);
  command->Apply();

  EXPECT_EQ(String(Vector<UChar>{'b', 0xD83D, 0xDE3A}),
            sample->lastChild()->nodeValue())
      << "Replace 'U+D83D U+DE3A(U+1F63A) with 'U+D83D U+DE3A'(U+1F63A)";
}

// http://crbug.com/706166
TEST_F(InsertIncrementalTextCommandTest, SurrogatePairsTwo) {
  SetBodyContent(
      "<div id=sample contenteditable><a>a</a>b&#x1F63A;&#x1F63A;</div>");
  Element* const sample = GetDocument().getElementById("sample");
  const String new_text(Vector<UChar>{0xD83D, 0xDE38});  // U+1F638
  Selection().SetSelectionAndEndTyping(
      SelectionInDOMTree::Builder()
          .Collapse(Position(sample->lastChild(), 1))
          .Extend(Position(sample->lastChild(), 5))
          .Build());
  CompositeEditCommand* const command =
      InsertIncrementalTextCommand::Create(GetDocument(), new_text);
  command->Apply();

  EXPECT_EQ(String(Vector<UChar>{'b', 0xD83D, 0xDE38}),
            sample->lastChild()->nodeValue())
      << "Replace 'U+1F63A U+1F63A with U+1F638";
}

}  // namespace blink
