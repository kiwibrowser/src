/**
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
 *           (C) 2008 Torch Mobile Inc. All rights reserved.
 *               (http://www.torchmobile.com/)
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

#include "third_party/blink/renderer/core/layout/layout_text_control_multi_line.h"

#include "third_party/blink/renderer/core/html/forms/html_text_area_element.h"
#include "third_party/blink/renderer/core/layout/hit_test_result.h"
#include "third_party/blink/renderer/core/layout/layout_theme.h"

namespace blink {

LayoutTextControlMultiLine::LayoutTextControlMultiLine(
    HTMLTextAreaElement* element)
    : LayoutTextControl(element) {
  DCHECK(element);
}

LayoutTextControlMultiLine::~LayoutTextControlMultiLine() = default;

bool LayoutTextControlMultiLine::NodeAtPoint(
    HitTestResult& result,
    const HitTestLocation& location_in_container,
    const LayoutPoint& accumulated_offset,
    HitTestAction hit_test_action) {
  if (!LayoutTextControl::NodeAtPoint(result, location_in_container,
                                      accumulated_offset, hit_test_action))
    return false;

  if (result.InnerNode() == GetNode() ||
      result.InnerNode() == InnerEditorElement())
    HitInnerEditorElement(result, location_in_container.Point(),
                          accumulated_offset);

  return true;
}

float LayoutTextControlMultiLine::GetAvgCharWidth(
    const AtomicString& family) const {
  // Match the default system font to the width of MS Shell Dlg, the default
  // font for textareas in Firefox, Safari Win and IE for some encodings (in
  // IE, the default font is encoding specific). 1229 is the avgCharWidth
  // value in the OS/2 table for Courier New.
  if (LayoutTheme::GetTheme().NeedsHackForTextControlWithFontFamily(family))
    return ScaleEmToUnits(1229);

  return LayoutTextControl::GetAvgCharWidth(family);
}

LayoutUnit LayoutTextControlMultiLine::PreferredContentLogicalWidth(
    float char_width) const {
  int factor = ToHTMLTextAreaElement(GetNode())->cols();
  return static_cast<LayoutUnit>(ceilf(char_width * factor)) +
         ScrollbarThickness();
}

LayoutUnit LayoutTextControlMultiLine::ComputeControlLogicalHeight(
    LayoutUnit line_height,
    LayoutUnit non_content_height) const {
  return line_height * ToHTMLTextAreaElement(GetNode())->rows() +
         non_content_height;
}

LayoutUnit LayoutTextControlMultiLine::BaselinePosition(
    FontBaseline baseline_type,
    bool first_line,
    LineDirectionMode direction,
    LinePositionMode line_position_mode) const {
  return LayoutBox::BaselinePosition(baseline_type, first_line, direction,
                                     line_position_mode);
}

LayoutObject* LayoutTextControlMultiLine::LayoutSpecialExcludedChild(
    bool relayout_children,
    SubtreeLayoutScope& layout_scope) {
  LayoutObject* placeholder_layout_object =
      LayoutTextControl::LayoutSpecialExcludedChild(relayout_children,
                                                    layout_scope);
  if (!placeholder_layout_object)
    return nullptr;
  if (!placeholder_layout_object->IsBox())
    return placeholder_layout_object;
  LayoutBox* placeholder_box = ToLayoutBox(placeholder_layout_object);
  placeholder_box->LayoutIfNeeded();
  placeholder_box->SetX(BorderLeft() + PaddingLeft());
  placeholder_box->SetY(BorderTop() + PaddingTop());
  return placeholder_layout_object;
}

}  // namespace blink
