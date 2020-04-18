// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/accessibility/ax_action_data.h"

#include <set>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/accessibility/ax_enum_util.h"

using base::IntToString;

namespace ui {

AXActionData::AXActionData() = default;
AXActionData::AXActionData(const AXActionData& other) = default;
AXActionData::~AXActionData() = default;

namespace {

bool IsFlagSet(uint32_t bitfield, ax::mojom::ActionFlags flag) {
  return 0 != (bitfield & (1 << static_cast<uint32_t>(flag)));
}

}  // namespace

// Note that this includes an initial space character if nonempty, but
// that works fine because this is normally printed by
// ax::mojom::Action::ToString.
std::string AXActionData::ToString() const {
  std::string result = ui::ToString(action);

  if (target_node_id != -1)
    result += " target_node_id=" + IntToString(target_node_id);

  if (IsFlagSet(flags, ax::mojom::ActionFlags::kRequestImages))
    result += " flag_request_images";

  if (IsFlagSet(flags, ax::mojom::ActionFlags::kRequestInlineTextBoxes))
    result += " flag_request_inline_text_boxes";

  if (anchor_node_id != -1) {
    result += " anchor_node_id=" + IntToString(anchor_node_id);
    result += " anchor_offset=" + IntToString(anchor_offset);
  }
  if (focus_node_id != -1) {
    result += " focus_node_id=" + IntToString(focus_node_id);
    result += " focus_offset=" + IntToString(focus_offset);
  }

  return result;
}

}  // namespace ui
