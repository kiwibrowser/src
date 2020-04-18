// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_MARKERS_GRAMMAR_MARKER_LIST_IMPL_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_MARKERS_GRAMMAR_MARKER_LIST_IMPL_H_

#include "third_party/blink/renderer/core/editing/markers/spell_check_marker_list_impl.h"

namespace blink {

// This is the DocumentMarkerList implementation used to store Grammar markers.
class CORE_EXPORT GrammarMarkerListImpl final
    : public SpellCheckMarkerListImpl {
 public:
  GrammarMarkerListImpl() = default;

  DocumentMarker::MarkerType MarkerType() const final;

 private:
  DISALLOW_COPY_AND_ASSIGN(GrammarMarkerListImpl);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_MARKERS_GRAMMAR_MARKER_LIST_IMPL_H_
