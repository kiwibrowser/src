/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2003, 2004, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc.
 *               All right reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2013 Adobe Systems Incorporated.
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

#include "third_party/blink/renderer/core/layout/line/breaking_context_inline_headers.h"

namespace blink {

InlineIterator BreakingContext::HandleEndOfLine() {
  if (line_break_ == resolver_.GetPosition() &&
      (!line_break_.GetLineLayoutItem() ||
       !line_break_.GetLineLayoutItem().IsBR())) {
    // we just add as much as possible
    if (block_style_->WhiteSpace() == EWhiteSpace::kPre && !current_.Offset()) {
      line_break_.MoveTo(last_object_,
                         last_object_.IsText() ? last_object_.length() : 0);
    } else if (line_break_.GetLineLayoutItem()) {
      // Don't ever break in the middle of a word if we can help it.
      // There's no room at all. We just have to be on this line,
      // even though we'll spill out.
      line_break_.MoveTo(current_.GetLineLayoutItem(), current_.Offset());
    }
  }

  // FIXME Bug 100049: We do not need to consume input in a multi-segment line
  // unless no segment will.
  if (line_break_ == resolver_.GetPosition())
    line_break_.Increment();

  // Sanity check our midpoints.
  line_midpoint_state_.CheckMidpoints(line_break_);

  trailing_objects_.UpdateMidpointsForTrailingObjects(
      line_midpoint_state_, line_break_, TrailingObjects::kCollapseFirstSpace);

  // We might have made lineBreak an iterator that points past the end
  // of the object. Do this adjustment to make it point to the start
  // of the next object instead to avoid confusing the rest of the
  // code.
  if (line_break_.Offset()) {
    // This loop enforces the invariant that line breaks should never point
    // at an empty inline. See http://crbug.com/305904.
    do {
      line_break_.SetOffset(line_break_.Offset() - 1);
      line_break_.Increment();
    } while (!line_break_.AtEnd() &&
             IsEmptyInline(line_break_.GetLineLayoutItem()));
  }

  return line_break_;
}

}  // namespace blink
