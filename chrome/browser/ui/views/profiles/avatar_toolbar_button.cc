// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/profiles/avatar_toolbar_button.h"

#include "chrome/app/chrome_command_ids.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/avatar_menu.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/grit/generated_resources.h"
#include "components/signin/core/browser/signin_manager.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/models/menu_model.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/button/label_button_border.h"

AvatarToolbarButton::AvatarToolbarButton(Profile* profile,
                                         views::ButtonListener* listener)
    : ToolbarButton(profile, listener, nullptr),
      profile_(profile),
      error_controller_(this, profile_),
      profile_observer_(this) {
  profile_observer_.Add(
      &g_browser_process->profile_manager()->GetProfileAttributesStorage());

  SetImageAlignment(HorizontalAlignment::ALIGN_CENTER,
                    VerticalAlignment::ALIGN_MIDDLE);

  // In non-touch mode we use a larger-than-normal icon size for avatars as 16dp
  // is hard to read for user avatars. This constant is correspondingly smaller
  // than GetLayoutInsets(TOOLBAR_BUTTON).
  if (!ui::MaterialDesignController::IsTouchOptimizedUiEnabled())
    SetBorder(views::CreateEmptyBorder(gfx::Insets(4)));

  set_triggerable_event_flags(ui::EF_LEFT_MOUSE_BUTTON |
                              ui::EF_MIDDLE_MOUSE_BUTTON);
  set_tag(IDC_SHOW_AVATAR_MENU);
  set_id(VIEW_ID_AVATAR_BUTTON);

  Init();

  // The profile switcher is only available outside incognito.
  SetEnabled(!IsIncognito());

  // Set initial tooltip. UpdateIcon() needs to be called from the outside as
  // GetThemeProvider() is not available until the button is added to
  // ToolbarView's hierarchy.
  UpdateTooltipText();
}

AvatarToolbarButton::~AvatarToolbarButton() = default;

void AvatarToolbarButton::UpdateIcon() {
  SetImage(views::Button::STATE_NORMAL, GetAvatarIcon());
}

void AvatarToolbarButton::UpdateTooltipText() {
  if (IsIncognito()) {
    SetTooltipText(
        l10n_util::GetStringUTF16(IDS_INCOGNITO_AVATAR_BUTTON_TOOLTIP));
  } else {
    SetTooltipText(profiles::GetAvatarNameForProfile(profile_->GetPath()));
  }
}

void AvatarToolbarButton::OnAvatarErrorChanged() {
  UpdateIcon();
}

void AvatarToolbarButton::OnProfileAdded(const base::FilePath& profile_path) {
  // Adding any profile changes the profile count, we might go from showing a
  // generic avatar button to profile pictures here. Update icon accordingly.
  UpdateIcon();
}

void AvatarToolbarButton::OnProfileWasRemoved(
    const base::FilePath& profile_path,
    const base::string16& profile_name) {
  // Removing a profile changes the profile count, we might go from showing
  // per-profile icons back to a generic avatar icon. Update icon accordingly.
  UpdateIcon();
}

void AvatarToolbarButton::OnProfileAvatarChanged(
    const base::FilePath& profile_path) {
  UpdateIcon();
}

void AvatarToolbarButton::OnProfileHighResAvatarLoaded(
    const base::FilePath& profile_path) {
  UpdateIcon();
}

void AvatarToolbarButton::OnProfileNameChanged(
    const base::FilePath& profile_path,
    const base::string16& old_profile_name) {
  UpdateTooltipText();
}

bool AvatarToolbarButton::IsIncognito() const {
  return profile_->IsOffTheRecord() && !profile_->IsGuestSession();
}

bool AvatarToolbarButton::ShouldShowGenericIcon() const {
  return g_browser_process->profile_manager()
                 ->GetProfileAttributesStorage()
                 .GetNumberOfProfiles() == 1 &&
         !SigninManagerFactory::GetForProfile(profile_)->IsAuthenticated();
}

gfx::ImageSkia AvatarToolbarButton::GetAvatarIcon() const {
  const int icon_size =
      ui::MaterialDesignController::IsTouchOptimizedUiEnabled() ? 24 : 20;

  const SkColor icon_color =
      GetThemeProvider()->GetColor(ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON);

  if (IsIncognito())
    return gfx::CreateVectorIcon(kIncognitoIcon, icon_size, icon_color);

  if (profile_->IsGuestSession())
    return gfx::CreateVectorIcon(kUserMenuGuestIcon, icon_size, icon_color);

  gfx::Image avatar_icon;
  if (!ShouldShowGenericIcon())
    avatar_icon = GetIconImageFromProfile();
  if (!avatar_icon.IsEmpty()) {
    return profiles::GetSizedAvatarIcon(avatar_icon, true, icon_size, icon_size,
                                        profiles::SHAPE_CIRCLE)
        .AsImageSkia();
  }

  return gfx::CreateVectorIcon(kUserAccountAvatarIcon, icon_size, icon_color);
}

gfx::Image AvatarToolbarButton::GetIconImageFromProfile() const {
  ProfileAttributesEntry* entry;
  if (!g_browser_process->profile_manager()
           ->GetProfileAttributesStorage()
           .GetProfileAttributesWithPath(profile_->GetPath(), &entry)) {
    // This can happen if the user deletes the current profile.
    return gfx::Image();
  }

  // If there is a GAIA image available, try to use that.
  if (entry->IsUsingGAIAPicture()) {
    // TODO(chengx): The GetGAIAPicture API call will trigger an async image
    // load from disk if it has not been loaded. This is non-obvious and
    // dependency should be avoided. We should come with a better idea to handle
    // this.
    const gfx::Image* gaia_image = entry->GetGAIAPicture();

    if (gaia_image)
      return *gaia_image;
    return gfx::Image();
  }

  return entry->GetAvatarIcon();
}
