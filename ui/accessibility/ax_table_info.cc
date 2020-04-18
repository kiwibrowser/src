// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/accessibility/ax_table_info.h"

#include "ui/accessibility/ax_node.h"
#include "ui/accessibility/ax_role_properties.h"
#include "ui/accessibility/ax_tree.h"
#include "ui/gfx/geometry/rect_f.h"

namespace ui {

namespace {

void FindCells(AXNode* node, std::vector<AXNode*>* cells) {
  for (AXNode* child : node->children()) {
    if (child->data().HasState(ax::mojom::State::kIgnored) ||
        child->data().role == ax::mojom::Role::kGenericContainer)
      FindCells(child, cells);
    else if (IsCellOrTableHeaderRole(child->data().role))
      cells->push_back(child);
  }
}

void FindRowsAndThenCells(AXNode* node, std::vector<AXNode*>* cells) {
  for (AXNode* child : node->children()) {
    if (child->data().HasState(ax::mojom::State::kIgnored) ||
        child->data().role == ax::mojom::Role::kGenericContainer)
      FindRowsAndThenCells(child, cells);
    else if (child->data().role == ax::mojom::Role::kRow)
      FindCells(child, cells);
  }
}

}  // namespace

// static
AXTableInfo* AXTableInfo::Create(AXTree* tree, AXNode* table_node) {
  DCHECK(tree);
  DCHECK(table_node);

#if DCHECK_IS_ON()
  // Sanity check, make sure the node is in the tree.
  AXNode* node = table_node;
  while (node && node != tree->root())
    node = node->parent();
  DCHECK(node == tree->root());
#endif

  if (!IsTableLikeRole(table_node->data().role))
    return nullptr;

  AXTableInfo* info = new AXTableInfo();

  std::vector<AXNode*> cells;
  FindRowsAndThenCells(table_node, &cells);

  // Compute the actual row and column count, and the set of all unique cell ids
  // in the table.
  info->row_count = table_node->data().GetIntAttribute(
      ax::mojom::IntAttribute::kTableRowCount);
  info->col_count = table_node->data().GetIntAttribute(
      ax::mojom::IntAttribute::kTableColumnCount);
  for (AXNode* cell : cells) {
    int row_index = cell->data().GetIntAttribute(
        ax::mojom::IntAttribute::kTableCellRowIndex);
    int row_span = std::max(1, cell->data().GetIntAttribute(
                                   ax::mojom::IntAttribute::kTableCellRowSpan));
    info->row_count = std::max(info->row_count, row_index + row_span);
    int col_index = cell->data().GetIntAttribute(
        ax::mojom::IntAttribute::kTableCellColumnIndex);
    int col_span =
        std::max(1, cell->data().GetIntAttribute(
                        ax::mojom::IntAttribute::kTableCellColumnSpan));
    info->col_count = std::max(info->col_count, col_index + col_span);
  }

  // Allocate space for the 2-D array of cell IDs and 1-D
  // arrays of row headers and column headers.
  info->row_headers.resize(info->row_count);
  info->col_headers.resize(info->col_count);
  info->cell_ids.resize(info->row_count);
  for (auto& row : info->cell_ids)
    row.resize(info->col_count);

  // Now iterate over the cells and fill in the cell IDs, row headers,
  // and column headers based on the index and span of each cell.
  int32_t cell_index = 0;
  for (AXNode* cell : cells) {
    info->unique_cell_ids.push_back(cell->id());
    info->cell_id_to_index[cell->id()] = cell_index++;
    int row_index = cell->data().GetIntAttribute(
        ax::mojom::IntAttribute::kTableCellRowIndex);
    int row_span = std::max(1, cell->data().GetIntAttribute(
                                   ax::mojom::IntAttribute::kTableCellRowSpan));
    int col_index = cell->data().GetIntAttribute(
        ax::mojom::IntAttribute::kTableCellColumnIndex);
    int col_span =
        std::max(1, cell->data().GetIntAttribute(
                        ax::mojom::IntAttribute::kTableCellColumnSpan));

    // Cells must contain a 0-based row index and col index.
    if (row_index < 0 || col_index < 0)
      continue;

    for (int r = row_index; r < row_index + row_span; r++) {
      DCHECK_LT(r, info->row_count);
      for (int c = col_index; c < col_index + col_span; c++) {
        DCHECK_LT(c, info->col_count);
        info->cell_ids[r][c] = cell->id();
        if (cell->data().role == ax::mojom::Role::kColumnHeader)
          info->col_headers[c].push_back(cell->id());
        else if (cell->data().role == ax::mojom::Role::kRowHeader)
          info->row_headers[r].push_back(cell->id());
      }
    }
  }

  return info;
}

AXTableInfo::AXTableInfo() {}

AXTableInfo::~AXTableInfo() {}

}  // namespace ui
