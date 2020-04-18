// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ACCESSIBILITY_AX_ROLE_PROPERTIES_H_
#define UI_ACCESSIBILITY_AX_ROLE_PROPERTIES_H_

#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_export.h"

namespace ui {

// Checks if the given role should belong to a control that can respond to
// clicks.
AX_EXPORT bool IsRoleClickable(ax::mojom::Role role);

// Returns true if this node is a link.
AX_EXPORT bool IsLink(ax::mojom::Role role);

// Returns true if this node is a list.
AX_EXPORT bool IsList(ax::mojom::Role role);

// Returns true if this node is a list item.
AX_EXPORT bool IsListItem(ax::mojom::Role role);

// Returns true if this node is a document.
AX_EXPORT bool IsDocument(ax::mojom::Role role);

// Returns true if this node is a cell or a table header.
AX_EXPORT bool IsCellOrTableHeaderRole(ax::mojom::Role role);

// Returns true if this node is a table, a grid or a treegrid.
AX_EXPORT bool IsTableLikeRole(ax::mojom::Role role);

// Returns true if this node is a container with selectable children.
AX_EXPORT bool IsContainerWithSelectableChildrenRole(ax::mojom::Role role);

// Returns true if this node is a row container.
AX_EXPORT bool IsRowContainer(ax::mojom::Role role);

// Returns true if this node is a control.
AX_EXPORT bool IsControl(ax::mojom::Role role);

// Returns true if this node is a menu or related role.
AX_EXPORT bool IsMenuRelated(ax::mojom::Role role);

// Returns true if it's an image, graphic, canvas, etc.
AX_EXPORT bool IsImage(ax::mojom::Role role);

// Returns true if it's a heading.
AX_EXPORT bool IsHeading(ax::mojom::Role role);

// Returns true if it's a heading.
AX_EXPORT bool IsHeadingOrTableHeader(ax::mojom::Role role);
}  // namespace ui

#endif  // UI_ACCESSIBILITY_AX_ROLE_PROPERTIES_H_
