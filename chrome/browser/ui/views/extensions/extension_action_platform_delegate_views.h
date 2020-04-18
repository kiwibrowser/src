// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_EXTENSIONS_EXTENSION_ACTION_PLATFORM_DELEGATE_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_EXTENSIONS_EXTENSION_ACTION_PLATFORM_DELEGATE_VIEWS_H_

#include "base/macros.h"
#include "chrome/browser/ui/extensions/extension_action_platform_delegate.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "ui/base/accelerators/accelerator.h"

class ToolbarActionViewDelegateViews;

// An abstract "View" for an ExtensionAction (either a BrowserAction or a
// PageAction). This contains the logic for showing the action's popup and
// the context menu. This class doesn't subclass View directly, as the
// implementations for page actions/browser actions are different types of
// views.
// All common logic for executing extension actions should go in this class;
// ToolbarActionViewDelegate classes should only have knowledge relating to
// the views::View wrapper.
class ExtensionActionPlatformDelegateViews
    : public ExtensionActionPlatformDelegate,
      public content::NotificationObserver,
      public ui::AcceleratorTarget {
 public:
  ExtensionActionPlatformDelegateViews(
      ExtensionActionViewController* controller);
  ~ExtensionActionPlatformDelegateViews() override;

 private:
  // ExtensionActionPlatformDelegate:
  void RegisterCommand() override;
  void ShowPopup(
      std::unique_ptr<extensions::ExtensionViewHost> host,
      bool grant_tab_permissions,
      ExtensionActionViewController::PopupShowAction show_action) override;
  void CloseOverflowMenu() override;
  void ShowContextMenu() override;

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // ui::AcceleratorTarget:
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;
  bool CanHandleAccelerators() const override;

  // Unregisters the accelerator for the extension action's command, if one
  // exists. If |only_if_removed| is true, then this will only unregister if the
  // command has been removed.
  void UnregisterCommand(bool only_if_removed);

  ToolbarActionViewDelegateViews* GetDelegateViews() const;

  // The owning ExtensionActionViewController.
  ExtensionActionViewController* controller_;

  // The extension key binding accelerator this extension action is listening
  // for (to show the popup).
  std::unique_ptr<ui::Accelerator> action_keybinding_;

  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionActionPlatformDelegateViews);
};

#endif  // CHROME_BROWSER_UI_VIEWS_EXTENSIONS_EXTENSION_ACTION_PLATFORM_DELEGATE_VIEWS_H_
