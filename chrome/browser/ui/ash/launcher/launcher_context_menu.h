// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_LAUNCHER_LAUNCHER_CONTEXT_MENU_H_
#define CHROME_BROWSER_UI_ASH_LAUNCHER_LAUNCHER_CONTEXT_MENU_H_

#include "ash/public/cpp/shelf_item.h"
#include "base/macros.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/gfx/vector_icon_types.h"

class ChromeLauncherController;

// A base class for browser, extension, and ARC shelf item context menus.
class LauncherContextMenu : public ui::SimpleMenuModel::Delegate {
 public:
  // Menu item command ids, used by subclasses and tests.
  // These are used in histograms, do not remove/renumber entries. Only add at
  // the end just before MENU_ITEM_COUNT. If you're adding to this enum with the
  // intention that it will be logged, add checks to ensure stability of the
  // enum and update the ChromeOSUICommands enum listing in
  // tools/metrics/histograms/enums.xml. Besides LAUNCH_APP_SHORTCUT_FIRST and
  // LAUNCH_APP_SHORTCUT_LAST, these enums must not overlap with enums in
  // AppContextMenu::CommandId.
  enum MenuItem {
    MENU_OPEN_NEW = 0,
    MENU_CLOSE = 1,
    MENU_PIN = 2,
    LAUNCH_TYPE_PINNED_TAB = 3,
    LAUNCH_TYPE_REGULAR_TAB = 4,
    LAUNCH_TYPE_FULLSCREEN = 5,
    LAUNCH_TYPE_WINDOW = 6,
    MENU_NEW_WINDOW = 7,
    MENU_NEW_INCOGNITO_WINDOW = 8,
    // Range of command ids reserved for launching app shortcuts from context
    // menu for Android app. Must overlap with AppContextMenu::CommandId.
    LAUNCH_APP_SHORTCUT_FIRST = 1000,
    LAUNCH_APP_SHORTCUT_LAST = 1999,
    MENU_ITEM_COUNT
  };

  ~LauncherContextMenu() override;

  // Static function to create a context menu instance.
  static std::unique_ptr<LauncherContextMenu> Create(
      ChromeLauncherController* controller,
      const ash::ShelfItem* item,
      int64_t display_id);

  using GetMenuModelCallback =
      base::OnceCallback<void(std::unique_ptr<ui::MenuModel>)>;
  virtual void GetMenuModel(GetMenuModelCallback callback) = 0;

  // ui::SimpleMenuModel::Delegate overrides:
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  void ExecuteCommand(int command_id, int event_flags) override;

 protected:
  LauncherContextMenu(ChromeLauncherController* controller,
                      const ash::ShelfItem* item,
                      int64_t display_id);

  ChromeLauncherController* controller() const { return controller_; }
  const ash::ShelfItem& item() const { return item_; }

  // Add menu item for pin/unpin.
  void AddPinMenu(ui::SimpleMenuModel* menu_model);

  // Helper method to execute common commands. Returns true if handled.
  bool ExecuteCommonCommand(int command_id, int event_flags);

  // Helper method to add touchable or normal context menu options.
  void AddContextMenuOption(ui::SimpleMenuModel* menu_model,
                            MenuItem type,
                            int string_id);

  // Helper method to get the gfx::VectorIcon for a |type|. Returns an empty
  // gfx::VectorIcon if there is no icon for this |type|.
  const gfx::VectorIcon& GetMenuItemVectorIcon(MenuItem type,
                                               int string_id) const;

  int64_t display_id() const { return display_id_; }

 private:
  ChromeLauncherController* controller_;

  const ash::ShelfItem item_;

  const int64_t display_id_;

  DISALLOW_COPY_AND_ASSIGN(LauncherContextMenu);
};

#endif  // CHROME_BROWSER_UI_ASH_LAUNCHER_LAUNCHER_CONTEXT_MENU_H_
