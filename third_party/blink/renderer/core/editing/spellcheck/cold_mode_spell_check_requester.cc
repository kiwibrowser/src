// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/spellcheck/cold_mode_spell_check_requester.h"

#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/idle_deadline.h"
#include "third_party/blink/renderer/core/editing/editing_utilities.h"
#include "third_party/blink/renderer/core/editing/ephemeral_range.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/iterators/character_iterator.h"
#include "third_party/blink/renderer/core/editing/selection_template.h"
#include "third_party/blink/renderer/core/editing/spellcheck/spell_check_requester.h"
#include "third_party/blink/renderer/core/editing/spellcheck/spell_checker.h"
#include "third_party/blink/renderer/core/editing/visible_units.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"

namespace blink {

namespace {

const int kColdModeChunkSize = 16384;  // in UTF16 code units
const int kInvalidChunkIndex = -1;

}  // namespace

// static
ColdModeSpellCheckRequester* ColdModeSpellCheckRequester::Create(
    LocalFrame& frame) {
  return new ColdModeSpellCheckRequester(frame);
}

void ColdModeSpellCheckRequester::Trace(blink::Visitor* visitor) {
  visitor->Trace(frame_);
  visitor->Trace(root_editable_);
  visitor->Trace(last_chunk_end_);
}

ColdModeSpellCheckRequester::ColdModeSpellCheckRequester(LocalFrame& frame)
    : frame_(frame),
      last_chunk_index_(kInvalidChunkIndex),
      needs_more_invocation_for_testing_(false) {}

bool ColdModeSpellCheckRequester::FullyChecked() const {
  if (needs_more_invocation_for_testing_) {
    needs_more_invocation_for_testing_ = false;
    return false;
  }
  return !root_editable_ ||
         last_chunk_end_ == Position::LastPositionInNode(*root_editable_);
}

SpellCheckRequester& ColdModeSpellCheckRequester::GetSpellCheckRequester()
    const {
  return GetFrame().GetSpellChecker().GetSpellCheckRequester();
}

const Element* ColdModeSpellCheckRequester::CurrentFocusedEditable() const {
  const Position position =
      GetFrame().Selection().GetSelectionInDOMTree().Extent();
  if (position.IsNull())
    return nullptr;

  const ContainerNode* root = HighestEditableRoot(position);
  if (!root || !root->isConnected() || !root->IsElementNode())
    return nullptr;

  const Element* element = ToElement(root);
  if (!element->IsSpellCheckingEnabled() ||
      !SpellChecker::IsSpellCheckingEnabledAt(position))
    return nullptr;

  return element;
}

void ColdModeSpellCheckRequester::Invoke(IdleDeadline* deadline) {
  TRACE_EVENT0("blink", "ColdModeSpellCheckRequester::invoke");

  // TODO(xiaochengh): Figure out if this has any performance impact.
  GetFrame().GetDocument()->UpdateStyleAndLayout();

  const Element* current_focused = CurrentFocusedEditable();
  if (!current_focused) {
    ClearProgress();
    return;
  }

  if (root_editable_ != current_focused) {
    root_editable_ = current_focused;
    last_chunk_index_ = 0;
    last_chunk_end_ = Position::FirstPositionInNode(*root_editable_);
  }

  while (!FullyChecked() && deadline->timeRemaining() > 0)
    RequestCheckingForNextChunk();
}

void ColdModeSpellCheckRequester::ClearProgress() {
  root_editable_ = nullptr;
  last_chunk_index_ = kInvalidChunkIndex;
  last_chunk_end_ = Position();
}

void ColdModeSpellCheckRequester::RequestCheckingForNextChunk() {
  DCHECK(root_editable_);

  const int chunk_index = last_chunk_index_;
  const Position chunk_start = last_chunk_end_;
  const Position chunk_end =
      CalculateCharacterSubrange(
          EphemeralRange(chunk_start,
                         Position::LastPositionInNode(*root_editable_)),
          0, kColdModeChunkSize)
          .EndPosition();
  const EphemeralRange chunk_range(chunk_start, chunk_end);
  const EphemeralRange check_range = ExpandEndToSentenceBoundary(chunk_range);

  if (!GetSpellCheckRequester().RequestCheckingFor(check_range, chunk_index)) {
    // Fully checked.
    last_chunk_end_ = Position::LastPositionInNode(*root_editable_);
    return;
  }

  last_chunk_index_ = chunk_index;
  last_chunk_end_ = check_range.EndPosition();
}

}  // namespace blink
