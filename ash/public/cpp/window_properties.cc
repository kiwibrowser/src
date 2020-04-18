// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/window_properties.h"

#include "ash/public/cpp/ash_constants.h"
#include "ash/public/cpp/shelf_types.h"
#include "ash/public/cpp/window_pin_type.h"
#include "ash/public/cpp/window_state_type.h"
#include "ash/public/interfaces/window_pin_type.mojom.h"
#include "ash/public/interfaces/window_properties.mojom.h"
#include "ash/public/interfaces/window_state_type.mojom.h"
#include "services/ui/public/interfaces/window_manager.mojom.h"
#include "ui/aura/mus/property_converter.h"
#include "ui/aura/window.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/wm/core/shadow_types.h"

DEFINE_EXPORTED_UI_CLASS_PROPERTY_TYPE(ASH_PUBLIC_EXPORT,
                                       ash::mojom::WindowPinType)
DEFINE_EXPORTED_UI_CLASS_PROPERTY_TYPE(ASH_PUBLIC_EXPORT,
                                       ash::mojom::WindowStateType)
DEFINE_EXPORTED_UI_CLASS_PROPERTY_TYPE(ASH_PUBLIC_EXPORT,
                                       ash::BackdropWindowMode)

namespace ash {

void RegisterWindowProperties(aura::PropertyConverter* property_converter) {
  property_converter->RegisterPrimitiveProperty(
      kCanConsumeSystemKeysKey, mojom::kCanConsumeSystemKeys_Property,
      aura::PropertyConverter::CreateAcceptAnyValueCallback());
  property_converter->RegisterImageSkiaProperty(
      kFrameImageActiveKey, mojom::kFrameImageActive_Property);
  property_converter->RegisterPrimitiveProperty(
      kHideShelfWhenFullscreenKey, mojom::kHideShelfWhenFullscreen_Property,
      aura::PropertyConverter::CreateAcceptAnyValueCallback());
  property_converter->RegisterPrimitiveProperty(
      kPanelAttachedKey, ui::mojom::WindowManager::kPanelAttached_Property,
      aura::PropertyConverter::CreateAcceptAnyValueCallback());
  property_converter->RegisterPrimitiveProperty(
      kRenderTitleAreaProperty,
      ui::mojom::WindowManager::kRenderParentTitleArea_Property,
      aura::PropertyConverter::CreateAcceptAnyValueCallback());
  // This property is already registered by MusClient in Chrome, but not in Ash.
  if (!property_converter->IsTransportNameRegistered(
          ui::mojom::WindowManager::kShadowElevation_Property)) {
    property_converter->RegisterPrimitiveProperty(
        ::wm::kShadowElevationKey,
        ui::mojom::WindowManager::kShadowElevation_Property,
        aura::PropertyConverter::CreateAcceptAnyValueCallback());
  }
  property_converter->RegisterPrimitiveProperty(
      kShelfItemTypeKey, ui::mojom::WindowManager::kShelfItemType_Property,
      base::BindRepeating(&IsValidShelfItemType));
  property_converter->RegisterPrimitiveProperty(
      kWindowStateTypeKey, mojom::kWindowStateType_Property,
      base::BindRepeating(&IsValidWindowStateType));
  property_converter->RegisterPrimitiveProperty(
      kWindowPinTypeKey, mojom::kWindowPinType_Property,
      base::BindRepeating(&IsValidWindowPinType));
  property_converter->RegisterPrimitiveProperty(
      kWindowPositionManagedTypeKey, mojom::kWindowPositionManaged_Property,
      aura::PropertyConverter::CreateAcceptAnyValueCallback());
  property_converter->RegisterStringProperty(
      kShelfIDKey, ui::mojom::WindowManager::kShelfID_Property);
  property_converter->RegisterPrimitiveProperty(
      kRestoreBoundsOverrideKey, mojom::kRestoreBoundsOverride_Property,
      aura::PropertyConverter::CreateAcceptAnyValueCallback());
  property_converter->RegisterPrimitiveProperty(
      kRestoreWindowStateTypeOverrideKey,
      mojom::kRestoreWindowStateTypeOverride_Property,
      base::BindRepeating(&IsValidWindowStateType));
  property_converter->RegisterPrimitiveProperty(
      kWindowTitleShownKey,
      ui::mojom::WindowManager::kWindowTitleShown_Property,
      aura::PropertyConverter::CreateAcceptAnyValueCallback());
}

DEFINE_UI_CLASS_PROPERTY_KEY(BackdropWindowMode,
                             kBackdropWindowMode,
                             BackdropWindowMode::kAuto);
DEFINE_UI_CLASS_PROPERTY_KEY(bool, kCanConsumeSystemKeysKey, false);
DEFINE_OWNED_UI_CLASS_PROPERTY_KEY(gfx::ImageSkia,
                                   kFrameImageActiveKey,
                                   nullptr);
DEFINE_UI_CLASS_PROPERTY_KEY(bool, kHideShelfWhenFullscreenKey, true);
DEFINE_UI_CLASS_PROPERTY_KEY(bool, kIsDraggingTabsKey, false);
DEFINE_UI_CLASS_PROPERTY_KEY(bool, kPanelAttachedKey, true);
DEFINE_UI_CLASS_PROPERTY_KEY(bool, kRenderTitleAreaProperty, false);
DEFINE_OWNED_UI_CLASS_PROPERTY_KEY(gfx::Rect,
                                   kRestoreBoundsOverrideKey,
                                   nullptr);
DEFINE_UI_CLASS_PROPERTY_KEY(mojom::WindowStateType,
                             kRestoreWindowStateTypeOverrideKey,
                             mojom::WindowStateType::DEFAULT);
DEFINE_OWNED_UI_CLASS_PROPERTY_KEY(std::string, kShelfIDKey, nullptr);
DEFINE_UI_CLASS_PROPERTY_KEY(int32_t, kShelfItemTypeKey, TYPE_UNDEFINED);
DEFINE_UI_CLASS_PROPERTY_KEY(bool, kShowInOverviewKey, true);
DEFINE_UI_CLASS_PROPERTY_KEY(aura::Window*,
                             kTabDraggingSourceWindowKey,
                             nullptr);

DEFINE_UI_CLASS_PROPERTY_KEY(SkColor, kFrameActiveColorKey, kDefaultFrameColor);
DEFINE_UI_CLASS_PROPERTY_KEY(SkColor,
                             kFrameInactiveColorKey,
                             kDefaultFrameColor);
DEFINE_UI_CLASS_PROPERTY_KEY(mojom::WindowPinType,
                             kWindowPinTypeKey,
                             mojom::WindowPinType::NONE);
DEFINE_UI_CLASS_PROPERTY_KEY(bool, kWindowPositionManagedTypeKey, false);
DEFINE_UI_CLASS_PROPERTY_KEY(mojom::WindowStateType,
                             kWindowStateTypeKey,
                             mojom::WindowStateType::DEFAULT);
DEFINE_UI_CLASS_PROPERTY_KEY(bool, kWindowTitleShownKey, true);

}  // namespace ash
