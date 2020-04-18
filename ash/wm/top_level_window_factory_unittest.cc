// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/top_level_window_factory.h"

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/test/ash_test_helper.h"
#include "ash/window_manager.h"
#include "ash/window_manager_service.h"
#include "ash/wm/window_properties.h"
#include "services/ui/public/cpp/property_type_converters.h"
#include "ui/aura/mus/property_converter.h"
#include "ui/aura/window.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/test/gfx_util.h"
#include "ui/wm/core/window_util.h"

namespace ash {

namespace {

int64_t GetDisplayId(aura::Window* window) {
  return display::Screen::GetScreen()->GetDisplayNearestWindow(window).id();
}

aura::Window* CreateFullscreenTestWindow(WindowManager* window_manager,
                                         int64_t display_id) {
  std::map<std::string, std::vector<uint8_t>> properties;
  properties[ui::mojom::WindowManager::kShowState_Property] =
      mojo::ConvertTo<std::vector<uint8_t>>(
          static_cast<aura::PropertyConverter::PrimitiveType>(
              ui::mojom::ShowState::FULLSCREEN));
  if (display_id != display::kInvalidDisplayId) {
    properties[ui::mojom::WindowManager::kDisplayId_InitProperty] =
        mojo::ConvertTo<std::vector<uint8_t>>(display_id);
  }
  aura::Window* window = CreateAndParentTopLevelWindow(
      window_manager, ui::mojom::WindowType::WINDOW,
      window_manager->property_converter(), &properties);
  window->Show();
  return window;
}

}  // namespace

using TopLevelWindowFactoryTest = AshTestBase;

TEST_F(TopLevelWindowFactoryTest, CreateFullscreenWindow) {
  std::unique_ptr<aura::Window> window = CreateTestWindow();
  ::wm::SetWindowFullscreen(window.get(), true);
  aura::Window* root_window = Shell::GetPrimaryRootWindow();
  EXPECT_EQ(root_window->bounds(), window->bounds());
}

using TopLevelWindowFactoryWmTest = AshTestBase;

TEST_F(TopLevelWindowFactoryWmTest, IsWindowShownInCorrectDisplay) {
  UpdateDisplay("400x400,400x400");
  EXPECT_NE(GetPrimaryDisplay().id(), GetSecondaryDisplay().id());

  WindowManager* window_manager =
      ash_test_helper()->window_manager_service()->window_manager();

  std::unique_ptr<aura::Window> window_primary_display(
      CreateFullscreenTestWindow(window_manager, GetPrimaryDisplay().id()));
  std::unique_ptr<aura::Window> window_secondary_display(
      CreateFullscreenTestWindow(window_manager, GetSecondaryDisplay().id()));

  EXPECT_EQ(GetPrimaryDisplay().id(),
            GetDisplayId(window_primary_display.get()));
  EXPECT_EQ(GetSecondaryDisplay().id(),
            GetDisplayId(window_secondary_display.get()));
}

using TopLevelWindowFactoryAshTest = AshTestBase;

TEST_F(TopLevelWindowFactoryAshTest, TopLevelNotShownOnCreate) {
  std::map<std::string, std::vector<uint8_t>> properties;
  auto* window_manager =
      ash_test_helper()->window_manager_service()->window_manager();
  std::unique_ptr<aura::Window> window(CreateAndParentTopLevelWindow(
      window_manager, ui::mojom::WindowType::WINDOW,
      window_manager->property_converter(), &properties));
  ASSERT_TRUE(window);
  EXPECT_FALSE(window->IsVisible());
}

TEST_F(TopLevelWindowFactoryAshTest, CreateTopLevelWindow) {
  const gfx::Rect bounds(1, 2, 124, 345);
  std::map<std::string, std::vector<uint8_t>> properties;
  properties[ui::mojom::WindowManager::kBounds_InitProperty] =
      mojo::ConvertTo<std::vector<uint8_t>>(bounds);
  properties[ui::mojom::WindowManager::kResizeBehavior_Property] =
      mojo::ConvertTo<std::vector<uint8_t>>(
          static_cast<aura::PropertyConverter::PrimitiveType>(
              ui::mojom::kResizeBehaviorCanResize |
              ui::mojom::kResizeBehaviorCanMaximize |
              ui::mojom::kResizeBehaviorCanMinimize));
  WindowManager* window_manager =
      ash_test_helper()->window_manager_service()->window_manager();
  // |window| is owned by its parent.
  aura::Window* window = CreateAndParentTopLevelWindow(
      window_manager, ui::mojom::WindowType::WINDOW,
      window_manager->property_converter(), &properties);
  ASSERT_TRUE(window->parent());
  EXPECT_EQ(kShellWindowId_DefaultContainer, window->parent()->id());
  EXPECT_EQ(bounds, window->bounds());
  EXPECT_EQ(WidgetCreationType::FOR_CLIENT,
            window->GetProperty(kWidgetCreationTypeKey));
}

}  // namespace ash
