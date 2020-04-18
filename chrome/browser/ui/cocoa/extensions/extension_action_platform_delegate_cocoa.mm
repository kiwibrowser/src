// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/extensions/extension_action_platform_delegate_cocoa.h"

#include <string>
#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/extensions/extension_action.h"
#include "chrome/browser/extensions/extension_view_host.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/app_menu/app_menu_controller.h"
#include "chrome/browser/ui/cocoa/browser_dialogs_views_mac.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/extensions/browser_action_button.h"
#import "chrome/browser/ui/cocoa/extensions/browser_actions_controller.h"
#import "chrome/browser/ui/cocoa/extensions/extension_popup_controller.h"
#import "chrome/browser/ui/cocoa/extensions/extension_popup_views_mac.h"
#import "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_controller.h"
#include "chrome/browser/ui/toolbar/toolbar_action_view_delegate.h"
#include "chrome/common/extensions/api/extension_action/action_info.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "extensions/browser/notification_types.h"
#include "extensions/common/extension.h"
#import "ui/base/cocoa/cocoa_base_utils.h"
#include "ui/base/ui_features.h"
#include "ui/gfx/geometry/point.h"
#import "ui/gfx/mac/coordinate_conversion.h"
#include "ui/gfx/native_widget_types.h"

namespace {

// Returns the notification to listen to for activation for a particular
// |extension_action|.
int GetNotificationTypeForAction(const ExtensionAction& extension_action) {
  if (extension_action.action_type() == extensions::ActionInfo::TYPE_BROWSER)
    return extensions::NOTIFICATION_EXTENSION_COMMAND_BROWSER_ACTION_MAC;

  // We should only have page and browser action types.
  DCHECK_EQ(extensions::ActionInfo::TYPE_PAGE, extension_action.action_type());
  return extensions::NOTIFICATION_EXTENSION_COMMAND_PAGE_ACTION_MAC;
}

}  // namespace

// static
std::unique_ptr<ExtensionActionPlatformDelegate>
ExtensionActionPlatformDelegate::CreateCocoa(
    ExtensionActionViewController* controller) {
  return base::WrapUnique(new ExtensionActionPlatformDelegateCocoa(controller));
}

#if !BUILDFLAG(MAC_VIEWS_BROWSER)
std::unique_ptr<ExtensionActionPlatformDelegate>
ExtensionActionPlatformDelegate::Create(
    ExtensionActionViewController* controller) {
  return CreateCocoa(controller);
}
#endif

ExtensionActionPlatformDelegateCocoa::ExtensionActionPlatformDelegateCocoa(
    ExtensionActionViewController* controller)
    : controller_(controller) {
}

ExtensionActionPlatformDelegateCocoa::~ExtensionActionPlatformDelegateCocoa() {
}

void ExtensionActionPlatformDelegateCocoa::RegisterCommand() {
  // Commands are handled elsewhere for Cocoa.
}

void ExtensionActionPlatformDelegateCocoa::OnDelegateSet() {
  registrar_.Add(
      this,
      GetNotificationTypeForAction(*controller_->extension_action()),
      content::Source<Profile>(controller_->browser()->profile()));
}

void ExtensionActionPlatformDelegateCocoa::ShowPopup(
    std::unique_ptr<extensions::ExtensionViewHost> host,
    bool grant_tab_permissions,
    ExtensionActionViewController::PopupShowAction show_action) {
  if (!chrome::ShowAllDialogsWithViewsToolkit()) {
    BOOL devMode =
        show_action == ExtensionActionViewController::SHOW_POPUP_AND_INSPECT;
    [ExtensionPopupController host:std::move(host)
                         inBrowser:controller_->browser()
                        anchoredAt:GetPopupPoint()
                           devMode:devMode];
    return;
  }

  ExtensionPopup::ShowAction popupShowAction =
      show_action == ExtensionActionViewController::SHOW_POPUP
          ? ExtensionPopup::SHOW
          : ExtensionPopup::SHOW_AND_INSPECT;
  gfx::NativeWindow containingWindow =
      controller_->browser()->window()->GetNativeWindow();
  NSPoint pointWindow = GetPopupPoint();
  NSPoint pointScreen =
      ui::ConvertPointFromWindowToScreen(containingWindow, pointWindow);
  gfx::Point viewsScreenPoint = gfx::ScreenPointFromNSPoint(pointScreen);
  ExtensionPopupViewsMac::ShowPopup(std::move(host), containingWindow,
                                    viewsScreenPoint, popupShowAction);
}

void ExtensionActionPlatformDelegateCocoa::CloseOverflowMenu() {
  // If this was triggered by an action overflowed to the app menu, then the app
  // menu will be open. Close it before opening the popup.
  AppMenuController* appMenuController =
      [[[BrowserWindowController
          browserWindowControllerForWindow:
              controller_->browser()->window()->GetNativeWindow()]
          toolbarController] appMenuController];
  if ([appMenuController isMenuOpen])
    [appMenuController cancel];
}

void ExtensionActionPlatformDelegateCocoa::ShowContextMenu() {
  // We should only use this code path for extensions shown in the toolbar.
  BrowserWindowController* windowController = [BrowserWindowController
      browserWindowControllerForWindow:controller_->browser()
                                           ->window()
                                           ->GetNativeWindow()];
  BrowserActionsController* actionsController =
      [[windowController toolbarController] browserActionsController];
  [[actionsController mainButtonForId:controller_->GetId()] showContextMenu];
}

NSPoint ExtensionActionPlatformDelegateCocoa::GetPopupPoint() const {
  BrowserWindowController* windowController =
      [BrowserWindowController browserWindowControllerForWindow:
          controller_->browser()->window()->GetNativeWindow()];
  BrowserActionsController* actionsController =
      [[windowController toolbarController] browserActionsController];
  return [actionsController popupPointForId:controller_->GetId()];
}

void ExtensionActionPlatformDelegateCocoa::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  switch (type) {
    case extensions::NOTIFICATION_EXTENSION_COMMAND_BROWSER_ACTION_MAC:
    case extensions::NOTIFICATION_EXTENSION_COMMAND_PAGE_ACTION_MAC: {
      DCHECK_EQ(type,
                GetNotificationTypeForAction(*controller_->extension_action()));
      std::pair<const std::string, gfx::NativeWindow>* payload =
          content::Details<std::pair<const std::string, gfx::NativeWindow> >(
              details).ptr();
      const std::string& extension_id = payload->first;
      gfx::NativeWindow window = payload->second;
      if (window == controller_->browser()->window()->GetNativeWindow() &&
          extension_id == controller_->extension()->id() &&
          controller_->IsEnabled(
              controller_->view_delegate()->GetCurrentWebContents())) {
        controller_->ExecuteAction(true);
      }
      break;
    }
    default:
      NOTREACHED() << "Unexpected notification";
  }
}
