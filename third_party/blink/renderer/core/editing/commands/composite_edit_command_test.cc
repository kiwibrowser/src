// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/commands/composite_edit_command.h"

#include "third_party/blink/renderer/core/css/css_property_value_set.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/testing/editing_test_base.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"

namespace blink {

namespace {

class SampleCommand final : public CompositeEditCommand {
 public:
  SampleCommand(Document&);

  void InsertNodeBefore(Node*,
                        Node* ref_child,
                        EditingState*,
                        ShouldAssumeContentIsAlwaysEditable =
                            kDoNotAssumeContentIsAlwaysEditable);

  void MoveParagraphContentsToNewBlockIfNecessary(const Position&,
                                                  EditingState*);

  // CompositeEditCommand member implementations
  void DoApply(EditingState*) final {}
  InputEvent::InputType GetInputType() const final {
    return InputEvent::InputType::kNone;
  }
};

SampleCommand::SampleCommand(Document& document)
    : CompositeEditCommand(document) {}

void SampleCommand::InsertNodeBefore(
    Node* insert_child,
    Node* ref_child,
    EditingState* editing_state,
    ShouldAssumeContentIsAlwaysEditable
        should_assume_content_is_always_editable) {
  CompositeEditCommand::InsertNodeBefore(
      insert_child, ref_child, editing_state,
      should_assume_content_is_always_editable);
}

void SampleCommand::MoveParagraphContentsToNewBlockIfNecessary(
    const Position& position,
    EditingState* editing_state) {
  CompositeEditCommand::MoveParagraphContentsToNewBlockIfNecessary(
      position, editing_state);
}

}  // namespace

class CompositeEditCommandTest : public EditingTestBase {};

TEST_F(CompositeEditCommandTest, insertNodeBefore) {
  SetBodyContent("<div contenteditable><b></b></div>");
  SampleCommand& sample = *new SampleCommand(GetDocument());
  Node* insert_child = GetDocument().createTextNode("foo");
  Element* ref_child = GetDocument().QuerySelector("b");
  Element* div = GetDocument().QuerySelector("div");

  EditingState editing_state;
  sample.InsertNodeBefore(insert_child, ref_child, &editing_state);
  EXPECT_FALSE(editing_state.IsAborted());
  EXPECT_EQ("foo<b></b>", div->InnerHTMLAsString());
}

TEST_F(CompositeEditCommandTest, insertNodeBeforeInUneditable) {
  SetBodyContent("<div><b></b></div>");
  SampleCommand& sample = *new SampleCommand(GetDocument());
  Node* insert_child = GetDocument().createTextNode("foo");
  Element* ref_child = GetDocument().QuerySelector("b");

  EditingState editing_state;
  sample.InsertNodeBefore(insert_child, ref_child, &editing_state);
  EXPECT_TRUE(editing_state.IsAborted());
}

TEST_F(CompositeEditCommandTest, insertNodeBeforeDisconnectedNode) {
  SetBodyContent("<div><b></b></div>");
  SampleCommand& sample = *new SampleCommand(GetDocument());
  Node* insert_child = GetDocument().createTextNode("foo");
  Element* ref_child = GetDocument().QuerySelector("b");
  Element* div = GetDocument().QuerySelector("div");
  div->remove();

  EditingState editing_state;
  sample.InsertNodeBefore(insert_child, ref_child, &editing_state);
  EXPECT_FALSE(editing_state.IsAborted());
  EXPECT_EQ("<b></b>", div->InnerHTMLAsString())
      << "InsertNodeBeforeCommand does nothing for disconnected node";
}

TEST_F(CompositeEditCommandTest, insertNodeBeforeWithDirtyLayoutTree) {
  SetBodyContent("<div><b></b></div>");
  SampleCommand& sample = *new SampleCommand(GetDocument());
  Node* insert_child = GetDocument().createTextNode("foo");
  Element* ref_child = GetDocument().QuerySelector("b");
  Element* div = GetDocument().QuerySelector("div");
  div->setAttribute(HTMLNames::contenteditableAttr, "true");

  EditingState editing_state;
  sample.InsertNodeBefore(insert_child, ref_child, &editing_state);
  EXPECT_FALSE(editing_state.IsAborted());
  EXPECT_EQ("foo<b></b>", div->InnerHTMLAsString());
}

TEST_F(CompositeEditCommandTest,
       MoveParagraphContentsToNewBlockWithNonEditableStyle) {
  SetBodyContent(
      "<style>div{-webkit-user-modify:read-only;user-select:none;}</style>"
      "foo");
  SampleCommand& sample = *new SampleCommand(GetDocument());
  Element* body = GetDocument().body();
  Node* text = body->lastChild();
  body->setAttribute(HTMLNames::contenteditableAttr, "true");
  GetDocument().UpdateStyleAndLayout();

  EditingState editing_state;
  sample.MoveParagraphContentsToNewBlockIfNecessary(Position(text, 0),
                                                    &editing_state);
  EXPECT_TRUE(editing_state.IsAborted());
  EXPECT_EQ(
      "<div><br></div>"
      "<style>div{-webkit-user-modify:read-only;user-select:none;}</style>"
      "foo",
      body->InnerHTMLAsString());
}

TEST_F(CompositeEditCommandTest, InsertNodeOnDisconnectedParent) {
  SetBodyContent("<p><b></b></p>");
  SampleCommand& sample = *new SampleCommand(GetDocument());
  Node* insert_child = GetDocument().QuerySelector("b");
  Element* ref_child = GetDocument().QuerySelector("p");
  ref_child->remove();
  EditingState editing_state;
  // editing state should abort here.
  sample.InsertNodeBefore(insert_child, ref_child, &editing_state);
  EXPECT_TRUE(editing_state.IsAborted());
}

}  // namespace blink
