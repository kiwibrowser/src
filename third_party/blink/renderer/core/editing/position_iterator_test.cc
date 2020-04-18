// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/position_iterator.h"

#include "third_party/blink/renderer/core/dom/flat_tree_traversal.h"
#include "third_party/blink/renderer/core/editing/position.h"
#include "third_party/blink/renderer/core/editing/testing/editing_test_base.h"

namespace blink {

class PositionIteratorTest : public EditingTestBase {};

// For http://crbug.com/695317
TEST_F(PositionIteratorTest, decrementWithInputElement) {
  SetBodyContent("123<input value='abc'>");
  Element* const input = GetDocument().QuerySelector("input");
  Node* const text = input->previousSibling();

  // Decrement until start of "123" from INPUT on DOM tree
  PositionIterator dom_iterator(
      Position::LastPositionInNode(*GetDocument().body()));
  EXPECT_EQ(Position::LastPositionInNode(*GetDocument().body()),
            dom_iterator.ComputePosition());
  dom_iterator.Decrement();
  EXPECT_EQ(Position::AfterNode(*input), dom_iterator.ComputePosition());
  dom_iterator.Decrement();
  EXPECT_EQ(Position::BeforeNode(*input), dom_iterator.ComputePosition());
  dom_iterator.Decrement();
  EXPECT_EQ(Position(GetDocument().body(), 1), dom_iterator.ComputePosition());
  dom_iterator.Decrement();
  EXPECT_EQ(Position(text, 3), dom_iterator.ComputePosition());

  // Decrement until start of "123" from INPUT on flat tree
  PositionIteratorInFlatTree flat_iterator(
      PositionInFlatTree::LastPositionInNode(*GetDocument().body()));
  EXPECT_EQ(PositionInFlatTree::LastPositionInNode(*GetDocument().body()),
            flat_iterator.ComputePosition());
  flat_iterator.Decrement();
  EXPECT_EQ(PositionInFlatTree::AfterNode(*input),
            flat_iterator.ComputePosition());
  flat_iterator.Decrement();
  EXPECT_EQ(PositionInFlatTree::BeforeNode(*input),
            flat_iterator.ComputePosition());
  flat_iterator.Decrement();
  EXPECT_EQ(PositionInFlatTree(GetDocument().body(), 1),
            flat_iterator.ComputePosition());
  flat_iterator.Decrement();
  EXPECT_EQ(PositionInFlatTree(text, 3), flat_iterator.ComputePosition());
}

TEST_F(PositionIteratorTest, decrementWithSelectElement) {
  SetBodyContent("123<select><option>1</option><option>2</option></select>");
  Element* const select = GetDocument().QuerySelector("select");
  Node* text = select->previousSibling();

  // Decrement until start of "123" from SELECT on DOM tree
  PositionIterator dom_iterator(
      Position::LastPositionInNode(*GetDocument().body()));
  EXPECT_EQ(Position::LastPositionInNode(*GetDocument().body()),
            dom_iterator.ComputePosition());
  dom_iterator.Decrement();
  EXPECT_EQ(Position::AfterNode(*select), dom_iterator.ComputePosition());
  dom_iterator.Decrement();
  EXPECT_EQ(Position::AfterNode(*select), dom_iterator.ComputePosition())
      << "This is redundant result, we should not have. see "
         "http://crbug.com/697283";
  dom_iterator.Decrement();
  EXPECT_EQ(Position::BeforeNode(*select), dom_iterator.ComputePosition());
  dom_iterator.Decrement();
  EXPECT_EQ(Position(GetDocument().body(), 1), dom_iterator.ComputePosition());
  dom_iterator.Decrement();
  EXPECT_EQ(Position(text, 3), dom_iterator.ComputePosition());

  // Decrement until start of "123" from SELECT on flat tree
  PositionIteratorInFlatTree flat_iterator(
      PositionInFlatTree::LastPositionInNode(*GetDocument().body()));
  EXPECT_EQ(PositionInFlatTree::LastPositionInNode(*GetDocument().body()),
            flat_iterator.ComputePosition());
  flat_iterator.Decrement();
  EXPECT_EQ(PositionInFlatTree::AfterNode(*select),
            flat_iterator.ComputePosition());
  flat_iterator.Decrement();
  EXPECT_EQ(PositionInFlatTree::BeforeNode(*select),
            flat_iterator.ComputePosition());
  flat_iterator.Decrement();
  EXPECT_EQ(PositionInFlatTree(GetDocument().body(), 1),
            flat_iterator.ComputePosition());
  flat_iterator.Decrement();
  EXPECT_EQ(PositionInFlatTree(text, 3), flat_iterator.ComputePosition());
}

// For http://crbug.com/695317
TEST_F(PositionIteratorTest, decrementWithTextAreaElement) {
  SetBodyContent("123<textarea>456</textarea>");
  Element* const textarea = GetDocument().QuerySelector("textarea");
  Node* const text = textarea->previousSibling();

  // Decrement until end of "123" from after TEXTAREA on DOM tree
  PositionIterator dom_iterator(
      Position::LastPositionInNode(*GetDocument().body()));
  EXPECT_EQ(Position::LastPositionInNode(*GetDocument().body()),
            dom_iterator.ComputePosition());
  dom_iterator.Decrement();
  EXPECT_EQ(Position::AfterNode(*textarea), dom_iterator.ComputePosition());
  dom_iterator.Decrement();
  EXPECT_EQ(Position::BeforeNode(*textarea), dom_iterator.ComputePosition());
  dom_iterator.Decrement();
  EXPECT_EQ(Position(GetDocument().body(), 1), dom_iterator.ComputePosition());
  dom_iterator.Decrement();
  EXPECT_EQ(Position(text, 3), dom_iterator.ComputePosition());

  // Decrement until end of "123" from after TEXTAREA on flat tree
  PositionIteratorInFlatTree flat_iterator(
      PositionInFlatTree::LastPositionInNode(*GetDocument().body()));
  EXPECT_EQ(PositionInFlatTree::LastPositionInNode(*GetDocument().body()),
            flat_iterator.ComputePosition());
  flat_iterator.Decrement();
  EXPECT_EQ(PositionInFlatTree::AfterNode(*textarea),
            flat_iterator.ComputePosition());
  flat_iterator.Decrement();
  EXPECT_EQ(PositionInFlatTree::BeforeNode(*textarea),
            flat_iterator.ComputePosition());
  flat_iterator.Decrement();
  EXPECT_EQ(PositionInFlatTree(GetDocument().body(), 1),
            flat_iterator.ComputePosition());
  flat_iterator.Decrement();
  EXPECT_EQ(PositionInFlatTree(text, 3), flat_iterator.ComputePosition());
}

// For http://crbug.com/695317
TEST_F(PositionIteratorTest, incrementWithInputElement) {
  SetBodyContent("<input value='abc'>123");
  Element* const input = GetDocument().QuerySelector("input");
  Node* const text = input->nextSibling();

  // Increment until start of "123" from INPUT on DOM tree
  PositionIterator dom_iterator(
      Position::FirstPositionInNode(*GetDocument().body()));
  EXPECT_EQ(Position(GetDocument().body(), 0), dom_iterator.ComputePosition());
  dom_iterator.Increment();
  EXPECT_EQ(Position::BeforeNode(*input), dom_iterator.ComputePosition());
  dom_iterator.Increment();
  EXPECT_EQ(Position::AfterNode(*input), dom_iterator.ComputePosition());
  dom_iterator.Increment();
  EXPECT_EQ(Position(GetDocument().body(), 1), dom_iterator.ComputePosition());
  dom_iterator.Increment();
  EXPECT_EQ(Position(text, 0), dom_iterator.ComputePosition());

  // Increment until start of "123" from INPUT on flat tree
  PositionIteratorInFlatTree flat_iterator(
      PositionInFlatTree::FirstPositionInNode(*GetDocument().body()));
  EXPECT_EQ(PositionInFlatTree(GetDocument().body(), 0),
            flat_iterator.ComputePosition());
  flat_iterator.Increment();
  EXPECT_EQ(PositionInFlatTree::BeforeNode(*input),
            flat_iterator.ComputePosition());
  flat_iterator.Increment();
  EXPECT_EQ(PositionInFlatTree::AfterNode(*input),
            flat_iterator.ComputePosition());
  flat_iterator.Increment();
  EXPECT_EQ(PositionInFlatTree(GetDocument().body(), 1),
            flat_iterator.ComputePosition());
  flat_iterator.Increment();
  EXPECT_EQ(PositionInFlatTree(text, 0), flat_iterator.ComputePosition());
}

TEST_F(PositionIteratorTest, incrementWithSelectElement) {
  SetBodyContent("<select><option>1</option><option>2</option></select>123");
  Element* const select = GetDocument().QuerySelector("select");
  Node* const text = select->nextSibling();

  // Increment until start of "123" from SELECT on DOM tree
  PositionIterator dom_iterator(
      Position::FirstPositionInNode(*GetDocument().body()));
  EXPECT_EQ(Position(GetDocument().body(), 0), dom_iterator.ComputePosition());
  dom_iterator.Increment();
  EXPECT_EQ(Position::BeforeNode(*select), dom_iterator.ComputePosition());
  dom_iterator.Increment();
  EXPECT_EQ(Position::AfterNode(*select), dom_iterator.ComputePosition());
  dom_iterator.Increment();
  EXPECT_EQ(Position::AfterNode(*select), dom_iterator.ComputePosition())
      << "This is redundant result, we should not have. see "
         "http://crbug.com/697283";
  dom_iterator.Increment();
  EXPECT_EQ(Position(GetDocument().body(), 1), dom_iterator.ComputePosition());
  dom_iterator.Increment();
  EXPECT_EQ(Position(text, 0), dom_iterator.ComputePosition());

  // Increment until start of "123" from SELECT on flat tree
  PositionIteratorInFlatTree flat_iterator(
      PositionInFlatTree::FirstPositionInNode(*GetDocument().body()));
  EXPECT_EQ(PositionInFlatTree(GetDocument().body(), 0),
            flat_iterator.ComputePosition());
  flat_iterator.Increment();
  EXPECT_EQ(PositionInFlatTree::BeforeNode(*select),
            flat_iterator.ComputePosition());
  flat_iterator.Increment();
  EXPECT_EQ(PositionInFlatTree::AfterNode(*select),
            flat_iterator.ComputePosition());
  flat_iterator.Increment();
  EXPECT_EQ(PositionInFlatTree(GetDocument().body(), 1),
            flat_iterator.ComputePosition());
  flat_iterator.Increment();
  EXPECT_EQ(PositionInFlatTree(text, 0), flat_iterator.ComputePosition());
}

// For http://crbug.com/695317
TEST_F(PositionIteratorTest, incrementWithTextAreaElement) {
  SetBodyContent("<textarea>123</textarea>456");
  Element* const textarea = GetDocument().QuerySelector("textarea");
  Node* const text = textarea->nextSibling();

  // Increment until start of "123" from TEXTAREA on DOM tree
  PositionIterator dom_iterator(
      Position::FirstPositionInNode(*GetDocument().body()));
  EXPECT_EQ(Position(GetDocument().body(), 0), dom_iterator.ComputePosition());
  dom_iterator.Increment();
  EXPECT_EQ(Position::BeforeNode(*textarea), dom_iterator.ComputePosition());
  dom_iterator.Increment();
  EXPECT_EQ(Position::AfterNode(*textarea), dom_iterator.ComputePosition());
  dom_iterator.Increment();
  EXPECT_EQ(Position(GetDocument().body(), 1), dom_iterator.ComputePosition());
  dom_iterator.Increment();
  EXPECT_EQ(Position(text, 0), dom_iterator.ComputePosition());

  // Increment until start of "123" from TEXTAREA on flat tree
  PositionIteratorInFlatTree flat_iterator(
      PositionInFlatTree::FirstPositionInNode(*GetDocument().body()));
  EXPECT_EQ(PositionInFlatTree(GetDocument().body(), 0),
            flat_iterator.ComputePosition());
  // TODO(yosin): We should not traverse inside TEXTAREA
  flat_iterator.Increment();
  EXPECT_EQ(PositionInFlatTree::BeforeNode(*textarea),
            flat_iterator.ComputePosition());
  flat_iterator.Increment();
  EXPECT_EQ(PositionInFlatTree::AfterNode(*textarea),
            flat_iterator.ComputePosition());
  flat_iterator.Increment();
  EXPECT_EQ(PositionInFlatTree(GetDocument().body(), 1),
            flat_iterator.ComputePosition());
  flat_iterator.Increment();
  EXPECT_EQ(PositionInFlatTree(text, 0), flat_iterator.ComputePosition());
}

}  // namespace blink
