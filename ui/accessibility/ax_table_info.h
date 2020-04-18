// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ACCESSIBILITY_AX_TABLE_INFO_H_
#define UI_ACCESSIBILITY_AX_TABLE_INFO_H_

#include <set>
#include <vector>

#include "base/containers/hash_tables.h"
#include "ui/accessibility/ax_export.h"

namespace ui {

class AXTree;
class AXNode;

// This helper class computes info about tables and grids in AXTrees.
class AX_EXPORT AXTableInfo {
 public:
  // Returns nullptr if the node is not a valid table or grid node.
  static AXTableInfo* Create(AXTree* tree, AXNode* table_node);

  ~AXTableInfo();

  // The real row count, guaranteed to be at least as large as the
  // maximum row index of any cell.
  int row_count;

  // The real column count, guaranteed to be at least as large as the
  // maximum column index of any cell.
  int col_count;

  // List of column header nodes IDs for each column index.
  std::vector<std::vector<int32_t>> col_headers;

  // List of row header node IDs for each row index.
  std::vector<std::vector<int32_t>> row_headers;

  // 2-D array of [row][column] -> cell node ID.
  // This may contain duplicates if there is a rowspan or
  // colspan. The entry is empty (zero) only if the cell
  // really is missing from the table.
  std::vector<std::vector<int32_t>> cell_ids;

  // Set of all unique cell node IDs in the table.
  std::vector<int32_t> unique_cell_ids;

  // Map from each cell's node ID to its index in unique_cell_ids.
  base::hash_map<int32_t, int32_t> cell_id_to_index;

 private:
  AXTableInfo();
};

}  // namespace ui

#endif  // UI_ACCESSIBILITY_AX_TABLE_INFO
