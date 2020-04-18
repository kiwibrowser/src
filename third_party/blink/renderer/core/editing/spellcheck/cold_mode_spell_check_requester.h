// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_SPELLCHECK_COLD_MODE_SPELL_CHECK_REQUESTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_SPELLCHECK_COLD_MODE_SPELL_CHECK_REQUESTER_H_

#include "third_party/blink/renderer/core/editing/forward.h"
#include "third_party/blink/renderer/core/editing/position.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class Element;
class LocalFrame;
class IdleDeadline;
class SpellCheckRequester;

// This class is only supposed to be used by IdleSpellCheckCallback in cold mode
// invocation. Not to be confused with SpellCheckRequester.
// The class iteratively checks the editing host currently focused when the
// document is idle.
class ColdModeSpellCheckRequester
    : public GarbageCollected<ColdModeSpellCheckRequester> {
 public:
  static ColdModeSpellCheckRequester* Create(LocalFrame&);

  void SetNeedsMoreInvocationForTesting() {
    needs_more_invocation_for_testing_ = true;
  }

  void Invoke(IdleDeadline*);

  void ClearProgress();
  bool FullyChecked() const;

  void Trace(blink::Visitor*);

 private:
  explicit ColdModeSpellCheckRequester(LocalFrame&);

  LocalFrame& GetFrame() const { return *frame_; }
  SpellCheckRequester& GetSpellCheckRequester() const;

  const Element* CurrentFocusedEditable() const;

  void RequestCheckingForNextChunk();

  // The LocalFrame this cold mode checker belongs to.
  const Member<LocalFrame> frame_;

  // The root editable element checked in the last invocation. |nullptr| if not
  // invoked yet or nothing was checked in the last invocation.
  Member<const Element> root_editable_;

  // If the last invocation checked a chunk of text, the id and ending position
  // of the chunk; Otherwise, |kInvalidChunkIndex| and null.
  int last_chunk_index_;
  Position last_chunk_end_;

  // A test-only flag for forcing lifecycle advancing.
  mutable bool needs_more_invocation_for_testing_;

  DISALLOW_COPY_AND_ASSIGN(ColdModeSpellCheckRequester);
};
}

#endif
