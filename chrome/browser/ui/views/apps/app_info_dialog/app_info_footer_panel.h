// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_APPS_APP_INFO_DIALOG_APP_INFO_FOOTER_PANEL_H_
#define CHROME_BROWSER_UI_VIEWS_APPS_APP_INFO_DIALOG_APP_INFO_FOOTER_PANEL_H_

#include <memory>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/extensions/extension_uninstall_dialog.h"
#include "chrome/browser/ui/views/apps/app_info_dialog/app_info_panel.h"
#include "ui/views/controls/button/button.h"

class Profile;

namespace extensions {
class Extension;
}

namespace ui {
class Event;
}

// A small summary panel with buttons to control the app that is displayed at
// the bottom of the app info dialog.
class AppInfoFooterPanel
    : public AppInfoPanel,
      public views::ButtonListener,
      public extensions::ExtensionUninstallDialog::Delegate,
      public base::SupportsWeakPtr<AppInfoFooterPanel> {
 public:
  AppInfoFooterPanel(Profile* profile, const extensions::Extension* app);
  ~AppInfoFooterPanel() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(AppInfoDialogViewsTest,
                           PinButtonsAreFocusedAfterPinUnpin);

  void CreateButtons();
  void LayoutButtons();

  // Updates the visibility of the pin/unpin buttons so that only one is visible
  // at a time. If |focus_button| is true, sets the focus to whichever button is
  // now visible.
  void UpdatePinButtons(bool focus_visible_button);

  // Overridden from views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // Overridden from ExtensionUninstallDialog::Delegate:
  void OnExtensionUninstallDialogClosed(bool did_start_uninstall,
                                        const base::string16& error) override;

  // Create Shortcuts for the app. Must only be called if CanCreateShortcuts()
  // returns true.
  void CreateShortcuts();
  bool CanCreateShortcuts() const;

#if defined(OS_CHROMEOS)
  // Pins and unpins the app from the shelf. Must only be called if
  // CanSetPinnedToShelf() returns true.
  void SetPinnedToShelf(bool value);
  bool CanSetPinnedToShelf() const;
#endif

  // Uninstall the app. Must only be called if CanUninstallApp() returns true.
  void UninstallApp();
  bool CanUninstallApp() const;

  // UI elements on the dialog. Elements are NULL if they are not displayed.
  views::View* create_shortcuts_button_;
  views::View* pin_to_shelf_button_;
  views::View* unpin_from_shelf_button_;
  views::View* remove_button_;

  std::unique_ptr<extensions::ExtensionUninstallDialog>
      extension_uninstall_dialog_;

  base::WeakPtrFactory<AppInfoFooterPanel> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AppInfoFooterPanel);
};

#endif  // CHROME_BROWSER_UI_VIEWS_APPS_APP_INFO_DIALOG_APP_INFO_FOOTER_PANEL_H_
