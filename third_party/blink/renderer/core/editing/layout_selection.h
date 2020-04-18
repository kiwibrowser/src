/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2006 Apple Computer, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_LAYOUT_SELECTION_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_LAYOUT_SELECTION_H_

#include "base/optional.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/editing/forward.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class IntRect;
class LayoutObject;
class NGPaintFragment;
class FrameSelection;
struct LayoutSelectionStatus;
// This class represents a selection range in layout tree for painting and
// paint invalidation.
// The current selection to be painted is represented as 2 pairs of
// (LayoutObject, offset).
// 2 LayoutObjects are only valid for |Text| node without 'transform' or
// 'first-letter'.
// TODO(editing-dev): Clarify the meaning of "offset".
// editing/ passes them as offsets in the DOM tree but layout uses them as
// offset in the layout tree. This doesn't work in the cases of
// CSS first-letter or character transform. See crbug.com/17528.
class SelectionPaintRange {
  DISALLOW_NEW();

 public:
  class Iterator
      : public std::iterator<std::input_iterator_tag, LayoutObject*> {
   public:
    explicit Iterator(const SelectionPaintRange*);
    Iterator(const Iterator&) = default;
    bool operator==(const Iterator& other) const {
      return current_ == other.current_;
    }
    bool operator!=(const Iterator& other) const { return !operator==(other); }
    Iterator& operator++();
    LayoutObject* operator*() const;

   private:
    LayoutObject* current_;
    const LayoutObject* stop_;
  };
  Iterator begin() const { return Iterator(this); };
  Iterator end() const { return Iterator(nullptr); };

  SelectionPaintRange() = default;
  SelectionPaintRange(LayoutObject* start_layout_object,
                      base::Optional<unsigned> start_offset,
                      LayoutObject* end_layout_object,
                      base::Optional<unsigned> end_offset);

  bool operator==(const SelectionPaintRange& other) const;

  LayoutObject* StartLayoutObject() const;
  base::Optional<unsigned> StartOffset() const;
  LayoutObject* EndLayoutObject() const;
  base::Optional<unsigned> EndOffset() const;

  bool IsNull() const { return !start_layout_object_; }

 private:
  LayoutObject* start_layout_object_ = nullptr;
  base::Optional<unsigned> start_offset_ = base::nullopt;
  LayoutObject* end_layout_object_ = nullptr;
  base::Optional<unsigned> end_offset_ = base::nullopt;
};

class LayoutSelection final : public GarbageCollected<LayoutSelection> {
 public:
  static LayoutSelection* Create(FrameSelection& frame_selection) {
    return new LayoutSelection(frame_selection);
  }

  bool HasPendingSelection() const { return has_pending_selection_; }
  void SetHasPendingSelection();
  void Commit();

  IntRect AbsoluteSelectionBounds();
  void InvalidatePaintForSelection();

  void ClearSelection();
  base::Optional<unsigned> SelectionStart() const;
  base::Optional<unsigned> SelectionEnd() const;
  LayoutSelectionStatus ComputeSelectionStatus(const NGPaintFragment&) const;

  void OnDocumentShutdown();

  void Trace(blink::Visitor*);

 private:
  LayoutSelection(FrameSelection&);

  Member<FrameSelection> frame_selection_;
  bool has_pending_selection_ : 1;

  SelectionPaintRange paint_range_;
};

void CORE_EXPORT PrintLayoutObjectForSelection(std::ostream&, LayoutObject*);
#ifndef NDEBUG
void ShowLayoutObjectForSelection(LayoutObject*);
#endif

}  // namespace blink

#endif
