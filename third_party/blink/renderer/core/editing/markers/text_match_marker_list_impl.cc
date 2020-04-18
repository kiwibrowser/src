// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/markers/text_match_marker_list_impl.h"

#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/dom/range.h"
#include "third_party/blink/renderer/core/editing/ephemeral_range.h"
#include "third_party/blink/renderer/core/editing/markers/sorted_document_marker_list_editor.h"
#include "third_party/blink/renderer/core/editing/markers/text_match_marker.h"
#include "third_party/blink/renderer/core/editing/visible_units.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"

namespace blink {

DocumentMarker::MarkerType TextMatchMarkerListImpl::MarkerType() const {
  return DocumentMarker::kTextMatch;
}

bool TextMatchMarkerListImpl::IsEmpty() const {
  return markers_.IsEmpty();
}

void TextMatchMarkerListImpl::Add(DocumentMarker* marker) {
  DCHECK_EQ(DocumentMarker::kTextMatch, marker->GetType());
  SortedDocumentMarkerListEditor::AddMarkerWithoutMergingOverlapping(&markers_,
                                                                     marker);
}

void TextMatchMarkerListImpl::Clear() {
  markers_.clear();
}

const HeapVector<Member<DocumentMarker>>& TextMatchMarkerListImpl::GetMarkers()
    const {
  return markers_;
}

DocumentMarker* TextMatchMarkerListImpl::FirstMarkerIntersectingRange(
    unsigned start_offset,
    unsigned end_offset) const {
  return SortedDocumentMarkerListEditor::FirstMarkerIntersectingRange(
      markers_, start_offset, end_offset);
}

HeapVector<Member<DocumentMarker>>
TextMatchMarkerListImpl::MarkersIntersectingRange(unsigned start_offset,
                                                  unsigned end_offset) const {
  return SortedDocumentMarkerListEditor::MarkersIntersectingRange(
      markers_, start_offset, end_offset);
}

bool TextMatchMarkerListImpl::MoveMarkers(int length,
                                          DocumentMarkerList* dst_list) {
  return SortedDocumentMarkerListEditor::MoveMarkers(&markers_, length,
                                                     dst_list);
}

bool TextMatchMarkerListImpl::RemoveMarkers(unsigned start_offset, int length) {
  return SortedDocumentMarkerListEditor::RemoveMarkers(&markers_, start_offset,
                                                       length);
}

bool TextMatchMarkerListImpl::ShiftMarkers(const String&,
                                           unsigned offset,
                                           unsigned old_length,
                                           unsigned new_length) {
  return SortedDocumentMarkerListEditor::ShiftMarkersContentDependent(
      &markers_, offset, old_length, new_length);
}

void TextMatchMarkerListImpl::Trace(blink::Visitor* visitor) {
  visitor->Trace(markers_);
  DocumentMarkerList::Trace(visitor);
}

static void UpdateMarkerLayoutRect(const Node& node, TextMatchMarker& marker) {
  const Position start_position(node, marker.StartOffset());
  const Position end_position(node, marker.EndOffset());
  EphemeralRange range(start_position, end_position);

  DCHECK(node.GetDocument().GetFrame());
  LocalFrameView* frame_view = node.GetDocument().GetFrame()->View();

  DCHECK(frame_view);
  marker.SetLayoutRect(
      frame_view->AbsoluteToDocument(LayoutRect(ComputeTextRect(range))));
}

Vector<IntRect> TextMatchMarkerListImpl::LayoutRects(const Node& node) const {
  Vector<IntRect> result;

  for (DocumentMarker* marker : markers_) {
    TextMatchMarker* const text_match_marker = ToTextMatchMarker(marker);
    if (!text_match_marker->IsValid())
      UpdateMarkerLayoutRect(node, *text_match_marker);
    if (!text_match_marker->IsRendered())
      continue;
    result.push_back(PixelSnappedIntRect(text_match_marker->GetLayoutRect()));
  }

  return result;
}

bool TextMatchMarkerListImpl::SetTextMatchMarkersActive(unsigned start_offset,
                                                        unsigned end_offset,
                                                        bool active) {
  bool doc_dirty = false;
  auto* const start = std::upper_bound(
      markers_.begin(), markers_.end(), start_offset,
      [](size_t start_offset, const Member<DocumentMarker>& marker) {
        return start_offset < marker->EndOffset();
      });
  for (auto* it = start; it != markers_.end(); ++it) {
    DocumentMarker& marker = **it;
    // Markers are returned in order, so stop if we are now past the specified
    // range.
    if (marker.StartOffset() >= end_offset)
      break;
    ToTextMatchMarker(marker).SetIsActiveMatch(active);
    doc_dirty = true;
  }
  return doc_dirty;
}

}  // namespace blink
