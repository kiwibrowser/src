// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/window_properties.h"

#include "ash/wm/window_state.h"
#include "ui/gfx/geometry/rect.h"

DEFINE_UI_CLASS_PROPERTY_TYPE(ash::wm::WindowState*);
DEFINE_EXPORTED_UI_CLASS_PROPERTY_TYPE(ASH_EXPORT, ash::WidgetCreationType);

namespace ash {

DEFINE_UI_CLASS_PROPERTY_KEY(bool, kLockedToRootKey, false);

DEFINE_UI_CLASS_PROPERTY_KEY(bool, kSnapChildrenToPixelBoundary, false);

DEFINE_UI_CLASS_PROPERTY_KEY(WidgetCreationType,
                             kWidgetCreationTypeKey,
                             WidgetCreationType::INTERNAL);

DEFINE_UI_CLASS_PROPERTY_KEY(bool, kWindowIsJanky, false);

DEFINE_OWNED_UI_CLASS_PROPERTY_KEY(ash::wm::WindowState, kWindowStateKey, NULL);

}  // namespace ash
