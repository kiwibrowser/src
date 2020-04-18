// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/launcher/extension_launcher_context_menu.h"

#include <utility>

#include "ash/scoped_root_window_for_new_windows.h"  // mash-ok
#include "ash/shell.h"                               // mash-ok
#include "base/bind.h"
#include "chrome/browser/chromeos/ash_config.h"
#include "chrome/browser/extensions/context_menu_matcher.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/extensions/launch_util.h"
#include "chrome/browser/prefs/incognito_mode_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/ash/launcher/browser_shortcut_launcher_item_controller.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller_util.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/common/context_menu_params.h"
#include "extensions/browser/extension_prefs.h"
#include "ui/base/ui_base_features.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/menu/menu_config.h"

namespace {

// A helper used to filter which menu items added by the extension are shown.
bool MenuItemHasLauncherContext(const extensions::MenuItem* item) {
  return item->contexts().Contains(extensions::MenuItem::LAUNCHER);
}

}  // namespace

ExtensionLauncherContextMenu::ExtensionLauncherContextMenu(
    ChromeLauncherController* controller,
    const ash::ShelfItem* item,
    int64_t display_id)
    : LauncherContextMenu(controller, item, display_id) {}

ExtensionLauncherContextMenu::~ExtensionLauncherContextMenu() = default;

void ExtensionLauncherContextMenu::GetMenuModel(GetMenuModelCallback callback) {
  auto menu_model = std::make_unique<ui::SimpleMenuModel>(this);
  BuildMenu(menu_model.get());
  std::move(callback).Run(std::move(menu_model));
}

bool ExtensionLauncherContextMenu::IsCommandIdChecked(int command_id) const {
  switch (command_id) {
    case LAUNCH_TYPE_PINNED_TAB:
      return GetLaunchType() == extensions::LAUNCH_TYPE_PINNED;
    case LAUNCH_TYPE_REGULAR_TAB:
      return GetLaunchType() == extensions::LAUNCH_TYPE_REGULAR;
    case LAUNCH_TYPE_WINDOW:
      return GetLaunchType() == extensions::LAUNCH_TYPE_WINDOW;
    case LAUNCH_TYPE_FULLSCREEN:
      return GetLaunchType() == extensions::LAUNCH_TYPE_FULLSCREEN;
    default:
      if (command_id < MENU_ITEM_COUNT)
        return LauncherContextMenu::IsCommandIdChecked(command_id);
      return (extension_items_ &&
              extension_items_->IsCommandIdChecked(command_id));
  }
}

bool ExtensionLauncherContextMenu::IsCommandIdEnabled(int command_id) const {
  switch (command_id) {
    case MENU_NEW_WINDOW:
      // "Normal" windows are not allowed when incognito is enforced.
      return IncognitoModePrefs::GetAvailability(
                 controller()->profile()->GetPrefs()) !=
             IncognitoModePrefs::FORCED;
    case MENU_NEW_INCOGNITO_WINDOW:
      // Incognito windows are not allowed when incognito is disabled.
      return IncognitoModePrefs::GetAvailability(
                 controller()->profile()->GetPrefs()) !=
             IncognitoModePrefs::DISABLED;
    default:
      if (command_id < MENU_ITEM_COUNT)
        return LauncherContextMenu::IsCommandIdEnabled(command_id);
      return (extension_items_ &&
              extension_items_->IsCommandIdEnabled(command_id));
  }
}

void ExtensionLauncherContextMenu::ExecuteCommand(int command_id,
                                                  int event_flags) {
  if (ExecuteCommonCommand(command_id, event_flags))
    return;

  // Place new windows on the same display as the context menu.
  // TODO(crbug.com/768908): Fix this in mash (where Chrome can't use Shell).
  std::unique_ptr<ash::ScopedRootWindowForNewWindows> scoped_root;
  if (chromeos::GetAshConfig() != ash::Config::MASH) {
    aura::Window* root = ash::Shell::GetRootWindowForDisplayId(display_id());
    scoped_root = std::make_unique<ash::ScopedRootWindowForNewWindows>(root);
  }

  switch (static_cast<MenuItem>(command_id)) {
    case LAUNCH_TYPE_PINNED_TAB:
      SetLaunchType(extensions::LAUNCH_TYPE_PINNED);
      break;
    case LAUNCH_TYPE_REGULAR_TAB:
      SetLaunchType(extensions::LAUNCH_TYPE_REGULAR);
      break;
    case LAUNCH_TYPE_WINDOW: {
      extensions::LaunchType launch_type = extensions::LAUNCH_TYPE_WINDOW;
      // With bookmark apps enabled, hosted apps can only toggle between
      // LAUNCH_WINDOW and LAUNCH_REGULAR.
      if (extensions::util::IsNewBookmarkAppsEnabled()) {
        launch_type = GetLaunchType() == extensions::LAUNCH_TYPE_WINDOW
                          ? extensions::LAUNCH_TYPE_REGULAR
                          : extensions::LAUNCH_TYPE_WINDOW;
      }
      SetLaunchType(launch_type);
      break;
    }
    case LAUNCH_TYPE_FULLSCREEN:
      SetLaunchType(extensions::LAUNCH_TYPE_FULLSCREEN);
      break;
    case MENU_NEW_WINDOW:
      chrome::NewEmptyWindow(controller()->profile());
      break;
    case MENU_NEW_INCOGNITO_WINDOW:
      chrome::NewEmptyWindow(controller()->profile()->GetOffTheRecordProfile());
      break;
    default:
      if (extension_items_) {
        extension_items_->ExecuteCommand(command_id, nullptr, nullptr,
                                         content::ContextMenuParams());
      }
  }
}

void ExtensionLauncherContextMenu::CreateOpenNewSubmenu(
    ui::SimpleMenuModel* menu_model) {
  // Touchable extension context menus use an actionable submenu for
  // MENU_OPEN_NEW.
  const gfx::VectorIcon& icon =
      GetMenuItemVectorIcon(MENU_OPEN_NEW, GetLaunchTypeStringId());
  const views::MenuConfig& menu_config = views::MenuConfig::instance();
  const int kGroupId = 1;
  open_new_submenu_model_ = std::make_unique<ui::SimpleMenuModel>(this);
  open_new_submenu_model_->AddRadioItemWithStringId(
      LAUNCH_TYPE_REGULAR_TAB, IDS_APP_LIST_CONTEXT_MENU_NEW_TAB, kGroupId);
  open_new_submenu_model_->AddRadioItemWithStringId(
      LAUNCH_TYPE_WINDOW, IDS_APP_LIST_CONTEXT_MENU_NEW_WINDOW, kGroupId);
  menu_model->AddActionableSubmenuWithStringIdAndIcon(
      MENU_OPEN_NEW, GetLaunchTypeStringId(), open_new_submenu_model_.get(),
      gfx::CreateVectorIcon(icon, menu_config.touchable_icon_size,
                            menu_config.touchable_icon_color));
}

void ExtensionLauncherContextMenu::BuildMenu(ui::SimpleMenuModel* menu_model) {
  extension_items_.reset(new extensions::ContextMenuMatcher(
      controller()->profile(), this, menu_model,
      base::Bind(MenuItemHasLauncherContext)));
  if (item().type == ash::TYPE_PINNED_APP || item().type == ash::TYPE_APP) {
    // V1 apps can be started from the menu - but V2 apps should not.
    if (!controller()->IsPlatformApp(item().id)) {
      if (features::IsTouchableAppContextMenuEnabled()) {
        CreateOpenNewSubmenu(menu_model);
      } else {
        AddContextMenuOption(menu_model, MENU_OPEN_NEW,
                             GetLaunchTypeStringId());
        menu_model->AddSeparator(ui::NORMAL_SEPARATOR);

        // Touchable app context menus do not include these check items, their
        // functionality is achieved by an actionable submenu.
        if (item().type == ash::TYPE_PINNED_APP) {
          if (extensions::util::IsNewBookmarkAppsEnabled()) {
            // With bookmark apps enabled, hosted apps launch in a window by
            // default. This menu item is re-interpreted as a single,
            // toggle-able option to launch the hosted app as a tab.
            AddContextMenuOption(menu_model, LAUNCH_TYPE_WINDOW,
                                 IDS_APP_CONTEXT_MENU_OPEN_WINDOW);
          } else {
            AddContextMenuOption(menu_model, LAUNCH_TYPE_REGULAR_TAB,
                                 IDS_APP_CONTEXT_MENU_OPEN_REGULAR);
            AddContextMenuOption(menu_model, LAUNCH_TYPE_PINNED_TAB,
                                 IDS_APP_CONTEXT_MENU_OPEN_PINNED);
            AddContextMenuOption(menu_model, LAUNCH_TYPE_WINDOW,
                                 IDS_APP_CONTEXT_MENU_OPEN_WINDOW);
            // Even though the launch type is Full Screen it is more accurately
            // described as Maximized in Ash.
            AddContextMenuOption(menu_model, LAUNCH_TYPE_FULLSCREEN,
                                 IDS_APP_CONTEXT_MENU_OPEN_MAXIMIZED);
          }
          menu_model->AddSeparator(ui::NORMAL_SEPARATOR);
        }
      }
    }

    AddPinMenu(menu_model);

    if (controller()->IsOpen(item().id)) {
      AddContextMenuOption(menu_model, MENU_CLOSE,
                           IDS_LAUNCHER_CONTEXT_MENU_CLOSE);
    }
  } else if (item().type == ash::TYPE_BROWSER_SHORTCUT) {
    AddContextMenuOption(menu_model, MENU_NEW_WINDOW, IDS_APP_LIST_NEW_WINDOW);
    if (!controller()->profile()->IsGuestSession()) {
      AddContextMenuOption(menu_model, MENU_NEW_INCOGNITO_WINDOW,
                           IDS_APP_LIST_NEW_INCOGNITO_WINDOW);
    }
    if (!BrowserShortcutLauncherItemController(controller()->shelf_model())
             .IsListOfActiveBrowserEmpty()) {
      AddContextMenuOption(menu_model, MENU_CLOSE,
                           IDS_LAUNCHER_CONTEXT_MENU_CLOSE);
    }
  } else if (item().type == ash::TYPE_DIALOG) {
    AddContextMenuOption(menu_model, MENU_CLOSE,
                         IDS_LAUNCHER_CONTEXT_MENU_CLOSE);
  } else if (controller()->IsOpen(item().id)) {
    AddContextMenuOption(menu_model, MENU_CLOSE,
                         IDS_LAUNCHER_CONTEXT_MENU_CLOSE);
  }
  if (!features::IsTouchableAppContextMenuEnabled())
    menu_model->AddSeparator(ui::NORMAL_SEPARATOR);
  if (item().type == ash::TYPE_PINNED_APP || item().type == ash::TYPE_APP) {
    const extensions::MenuItem::ExtensionKey app_key(item().id.app_id);
    if (!app_key.empty()) {
      int index = 0;
      extension_items_->AppendExtensionItems(app_key, base::string16(), &index,
                                             false);  // is_action_menu
      if (!features::IsTouchableAppContextMenuEnabled())
        menu_model->AddSeparator(ui::NORMAL_SEPARATOR);
    }
  }
}

extensions::LaunchType ExtensionLauncherContextMenu::GetLaunchType() const {
  const extensions::Extension* extension =
      GetExtensionForAppID(item().id.app_id, controller()->profile());

  // An extension can be unloaded/updated/unavailable at any time.
  if (!extension)
    return extensions::LAUNCH_TYPE_DEFAULT;

  return extensions::GetLaunchType(
      extensions::ExtensionPrefs::Get(controller()->profile()), extension);
}

void ExtensionLauncherContextMenu::SetLaunchType(extensions::LaunchType type) {
  extensions::SetLaunchType(controller()->profile(), item().id.app_id, type);
}

int ExtensionLauncherContextMenu::GetLaunchTypeStringId() const {
  return (GetLaunchType() == extensions::LAUNCH_TYPE_PINNED ||
          GetLaunchType() == extensions::LAUNCH_TYPE_REGULAR)
             ? IDS_APP_LIST_CONTEXT_MENU_NEW_TAB
             : IDS_APP_LIST_CONTEXT_MENU_NEW_WINDOW;
}
