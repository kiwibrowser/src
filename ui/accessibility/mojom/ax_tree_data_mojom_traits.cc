// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/accessibility/mojom/ax_tree_data_mojom_traits.h"

namespace mojo {

// static
bool StructTraits<ax::mojom::AXTreeDataDataView, ui::AXTreeData>::Read(
    ax::mojom::AXTreeDataDataView data,
    ui::AXTreeData* out) {
  out->tree_id = data.tree_id();
  out->parent_tree_id = data.parent_tree_id();
  out->focused_tree_id = data.focused_tree_id();
  if (!data.ReadDoctype(&out->doctype))
    return false;
  out->loaded = data.loaded();
  out->loading_progress = data.loading_progress();
  if (!data.ReadMimetype(&out->mimetype))
    return false;
  if (!data.ReadTitle(&out->title))
    return false;
  if (!data.ReadUrl(&out->url))
    return false;
  out->focus_id = data.focus_id();
  out->sel_anchor_object_id = data.sel_anchor_object_id();
  out->sel_anchor_offset = data.sel_anchor_offset();
  out->sel_anchor_affinity = data.sel_anchor_affinity();
  out->sel_focus_object_id = data.sel_focus_object_id();
  out->sel_focus_offset = data.sel_focus_offset();
  out->sel_focus_affinity = data.sel_focus_affinity();
  return true;
}

}  // namespace mojo
