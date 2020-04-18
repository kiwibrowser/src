// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ACCESSIBILITY_MOJOM_AX_TREE_DATA_MOJOM_TRAITS_H_
#define UI_ACCESSIBILITY_MOJOM_AX_TREE_DATA_MOJOM_TRAITS_H_

#include "ui/accessibility/ax_tree_data.h"
#include "ui/accessibility/mojom/ax_tree_data.mojom-shared.h"

namespace mojo {

template <>
struct StructTraits<ax::mojom::AXTreeDataDataView, ui::AXTreeData> {
  static int32_t tree_id(const ui::AXTreeData& p) { return p.tree_id; }
  static int32_t parent_tree_id(const ui::AXTreeData& p) {
    return p.parent_tree_id;
  }
  static int32_t focused_tree_id(const ui::AXTreeData& p) {
    return p.focused_tree_id;
  }
  static std::string doctype(const ui::AXTreeData& p) { return p.doctype; }
  static bool loaded(const ui::AXTreeData& p) { return p.loaded; }
  static float loading_progress(const ui::AXTreeData& p) {
    return p.loading_progress;
  }
  static std::string mimetype(const ui::AXTreeData& p) { return p.mimetype; }
  static std::string title(const ui::AXTreeData& p) { return p.title; }
  static std::string url(const ui::AXTreeData& p) { return p.url; }
  static int32_t focus_id(const ui::AXTreeData& p) { return p.focus_id; }
  static int32_t sel_anchor_object_id(const ui::AXTreeData& p) {
    return p.sel_anchor_object_id;
  }
  static int32_t sel_anchor_offset(const ui::AXTreeData& p) {
    return p.sel_anchor_offset;
  }
  static ax::mojom::TextAffinity sel_anchor_affinity(const ui::AXTreeData& p) {
    return p.sel_anchor_affinity;
  }
  static int32_t sel_focus_object_id(const ui::AXTreeData& p) {
    return p.sel_focus_object_id;
  }
  static int32_t sel_focus_offset(const ui::AXTreeData& p) {
    return p.sel_focus_offset;
  }
  static ax::mojom::TextAffinity sel_focus_affinity(const ui::AXTreeData& p) {
    return p.sel_focus_affinity;
  }

  static bool Read(ax::mojom::AXTreeDataDataView data, ui::AXTreeData* out);
};

}  // namespace mojo

#endif  // UI_ACCESSIBILITY_MOJOM_AX_TREE_DATA_MOJOM_TRAITS_H_
