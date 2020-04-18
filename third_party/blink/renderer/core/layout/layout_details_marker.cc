/*
 * Copyright (C) 2010, 2011 Nokia Corporation and/or its subsidiary(-ies)
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

#include "third_party/blink/renderer/core/layout/layout_details_marker.h"

#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/paint/details_marker_painter.h"

namespace blink {

using namespace HTMLNames;

LayoutDetailsMarker::LayoutDetailsMarker(Element* element)
    : LayoutBlockFlow(element) {}

LayoutDetailsMarker::Orientation LayoutDetailsMarker::GetOrientation() const {
  switch (Style()->GetWritingMode()) {
    case WritingMode::kHorizontalTb:
      if (Style()->IsLeftToRightDirection())
        return IsOpen() ? kDown : kRight;
      return IsOpen() ? kDown : kLeft;
    case WritingMode::kVerticalRl:
      if (Style()->IsLeftToRightDirection())
        return IsOpen() ? kLeft : kDown;
      return IsOpen() ? kLeft : kUp;
    case WritingMode::kVerticalLr:
      if (Style()->IsLeftToRightDirection())
        return IsOpen() ? kRight : kDown;
      return IsOpen() ? kRight : kUp;
    // TODO(layout-dev): Sideways-lr and sideways-rl are not yet supported.
    default:
      break;
  }
  NOTREACHED();
  return kRight;
}

void LayoutDetailsMarker::Paint(const PaintInfo& paint_info,
                                const LayoutPoint& paint_offset) const {
  DetailsMarkerPainter(*this).Paint(paint_info, paint_offset);
}

bool LayoutDetailsMarker::IsOpen() const {
  for (LayoutObject* layout_object = Parent(); layout_object;
       layout_object = layout_object->Parent()) {
    if (!layout_object->GetNode())
      continue;
    if (IsHTMLDetailsElement(*layout_object->GetNode()))
      return !ToElement(layout_object->GetNode())
                  ->getAttribute(openAttr)
                  .IsNull();
    if (IsHTMLInputElement(*layout_object->GetNode()))
      return true;
  }

  return false;
}

}  // namespace blink
