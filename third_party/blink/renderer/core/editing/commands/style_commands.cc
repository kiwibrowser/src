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

#include "third_party/blink/renderer/core/editing/commands/style_commands.h"

#include "third_party/blink/renderer/core/css/css_computed_style_declaration.h"
#include "third_party/blink/renderer/core/css/css_identifier_value.h"
#include "third_party/blink/renderer/core/css/css_property_value_set.h"
#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/editing/commands/apply_style_command.h"
#include "third_party/blink/renderer/core/editing/editing_style_utilities.h"
#include "third_party/blink/renderer/core/editing/editing_tri_state.h"
#include "third_party/blink/renderer/core/editing/editing_utilities.h"
#include "third_party/blink/renderer/core/editing/editor.h"
#include "third_party/blink/renderer/core/editing/ephemeral_range.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/visible_position.h"
#include "third_party/blink/renderer/core/editing/writing_direction.h"
#include "third_party/blink/renderer/core/html/html_font_element.h"

namespace blink {

void StyleCommands::ApplyStyle(LocalFrame& frame,
                               CSSPropertyValueSet* style,
                               InputEvent::InputType input_type) {
  const VisibleSelection& selection =
      frame.Selection().ComputeVisibleSelectionInDOMTreeDeprecated();
  if (selection.IsNone())
    return;
  if (selection.IsCaret()) {
    frame.GetEditor().ComputeAndSetTypingStyle(style, input_type);
    return;
  }
  DCHECK(selection.IsRange()) << selection;
  if (!style)
    return;
  DCHECK(frame.GetDocument());
  ApplyStyleCommand::Create(*frame.GetDocument(), EditingStyle::Create(style),
                            input_type)
      ->Apply();
}

void StyleCommands::ApplyStyleToSelection(LocalFrame& frame,
                                          CSSPropertyValueSet* style,
                                          InputEvent::InputType input_type) {
  if (!style || style->IsEmpty() || !frame.GetEditor().CanEditRichly())
    return;

  ApplyStyle(frame, style, input_type);
}

bool StyleCommands::ApplyCommandToFrame(LocalFrame& frame,
                                        EditorCommandSource source,
                                        InputEvent::InputType input_type,
                                        CSSPropertyValueSet* style) {
  // TODO(editnig-dev): We don't call shouldApplyStyle when the source is DOM;
  // is there a good reason for that?
  switch (source) {
    case EditorCommandSource::kMenuOrKeyBinding:
      ApplyStyleToSelection(frame, style, input_type);
      return true;
    case EditorCommandSource::kDOM:
      ApplyStyle(frame, style, input_type);
      return true;
  }
  NOTREACHED();
  return false;
}

bool StyleCommands::ExecuteApplyStyle(LocalFrame& frame,
                                      EditorCommandSource source,
                                      InputEvent::InputType input_type,
                                      CSSPropertyID property_id,
                                      const String& property_value) {
  DCHECK(frame.GetDocument());
  MutableCSSPropertyValueSet* const style =
      MutableCSSPropertyValueSet::Create(kHTMLQuirksMode);
  style->SetProperty(property_id, property_value, /* important */ false,
                     frame.GetDocument()->GetSecureContextMode());
  return ApplyCommandToFrame(frame, source, input_type, style);
}

bool StyleCommands::ExecuteApplyStyle(LocalFrame& frame,
                                      EditorCommandSource source,
                                      InputEvent::InputType input_type,
                                      CSSPropertyID property_id,
                                      CSSValueID property_value) {
  MutableCSSPropertyValueSet* const style =
      MutableCSSPropertyValueSet::Create(kHTMLQuirksMode);
  style->SetProperty(property_id, property_value);
  return ApplyCommandToFrame(frame, source, input_type, style);
}

bool StyleCommands::ExecuteBackColor(LocalFrame& frame,
                                     Event*,
                                     EditorCommandSource source,
                                     const String& value) {
  return ExecuteApplyStyle(frame, source, InputEvent::InputType::kNone,
                           CSSPropertyBackgroundColor, value);
}

bool StyleCommands::ExecuteForeColor(LocalFrame& frame,
                                     Event*,
                                     EditorCommandSource source,
                                     const String& value) {
  return ExecuteApplyStyle(frame, source, InputEvent::InputType::kNone,
                           CSSPropertyColor, value);
}

bool StyleCommands::ExecuteFontName(LocalFrame& frame,
                                    Event*,
                                    EditorCommandSource source,
                                    const String& value) {
  return ExecuteApplyStyle(frame, source, InputEvent::InputType::kNone,
                           CSSPropertyFontFamily, value);
}

bool StyleCommands::ExecuteFontSize(LocalFrame& frame,
                                    Event*,
                                    EditorCommandSource source,
                                    const String& value) {
  CSSValueID size;
  if (!HTMLFontElement::CssValueFromFontSizeNumber(value, size))
    return false;
  return ExecuteApplyStyle(frame, source, InputEvent::InputType::kNone,
                           CSSPropertyFontSize, size);
}

bool StyleCommands::ExecuteFontSizeDelta(LocalFrame& frame,
                                         Event*,
                                         EditorCommandSource source,
                                         const String& value) {
  return ExecuteApplyStyle(frame, source, InputEvent::InputType::kNone,
                           CSSPropertyWebkitFontSizeDelta, value);
}

bool StyleCommands::ExecuteMakeTextWritingDirectionLeftToRight(
    LocalFrame& frame,
    Event*,
    EditorCommandSource,
    const String&) {
  MutableCSSPropertyValueSet* const style =
      MutableCSSPropertyValueSet::Create(kHTMLQuirksMode);
  style->SetProperty(CSSPropertyUnicodeBidi, CSSValueIsolate);
  style->SetProperty(CSSPropertyDirection, CSSValueLtr);
  ApplyStyle(frame, style, InputEvent::InputType::kFormatSetBlockTextDirection);
  return true;
}

bool StyleCommands::ExecuteMakeTextWritingDirectionNatural(LocalFrame& frame,
                                                           Event*,
                                                           EditorCommandSource,
                                                           const String&) {
  MutableCSSPropertyValueSet* const style =
      MutableCSSPropertyValueSet::Create(kHTMLQuirksMode);
  style->SetProperty(CSSPropertyUnicodeBidi, CSSValueNormal);
  ApplyStyle(frame, style, InputEvent::InputType::kFormatSetBlockTextDirection);
  return true;
}

bool StyleCommands::ExecuteMakeTextWritingDirectionRightToLeft(
    LocalFrame& frame,
    Event*,
    EditorCommandSource,
    const String&) {
  MutableCSSPropertyValueSet* const style =
      MutableCSSPropertyValueSet::Create(kHTMLQuirksMode);
  style->SetProperty(CSSPropertyUnicodeBidi, CSSValueIsolate);
  style->SetProperty(CSSPropertyDirection, CSSValueRtl);
  ApplyStyle(frame, style, InputEvent::InputType::kFormatSetBlockTextDirection);
  return true;
}

bool StyleCommands::SelectionStartHasStyle(LocalFrame& frame,
                                           CSSPropertyID property_id,
                                           const String& value) {
  const SecureContextMode secure_context_mode =
      frame.GetDocument()->GetSecureContextMode();

  EditingStyle* const style_to_check =
      EditingStyle::Create(property_id, value, secure_context_mode);
  EditingStyle* const style_at_start =
      EditingStyleUtilities::CreateStyleAtSelectionStart(
          frame.Selection().ComputeVisibleSelectionInDOMTreeDeprecated(),
          property_id == CSSPropertyBackgroundColor, style_to_check->Style());
  return style_to_check->TriStateOfStyle(style_at_start, secure_context_mode) !=
         EditingTriState::kFalse;
}

bool StyleCommands::ExecuteToggleStyle(LocalFrame& frame,
                                       EditorCommandSource source,
                                       InputEvent::InputType input_type,
                                       CSSPropertyID property_id,
                                       const char* off_value,
                                       const char* on_value) {
  // Style is considered present when
  // Mac: present at the beginning of selection
  // other: present throughout the selection
  const bool style_is_present =
      frame.GetEditor().Behavior().ShouldToggleStyleBasedOnStartOfSelection()
          ? SelectionStartHasStyle(frame, property_id, on_value)
          : EditingStyle::SelectionHasStyle(frame, property_id, on_value) ==
                EditingTriState::kTrue;

  EditingStyle* const style =
      EditingStyle::Create(property_id, style_is_present ? off_value : on_value,
                           frame.GetDocument()->GetSecureContextMode());
  return ApplyCommandToFrame(frame, source, input_type, style->Style());
}

bool StyleCommands::ExecuteToggleBold(LocalFrame& frame,
                                      Event*,
                                      EditorCommandSource source,
                                      const String&) {
  return ExecuteToggleStyle(frame, source, InputEvent::InputType::kFormatBold,
                            CSSPropertyFontWeight, "normal", "bold");
}

bool StyleCommands::ExecuteToggleItalic(LocalFrame& frame,
                                        Event*,
                                        EditorCommandSource source,
                                        const String&) {
  return ExecuteToggleStyle(frame, source, InputEvent::InputType::kFormatItalic,
                            CSSPropertyFontStyle, "normal", "italic");
}

bool StyleCommands::ExecuteSubscript(LocalFrame& frame,
                                     Event*,
                                     EditorCommandSource source,
                                     const String&) {
  return ExecuteToggleStyle(frame, source,
                            InputEvent::InputType::kFormatSubscript,
                            CSSPropertyVerticalAlign, "baseline", "sub");
}

bool StyleCommands::ExecuteSuperscript(LocalFrame& frame,
                                       Event*,
                                       EditorCommandSource source,
                                       const String&) {
  return ExecuteToggleStyle(frame, source,
                            InputEvent::InputType::kFormatSuperscript,
                            CSSPropertyVerticalAlign, "baseline", "super");
}

bool StyleCommands::ExecuteUnscript(LocalFrame& frame,
                                    Event*,
                                    EditorCommandSource source,
                                    const String&) {
  return ExecuteApplyStyle(frame, source, InputEvent::InputType::kNone,
                           CSSPropertyVerticalAlign, "baseline");
}

String StyleCommands::ComputeToggleStyleInList(EditingStyle& selection_style,
                                               CSSPropertyID property_id,
                                               const CSSValue& value) {
  const CSSValue& selected_css_value =
      *selection_style.Style()->GetPropertyCSSValue(property_id);
  if (selected_css_value.IsValueList()) {
    CSSValueList& selected_css_value_list =
        *ToCSSValueList(selected_css_value).Copy();
    if (!selected_css_value_list.RemoveAll(value))
      selected_css_value_list.Append(value);
    if (selected_css_value_list.length())
      return selected_css_value_list.CssText();
  } else if (selected_css_value.CssText() == "none") {
    return value.CssText();
  }
  return "none";
}

bool StyleCommands::ExecuteToggleStyleInList(LocalFrame& frame,
                                             EditorCommandSource source,
                                             InputEvent::InputType input_type,
                                             CSSPropertyID property_id,
                                             const CSSValue& value) {
  EditingStyle* const selection_style =
      EditingStyleUtilities::CreateStyleAtSelectionStart(
          frame.Selection().ComputeVisibleSelectionInDOMTree());
  if (!selection_style || !selection_style->Style())
    return false;

  const String new_style =
      ComputeToggleStyleInList(*selection_style, property_id, value);

  // TODO(editnig-dev): We shouldn't be having to convert new style into text.
  // We should have setPropertyCSSValue.
  MutableCSSPropertyValueSet* const new_mutable_style =
      MutableCSSPropertyValueSet::Create(kHTMLQuirksMode);
  new_mutable_style->SetProperty(property_id, new_style, /* important */ false,
                                 frame.GetDocument()->GetSecureContextMode());
  return ApplyCommandToFrame(frame, source, input_type, new_mutable_style);
}

bool StyleCommands::ExecuteStrikethrough(LocalFrame& frame,
                                         Event*,
                                         EditorCommandSource source,
                                         const String&) {
  const CSSIdentifierValue& line_through =
      *CSSIdentifierValue::Create(CSSValueLineThrough);
  return ExecuteToggleStyleInList(
      frame, source, InputEvent::InputType::kFormatStrikeThrough,
      CSSPropertyWebkitTextDecorationsInEffect, line_through);
}

bool StyleCommands::ExecuteUnderline(LocalFrame& frame,
                                     Event*,
                                     EditorCommandSource source,
                                     const String&) {
  const CSSIdentifierValue& underline =
      *CSSIdentifierValue::Create(CSSValueUnderline);
  return ExecuteToggleStyleInList(
      frame, source, InputEvent::InputType::kFormatUnderline,
      CSSPropertyWebkitTextDecorationsInEffect, underline);
}

bool StyleCommands::ExecuteStyleWithCSS(LocalFrame& frame,
                                        Event*,
                                        EditorCommandSource,
                                        const String& value) {
  frame.GetEditor().SetShouldStyleWithCSS(
      !DeprecatedEqualIgnoringCase(value, "false"));
  return true;
}

bool StyleCommands::ExecuteUseCSS(LocalFrame& frame,
                                  Event*,
                                  EditorCommandSource,
                                  const String& value) {
  frame.GetEditor().SetShouldStyleWithCSS(
      DeprecatedEqualIgnoringCase(value, "false"));
  return true;
}

// State functions
EditingTriState StyleCommands::StateStyle(LocalFrame& frame,
                                          CSSPropertyID property_id,
                                          const char* desired_value) {
  frame.GetDocument()->UpdateStyleAndLayoutIgnorePendingStylesheets();
  if (frame.GetEditor().Behavior().ShouldToggleStyleBasedOnStartOfSelection()) {
    return SelectionStartHasStyle(frame, property_id, desired_value)
               ? EditingTriState::kTrue
               : EditingTriState::kFalse;
  }
  return EditingStyle::SelectionHasStyle(frame, property_id, desired_value);
}

EditingTriState StyleCommands::StateBold(LocalFrame& frame, Event*) {
  return StateStyle(frame, CSSPropertyFontWeight, "bold");
}

EditingTriState StyleCommands::StateItalic(LocalFrame& frame, Event*) {
  return StateStyle(frame, CSSPropertyFontStyle, "italic");
}

EditingTriState StyleCommands::StateStrikethrough(LocalFrame& frame, Event*) {
  return StateStyle(frame, CSSPropertyWebkitTextDecorationsInEffect,
                    "line-through");
}

EditingTriState StyleCommands::StateStyleWithCSS(LocalFrame& frame, Event*) {
  return frame.GetEditor().ShouldStyleWithCSS() ? EditingTriState::kTrue
                                                : EditingTriState::kFalse;
}

EditingTriState StyleCommands::StateSubscript(LocalFrame& frame, Event*) {
  return StateStyle(frame, CSSPropertyVerticalAlign, "sub");
}

EditingTriState StyleCommands::StateSuperscript(LocalFrame& frame, Event*) {
  return StateStyle(frame, CSSPropertyVerticalAlign, "super");
}

bool StyleCommands::IsUnicodeBidiNestedOrMultipleEmbeddings(
    CSSValueID value_id) {
  return value_id == CSSValueEmbed || value_id == CSSValueBidiOverride ||
         value_id == CSSValueWebkitIsolate ||
         value_id == CSSValueWebkitIsolateOverride ||
         value_id == CSSValueWebkitPlaintext || value_id == CSSValueIsolate ||
         value_id == CSSValueIsolateOverride || value_id == CSSValuePlaintext;
}

WritingDirection StyleCommands::TextDirectionForSelection(
    const VisibleSelection& selection,
    EditingStyle* typing_style,
    bool& has_nested_or_multiple_embeddings) {
  has_nested_or_multiple_embeddings = true;

  if (selection.IsNone())
    return WritingDirection::kNatural;

  const Position position = MostForwardCaretPosition(selection.Start());

  const Node* anchor_node = position.AnchorNode();
  if (!anchor_node)
    return WritingDirection::kNatural;

  Position end;
  if (selection.IsRange()) {
    end = MostBackwardCaretPosition(selection.End());

    DCHECK(end.GetDocument());
    const EphemeralRange caret_range(position.ParentAnchoredEquivalent(),
                                     end.ParentAnchoredEquivalent());
    for (Node& node : caret_range.Nodes()) {
      if (!node.IsStyledElement())
        continue;

      const CSSComputedStyleDeclaration& style =
          *CSSComputedStyleDeclaration::Create(&node);
      const CSSValue* unicode_bidi =
          style.GetPropertyCSSValue(GetCSSPropertyUnicodeBidi());
      if (!unicode_bidi || !unicode_bidi->IsIdentifierValue())
        continue;

      const CSSValueID unicode_bidi_value =
          ToCSSIdentifierValue(unicode_bidi)->GetValueID();
      if (IsUnicodeBidiNestedOrMultipleEmbeddings(unicode_bidi_value))
        return WritingDirection::kNatural;
    }
  }

  if (selection.IsCaret()) {
    WritingDirection direction;
    if (typing_style && typing_style->GetTextDirection(direction)) {
      has_nested_or_multiple_embeddings = false;
      return direction;
    }
    anchor_node = selection.VisibleStart().DeepEquivalent().AnchorNode();
  }
  DCHECK(anchor_node);

  // The selection is either a caret with no typing attributes or a range in
  // which no embedding is added, so just use the start position to decide.
  const Node* block = EnclosingBlock(anchor_node);
  WritingDirection found_direction = WritingDirection::kNatural;

  for (Node& runner : NodeTraversal::InclusiveAncestorsOf(*anchor_node)) {
    if (runner == block)
      break;
    if (!runner.IsStyledElement())
      continue;

    Element* element = &ToElement(runner);
    const CSSComputedStyleDeclaration& style =
        *CSSComputedStyleDeclaration::Create(element);
    const CSSValue* unicode_bidi =
        style.GetPropertyCSSValue(GetCSSPropertyUnicodeBidi());
    if (!unicode_bidi || !unicode_bidi->IsIdentifierValue())
      continue;

    const CSSValueID unicode_bidi_value =
        ToCSSIdentifierValue(unicode_bidi)->GetValueID();
    if (unicode_bidi_value == CSSValueNormal)
      continue;

    if (unicode_bidi_value == CSSValueBidiOverride)
      return WritingDirection::kNatural;

    DCHECK(EditingStyleUtilities::IsEmbedOrIsolate(unicode_bidi_value))
        << unicode_bidi_value;
    const CSSValue* direction =
        style.GetPropertyCSSValue(GetCSSPropertyDirection());
    if (!direction || !direction->IsIdentifierValue())
      continue;

    const int direction_value = ToCSSIdentifierValue(direction)->GetValueID();
    if (direction_value != CSSValueLtr && direction_value != CSSValueRtl)
      continue;

    if (found_direction != WritingDirection::kNatural)
      return WritingDirection::kNatural;

    // In the range case, make sure that the embedding element persists until
    // the end of the range.
    if (selection.IsRange() && !end.AnchorNode()->IsDescendantOf(element))
      return WritingDirection::kNatural;

    found_direction = direction_value == CSSValueLtr
                          ? WritingDirection::kLeftToRight
                          : WritingDirection::kRightToLeft;
  }
  has_nested_or_multiple_embeddings = false;
  return found_direction;
}

EditingTriState StyleCommands::StateTextWritingDirection(
    LocalFrame& frame,
    WritingDirection direction) {
  frame.GetDocument()->UpdateStyleAndLayoutIgnorePendingStylesheets();

  bool has_nested_or_multiple_embeddings;
  WritingDirection selection_direction = TextDirectionForSelection(
      frame.Selection().ComputeVisibleSelectionInDOMTreeDeprecated(),
      frame.GetEditor().TypingStyle(), has_nested_or_multiple_embeddings);
  // TODO(editnig-dev): We should be returning MixedTriState when
  // selectionDirection == direction && hasNestedOrMultipleEmbeddings
  return (selection_direction == direction &&
          !has_nested_or_multiple_embeddings)
             ? EditingTriState::kTrue
             : EditingTriState::kFalse;
}

EditingTriState StyleCommands::StateTextWritingDirectionLeftToRight(
    LocalFrame& frame,
    Event*) {
  return StateTextWritingDirection(frame, WritingDirection::kLeftToRight);
}

EditingTriState StyleCommands::StateTextWritingDirectionNatural(
    LocalFrame& frame,
    Event*) {
  return StateTextWritingDirection(frame, WritingDirection::kNatural);
}

EditingTriState StyleCommands::StateTextWritingDirectionRightToLeft(
    LocalFrame& frame,
    Event*) {
  return StateTextWritingDirection(frame, WritingDirection::kRightToLeft);
}

EditingTriState StyleCommands::StateUnderline(LocalFrame& frame, Event*) {
  return StateStyle(frame, CSSPropertyWebkitTextDecorationsInEffect,
                    "underline");
}

// Value functions
String StyleCommands::SelectionStartCSSPropertyValue(
    LocalFrame& frame,
    CSSPropertyID property_id) {
  EditingStyle* const selection_style =
      EditingStyleUtilities::CreateStyleAtSelectionStart(
          frame.Selection().ComputeVisibleSelectionInDOMTreeDeprecated(),
          property_id == CSSPropertyBackgroundColor);
  if (!selection_style || !selection_style->Style())
    return String();

  if (property_id == CSSPropertyFontSize)
    return String::Number(selection_style->LegacyFontSize(frame.GetDocument()));
  return selection_style->Style()->GetPropertyValue(property_id);
}

String StyleCommands::ValueStyle(LocalFrame& frame, CSSPropertyID property_id) {
  frame.GetDocument()->UpdateStyleAndLayoutIgnorePendingStylesheets();

  // TODO(editnig-dev): Rather than retrieving the style at the start of the
  // current selection, we should retrieve the style present throughout the
  // selection for non-Mac platforms.
  return SelectionStartCSSPropertyValue(frame, property_id);
}

String StyleCommands::ValueBackColor(const EditorInternalCommand&,
                                     LocalFrame& frame,
                                     Event*) {
  return ValueStyle(frame, CSSPropertyBackgroundColor);
}

String StyleCommands::ValueForeColor(const EditorInternalCommand&,
                                     LocalFrame& frame,
                                     Event*) {
  return ValueStyle(frame, CSSPropertyColor);
}

String StyleCommands::ValueFontName(const EditorInternalCommand&,
                                    LocalFrame& frame,
                                    Event*) {
  return ValueStyle(frame, CSSPropertyFontFamily);
}

String StyleCommands::ValueFontSize(const EditorInternalCommand&,
                                    LocalFrame& frame,
                                    Event*) {
  return ValueStyle(frame, CSSPropertyFontSize);
}

String StyleCommands::ValueFontSizeDelta(const EditorInternalCommand&,
                                         LocalFrame& frame,
                                         Event*) {
  return ValueStyle(frame, CSSPropertyWebkitFontSizeDelta);
}

}  // namespace blink
