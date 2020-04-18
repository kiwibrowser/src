// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/apps/app_info_dialog/app_info_footer_panel.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/grit/generated_resources.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/management_policy.h"
#include "extensions/browser/uninstall_reason.h"
#include "extensions/common/extension.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

#if defined(OS_CHROMEOS)
// gn check complains on Linux Ozone.
#include "ash/public/cpp/shelf_model.h"  // nogncheck
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller_util.h"
#endif

AppInfoFooterPanel::AppInfoFooterPanel(Profile* profile,
                                       const extensions::Extension* app)
    : AppInfoPanel(profile, app),
      create_shortcuts_button_(NULL),
      pin_to_shelf_button_(NULL),
      unpin_from_shelf_button_(NULL),
      remove_button_(NULL),
      weak_ptr_factory_(this) {
  CreateButtons();

  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::kHorizontal,
      provider->GetInsetsMetric(views::INSETS_DIALOG_SUBSECTION),
      provider->GetDistanceMetric(views::DISTANCE_RELATED_BUTTON_HORIZONTAL)));

  LayoutButtons();
}

AppInfoFooterPanel::~AppInfoFooterPanel() {
}

void AppInfoFooterPanel::CreateButtons() {
  if (CanCreateShortcuts()) {
    create_shortcuts_button_ = views::MdTextButton::CreateSecondaryUiButton(
        this, l10n_util::GetStringUTF16(
                  IDS_APPLICATION_INFO_CREATE_SHORTCUTS_BUTTON_TEXT));
  }

#if defined(OS_CHROMEOS)
  if (CanSetPinnedToShelf()) {
    pin_to_shelf_button_ = views::MdTextButton::CreateSecondaryUiButton(
        this, l10n_util::GetStringUTF16(IDS_APP_LIST_CONTEXT_MENU_PIN));
    unpin_from_shelf_button_ = views::MdTextButton::CreateSecondaryUiButton(
        this, l10n_util::GetStringUTF16(IDS_APP_LIST_CONTEXT_MENU_UNPIN));
  }
#endif

  if (CanUninstallApp()) {
    remove_button_ = views::MdTextButton::CreateSecondaryUiButton(
        this,
        l10n_util::GetStringUTF16(IDS_APPLICATION_INFO_UNINSTALL_BUTTON_TEXT));
  }
}

void AppInfoFooterPanel::LayoutButtons() {
  if (create_shortcuts_button_)
    AddChildView(create_shortcuts_button_);

  if (pin_to_shelf_button_)
    AddChildView(pin_to_shelf_button_);
  if (unpin_from_shelf_button_)
    AddChildView(unpin_from_shelf_button_);
  UpdatePinButtons(false);

  if (remove_button_)
    AddChildView(remove_button_);
}

void AppInfoFooterPanel::UpdatePinButtons(bool focus_visible_button) {
#if defined(OS_CHROMEOS)
  if (pin_to_shelf_button_ && unpin_from_shelf_button_) {
    const bool was_pinned =
        ChromeLauncherController::instance()->shelf_model()->IsAppPinned(
            app_->id());
    pin_to_shelf_button_->SetVisible(!was_pinned);
    unpin_from_shelf_button_->SetVisible(was_pinned);

    if (focus_visible_button) {
      views::View* button_to_focus =
          was_pinned ? unpin_from_shelf_button_ : pin_to_shelf_button_;
      button_to_focus->RequestFocus();
    }
  }
#endif
}

void AppInfoFooterPanel::ButtonPressed(views::Button* sender,
                                       const ui::Event& event) {
  if (sender == create_shortcuts_button_) {
    CreateShortcuts();
#if defined(OS_CHROMEOS)
  } else if (sender == pin_to_shelf_button_) {
    SetPinnedToShelf(true);
  } else if (sender == unpin_from_shelf_button_) {
    SetPinnedToShelf(false);
#endif
  } else if (sender == remove_button_) {
    UninstallApp();
  } else {
    NOTREACHED();
  }
}

void AppInfoFooterPanel::OnExtensionUninstallDialogClosed(
    bool did_start_uninstall,
    const base::string16& error) {
  if (did_start_uninstall) {
    // Close the App Info dialog as well (which will free the dialog too).
    Close();
  } else {
    extension_uninstall_dialog_.reset();
  }
}

void AppInfoFooterPanel::CreateShortcuts() {
  DCHECK(CanCreateShortcuts());
  chrome::ShowCreateChromeAppShortcutsDialog(GetWidget()->GetNativeWindow(),
                                             profile_,
                                             app_,
                                             base::Callback<void(bool)>());
}

bool AppInfoFooterPanel::CanCreateShortcuts() const {
#if defined(OS_CHROMEOS)
  // Ash platforms can't create shortcuts.
  return false;
#else
  // Extensions and the Chrome component app can't have shortcuts.
  return app_->id() != extension_misc::kChromeAppId && !app_->is_extension();
#endif  // OS_CHROMEOS
}

#if defined(OS_CHROMEOS)
void AppInfoFooterPanel::SetPinnedToShelf(bool value) {
  DCHECK(CanSetPinnedToShelf());
  ash::ShelfModel* shelf_model =
      ChromeLauncherController::instance()->shelf_model();
  DCHECK(shelf_model);
  if (value)
    shelf_model->PinAppWithID(app_->id());
  else
    shelf_model->UnpinAppWithID(app_->id());

  UpdatePinButtons(true);
  Layout();
}

bool AppInfoFooterPanel::CanSetPinnedToShelf() const {
  // The Chrome app can't be unpinned, and extensions can't be pinned.
  return app_->id() != extension_misc::kChromeAppId && !app_->is_extension() &&
         (GetPinnableForAppID(app_->id(), profile_) ==
          AppListControllerDelegate::PIN_EDITABLE);
}
#endif  // OS_CHROMEOS

void AppInfoFooterPanel::UninstallApp() {
  DCHECK(CanUninstallApp());
  extension_uninstall_dialog_.reset(
      extensions::ExtensionUninstallDialog::Create(
          profile_, GetWidget()->GetNativeWindow(), this));
  extension_uninstall_dialog_->ConfirmUninstall(
      app_, extensions::UNINSTALL_REASON_USER_INITIATED,
      extensions::UNINSTALL_SOURCE_APP_INFO_DIALOG);
}

bool AppInfoFooterPanel::CanUninstallApp() const {
  extensions::ManagementPolicy* policy =
      extensions::ExtensionSystem::Get(profile_)->management_policy();
  return policy->UserMayModifySettings(app_, nullptr) &&
         !policy->MustRemainInstalled(app_, nullptr);
}
