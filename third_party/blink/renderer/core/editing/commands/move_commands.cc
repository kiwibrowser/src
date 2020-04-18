/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2009 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/commands/move_commands.h"

#include "third_party/blink/renderer/core/editing/editing_utilities.h"
#include "third_party/blink/renderer/core/editing/editor.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/selection_modifier.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/layout/layout_box.h"

namespace blink {

unsigned MoveCommands::VerticalScrollDistance(LocalFrame& frame) {
  const Element* const focused_element = frame.GetDocument()->FocusedElement();
  if (!focused_element)
    return 0;
  LayoutObject* const layout_object = focused_element->GetLayoutObject();
  if (!layout_object || !layout_object->IsBox())
    return 0;
  LayoutBox& layout_box = ToLayoutBox(*layout_object);
  const ComputedStyle* const style = layout_box.Style();
  if (!style)
    return 0;
  if (!(style->OverflowY() == EOverflow::kScroll ||
        style->OverflowY() == EOverflow::kAuto ||
        HasEditableStyle(*focused_element)))
    return 0;
  const ScrollableArea& scrollable_area =
      *frame.View()->LayoutViewportScrollableArea();
  const int height = std::min<int>(layout_box.ClientHeight().ToInt(),
                                   scrollable_area.VisibleHeight());
  return static_cast<unsigned>(
      max(max<int>(height * ScrollableArea::MinFractionToStepWhenPaging(),
                   height - scrollable_area.MaxOverlapBetweenPages()),
          1));
}

bool MoveCommands::ModifySelectionWithPageGranularity(
    LocalFrame& frame,
    SelectionModifyAlteration alter,
    unsigned vertical_distance,
    SelectionModifyVerticalDirection direction) {
  SelectionModifier selection_modifier(
      frame, frame.Selection().GetSelectionInDOMTree());
  selection_modifier.SetSelectionIsDirectional(
      frame.Selection().IsDirectional());
  if (!selection_modifier.ModifyWithPageGranularity(alter, vertical_distance,
                                                    direction)) {
    return false;
  }

  frame.Selection().SetSelection(
      selection_modifier.Selection().AsSelection(),
      SetSelectionOptions::Builder()
          .SetSetSelectionBy(SetSelectionBy::kUser)
          .SetShouldCloseTyping(true)
          .SetShouldClearTypingStyle(true)
          .SetCursorAlignOnScroll(alter == SelectionModifyAlteration::kMove
                                      ? CursorAlignOnScroll::kAlways
                                      : CursorAlignOnScroll::kIfNeeded)
          .SetIsDirectional(alter == SelectionModifyAlteration::kExtend ||
                            frame.GetEditor()
                                .Behavior()
                                .ShouldConsiderSelectionAsDirectional())
          .Build());
  return true;
}

bool MoveCommands::ExecuteMoveBackward(LocalFrame& frame,
                                       Event*,
                                       EditorCommandSource,
                                       const String&) {
  frame.Selection().Modify(SelectionModifyAlteration::kMove,
                           SelectionModifyDirection::kBackward,
                           TextGranularity::kCharacter, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveBackwardAndModifySelection(LocalFrame& frame,
                                                         Event*,
                                                         EditorCommandSource,
                                                         const String&) {
  frame.Selection().Modify(SelectionModifyAlteration::kExtend,
                           SelectionModifyDirection::kBackward,
                           TextGranularity::kCharacter, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveDown(LocalFrame& frame,
                                   Event*,
                                   EditorCommandSource,
                                   const String&) {
  return frame.Selection().Modify(
      SelectionModifyAlteration::kMove, SelectionModifyDirection::kForward,
      TextGranularity::kLine, SetSelectionBy::kUser);
}

bool MoveCommands::ExecuteMoveDownAndModifySelection(LocalFrame& frame,
                                                     Event*,
                                                     EditorCommandSource,
                                                     const String&) {
  frame.Selection().Modify(SelectionModifyAlteration::kExtend,
                           SelectionModifyDirection::kForward,
                           TextGranularity::kLine, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveForward(LocalFrame& frame,
                                      Event*,
                                      EditorCommandSource,
                                      const String&) {
  frame.Selection().Modify(SelectionModifyAlteration::kMove,
                           SelectionModifyDirection::kForward,
                           TextGranularity::kCharacter, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveForwardAndModifySelection(LocalFrame& frame,
                                                        Event*,
                                                        EditorCommandSource,
                                                        const String&) {
  frame.Selection().Modify(SelectionModifyAlteration::kExtend,
                           SelectionModifyDirection::kForward,
                           TextGranularity::kCharacter, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveLeft(LocalFrame& frame,
                                   Event*,
                                   EditorCommandSource,
                                   const String&) {
  return frame.Selection().Modify(
      SelectionModifyAlteration::kMove, SelectionModifyDirection::kLeft,
      TextGranularity::kCharacter, SetSelectionBy::kUser);
}

bool MoveCommands::ExecuteMoveLeftAndModifySelection(LocalFrame& frame,
                                                     Event*,
                                                     EditorCommandSource,
                                                     const String&) {
  frame.Selection().Modify(SelectionModifyAlteration::kExtend,
                           SelectionModifyDirection::kLeft,
                           TextGranularity::kCharacter, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMovePageDown(LocalFrame& frame,
                                       Event*,
                                       EditorCommandSource,
                                       const String&) {
  const unsigned distance = VerticalScrollDistance(frame);
  if (!distance)
    return false;
  return ModifySelectionWithPageGranularity(
      frame, SelectionModifyAlteration::kMove, distance,
      SelectionModifyVerticalDirection::kDown);
}

bool MoveCommands::ExecuteMovePageDownAndModifySelection(LocalFrame& frame,
                                                         Event*,
                                                         EditorCommandSource,
                                                         const String&) {
  const unsigned distance = VerticalScrollDistance(frame);
  if (!distance)
    return false;
  return ModifySelectionWithPageGranularity(
      frame, SelectionModifyAlteration::kExtend, distance,
      SelectionModifyVerticalDirection::kDown);
}

bool MoveCommands::ExecuteMovePageUp(LocalFrame& frame,
                                     Event*,
                                     EditorCommandSource,
                                     const String&) {
  const unsigned distance = VerticalScrollDistance(frame);
  if (!distance)
    return false;
  return ModifySelectionWithPageGranularity(
      frame, SelectionModifyAlteration::kMove, distance,
      SelectionModifyVerticalDirection::kUp);
}

bool MoveCommands::ExecuteMovePageUpAndModifySelection(LocalFrame& frame,
                                                       Event*,
                                                       EditorCommandSource,
                                                       const String&) {
  const unsigned distance = VerticalScrollDistance(frame);
  if (!distance)
    return false;
  return ModifySelectionWithPageGranularity(
      frame, SelectionModifyAlteration::kExtend, distance,
      SelectionModifyVerticalDirection::kUp);
}

bool MoveCommands::ExecuteMoveParagraphBackward(LocalFrame& frame,
                                                Event*,
                                                EditorCommandSource,
                                                const String&) {
  frame.Selection().Modify(SelectionModifyAlteration::kMove,
                           SelectionModifyDirection::kBackward,
                           TextGranularity::kParagraph, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveParagraphBackwardAndModifySelection(
    LocalFrame& frame,
    Event*,
    EditorCommandSource,
    const String&) {
  frame.Selection().Modify(SelectionModifyAlteration::kExtend,
                           SelectionModifyDirection::kBackward,
                           TextGranularity::kParagraph, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveParagraphForward(LocalFrame& frame,
                                               Event*,
                                               EditorCommandSource,
                                               const String&) {
  frame.Selection().Modify(SelectionModifyAlteration::kMove,
                           SelectionModifyDirection::kForward,
                           TextGranularity::kParagraph, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveParagraphForwardAndModifySelection(
    LocalFrame& frame,
    Event*,
    EditorCommandSource,
    const String&) {
  frame.Selection().Modify(SelectionModifyAlteration::kExtend,
                           SelectionModifyDirection::kForward,
                           TextGranularity::kParagraph, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveRight(LocalFrame& frame,
                                    Event*,
                                    EditorCommandSource,
                                    const String&) {
  return frame.Selection().Modify(
      SelectionModifyAlteration::kMove, SelectionModifyDirection::kRight,
      TextGranularity::kCharacter, SetSelectionBy::kUser);
}

bool MoveCommands::ExecuteMoveRightAndModifySelection(LocalFrame& frame,
                                                      Event*,
                                                      EditorCommandSource,
                                                      const String&) {
  frame.Selection().Modify(SelectionModifyAlteration::kExtend,
                           SelectionModifyDirection::kRight,
                           TextGranularity::kCharacter, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveToBeginningOfDocument(LocalFrame& frame,
                                                    Event*,
                                                    EditorCommandSource,
                                                    const String&) {
  frame.Selection().Modify(
      SelectionModifyAlteration::kMove, SelectionModifyDirection::kBackward,
      TextGranularity::kDocumentBoundary, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveToBeginningOfDocumentAndModifySelection(
    LocalFrame& frame,
    Event*,
    EditorCommandSource,
    const String&) {
  frame.Selection().Modify(
      SelectionModifyAlteration::kExtend, SelectionModifyDirection::kBackward,
      TextGranularity::kDocumentBoundary, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveToBeginningOfLine(LocalFrame& frame,
                                                Event*,
                                                EditorCommandSource,
                                                const String&) {
  frame.Selection().Modify(
      SelectionModifyAlteration::kMove, SelectionModifyDirection::kBackward,
      TextGranularity::kLineBoundary, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveToBeginningOfLineAndModifySelection(
    LocalFrame& frame,
    Event*,
    EditorCommandSource,
    const String&) {
  frame.Selection().Modify(
      SelectionModifyAlteration::kExtend, SelectionModifyDirection::kBackward,
      TextGranularity::kLineBoundary, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveToBeginningOfParagraph(LocalFrame& frame,
                                                     Event*,
                                                     EditorCommandSource,
                                                     const String&) {
  frame.Selection().Modify(
      SelectionModifyAlteration::kMove, SelectionModifyDirection::kBackward,
      TextGranularity::kParagraphBoundary, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveToBeginningOfParagraphAndModifySelection(
    LocalFrame& frame,
    Event*,
    EditorCommandSource,
    const String&) {
  frame.Selection().Modify(
      SelectionModifyAlteration::kExtend, SelectionModifyDirection::kBackward,
      TextGranularity::kParagraphBoundary, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveToBeginningOfSentence(LocalFrame& frame,
                                                    Event*,
                                                    EditorCommandSource,
                                                    const String&) {
  frame.Selection().Modify(
      SelectionModifyAlteration::kMove, SelectionModifyDirection::kBackward,
      TextGranularity::kSentenceBoundary, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveToBeginningOfSentenceAndModifySelection(
    LocalFrame& frame,
    Event*,
    EditorCommandSource,
    const String&) {
  frame.Selection().Modify(
      SelectionModifyAlteration::kExtend, SelectionModifyDirection::kBackward,
      TextGranularity::kSentenceBoundary, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveToEndOfDocument(LocalFrame& frame,
                                              Event*,
                                              EditorCommandSource,
                                              const String&) {
  frame.Selection().Modify(
      SelectionModifyAlteration::kMove, SelectionModifyDirection::kForward,
      TextGranularity::kDocumentBoundary, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveToEndOfDocumentAndModifySelection(
    LocalFrame& frame,
    Event*,
    EditorCommandSource,
    const String&) {
  frame.Selection().Modify(
      SelectionModifyAlteration::kExtend, SelectionModifyDirection::kForward,
      TextGranularity::kDocumentBoundary, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveToEndOfLine(LocalFrame& frame,
                                          Event*,
                                          EditorCommandSource,
                                          const String&) {
  frame.Selection().Modify(
      SelectionModifyAlteration::kMove, SelectionModifyDirection::kForward,
      TextGranularity::kLineBoundary, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveToEndOfLineAndModifySelection(LocalFrame& frame,
                                                            Event*,
                                                            EditorCommandSource,
                                                            const String&) {
  frame.Selection().Modify(
      SelectionModifyAlteration::kExtend, SelectionModifyDirection::kForward,
      TextGranularity::kLineBoundary, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveToEndOfParagraph(LocalFrame& frame,
                                               Event*,
                                               EditorCommandSource,
                                               const String&) {
  frame.Selection().Modify(
      SelectionModifyAlteration::kMove, SelectionModifyDirection::kForward,
      TextGranularity::kParagraphBoundary, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveToEndOfParagraphAndModifySelection(
    LocalFrame& frame,
    Event*,
    EditorCommandSource,
    const String&) {
  frame.Selection().Modify(
      SelectionModifyAlteration::kExtend, SelectionModifyDirection::kForward,
      TextGranularity::kParagraphBoundary, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveToEndOfSentence(LocalFrame& frame,
                                              Event*,
                                              EditorCommandSource,
                                              const String&) {
  frame.Selection().Modify(
      SelectionModifyAlteration::kMove, SelectionModifyDirection::kForward,
      TextGranularity::kSentenceBoundary, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveToEndOfSentenceAndModifySelection(
    LocalFrame& frame,
    Event*,
    EditorCommandSource,
    const String&) {
  frame.Selection().Modify(
      SelectionModifyAlteration::kExtend, SelectionModifyDirection::kForward,
      TextGranularity::kSentenceBoundary, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveToLeftEndOfLine(LocalFrame& frame,
                                              Event*,
                                              EditorCommandSource,
                                              const String&) {
  frame.Selection().Modify(
      SelectionModifyAlteration::kMove, SelectionModifyDirection::kLeft,
      TextGranularity::kLineBoundary, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveToLeftEndOfLineAndModifySelection(
    LocalFrame& frame,
    Event*,
    EditorCommandSource,
    const String&) {
  frame.Selection().Modify(
      SelectionModifyAlteration::kExtend, SelectionModifyDirection::kLeft,
      TextGranularity::kLineBoundary, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveToRightEndOfLine(LocalFrame& frame,
                                               Event*,
                                               EditorCommandSource,
                                               const String&) {
  frame.Selection().Modify(
      SelectionModifyAlteration::kMove, SelectionModifyDirection::kRight,
      TextGranularity::kLineBoundary, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveToRightEndOfLineAndModifySelection(
    LocalFrame& frame,
    Event*,
    EditorCommandSource,
    const String&) {
  frame.Selection().Modify(
      SelectionModifyAlteration::kExtend, SelectionModifyDirection::kRight,
      TextGranularity::kLineBoundary, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveUp(LocalFrame& frame,
                                 Event*,
                                 EditorCommandSource,
                                 const String&) {
  return frame.Selection().Modify(
      SelectionModifyAlteration::kMove, SelectionModifyDirection::kBackward,
      TextGranularity::kLine, SetSelectionBy::kUser);
}

bool MoveCommands::ExecuteMoveUpAndModifySelection(LocalFrame& frame,
                                                   Event*,
                                                   EditorCommandSource,
                                                   const String&) {
  frame.Selection().Modify(SelectionModifyAlteration::kExtend,
                           SelectionModifyDirection::kBackward,
                           TextGranularity::kLine, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveWordBackward(LocalFrame& frame,
                                           Event*,
                                           EditorCommandSource,
                                           const String&) {
  frame.Selection().Modify(SelectionModifyAlteration::kMove,
                           SelectionModifyDirection::kBackward,
                           TextGranularity::kWord, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveWordBackwardAndModifySelection(
    LocalFrame& frame,
    Event*,
    EditorCommandSource,
    const String&) {
  frame.Selection().Modify(SelectionModifyAlteration::kExtend,
                           SelectionModifyDirection::kBackward,
                           TextGranularity::kWord, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveWordForward(LocalFrame& frame,
                                          Event*,
                                          EditorCommandSource,
                                          const String&) {
  frame.Selection().Modify(SelectionModifyAlteration::kMove,
                           SelectionModifyDirection::kForward,
                           TextGranularity::kWord, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveWordForwardAndModifySelection(LocalFrame& frame,
                                                            Event*,
                                                            EditorCommandSource,
                                                            const String&) {
  frame.Selection().Modify(SelectionModifyAlteration::kExtend,
                           SelectionModifyDirection::kForward,
                           TextGranularity::kWord, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveWordLeft(LocalFrame& frame,
                                       Event*,
                                       EditorCommandSource,
                                       const String&) {
  frame.Selection().Modify(SelectionModifyAlteration::kMove,
                           SelectionModifyDirection::kLeft,
                           TextGranularity::kWord, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveWordLeftAndModifySelection(LocalFrame& frame,
                                                         Event*,
                                                         EditorCommandSource,
                                                         const String&) {
  frame.Selection().Modify(SelectionModifyAlteration::kExtend,
                           SelectionModifyDirection::kLeft,
                           TextGranularity::kWord, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveWordRight(LocalFrame& frame,
                                        Event*,
                                        EditorCommandSource,
                                        const String&) {
  frame.Selection().Modify(SelectionModifyAlteration::kMove,
                           SelectionModifyDirection::kRight,
                           TextGranularity::kWord, SetSelectionBy::kUser);
  return true;
}

bool MoveCommands::ExecuteMoveWordRightAndModifySelection(LocalFrame& frame,
                                                          Event*,
                                                          EditorCommandSource,
                                                          const String&) {
  frame.Selection().Modify(SelectionModifyAlteration::kExtend,
                           SelectionModifyDirection::kRight,
                           TextGranularity::kWord, SetSelectionBy::kUser);
  return true;
}

}  // namespace blink
