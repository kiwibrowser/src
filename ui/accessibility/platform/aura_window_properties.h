// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ACCESSIBILITY_PLATFORM_AURA_WINDOW_PROPERTIES_H_
#define UI_ACCESSIBILITY_PLATFORM_AURA_WINDOW_PROPERTIES_H_

#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_export.h"
#include "ui/accessibility/ax_tree_id_registry.h"
#include "ui/aura/window.h"

namespace ui {

AX_EXPORT extern const aura::WindowProperty<AXTreeIDRegistry::AXTreeID>* const
    kChildAXTreeID;

AX_EXPORT extern const aura::WindowProperty<ax::mojom::Role>* const
    kAXRoleOverride;

}  // namespace ui

#endif  // UI_ACCESSIBILITY_PLATFORM_AURA_WINDOW_PROPERTIES_H_
