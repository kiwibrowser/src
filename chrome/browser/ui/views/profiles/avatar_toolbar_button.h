// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_PROFILES_AVATAR_TOOLBAR_BUTTON_H_
#define CHROME_BROWSER_UI_VIEWS_PROFILES_AVATAR_TOOLBAR_BUTTON_H_

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/ui/avatar_button_error_controller.h"
#include "chrome/browser/ui/avatar_button_error_controller_delegate.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"

class AvatarToolbarButton : public ToolbarButton,
                            public AvatarButtonErrorControllerDelegate,
                            public ProfileAttributesStorage::Observer {
 public:
  AvatarToolbarButton(Profile* profile, views::ButtonListener* listener);
  ~AvatarToolbarButton() override;

  void UpdateIcon();
  void UpdateTooltipText();

 private:
  // AvatarButtonErrorControllerDelegate:
  void OnAvatarErrorChanged() override;

  // ProfileAttributesStorage::Observer:
  void OnProfileAdded(const base::FilePath& profile_path) override;
  void OnProfileWasRemoved(const base::FilePath& profile_path,
                           const base::string16& profile_name) override;
  void OnProfileAvatarChanged(const base::FilePath& profile_path) override;
  void OnProfileHighResAvatarLoaded(
      const base::FilePath& profile_path) override;
  void OnProfileNameChanged(const base::FilePath& profile_path,
                            const base::string16& old_profile_name) override;

  bool IsIncognito() const;
  bool ShouldShowGenericIcon() const;
  gfx::ImageSkia GetAvatarIcon() const;
  gfx::Image GetIconImageFromProfile() const;

  Profile* const profile_;

  AvatarButtonErrorController error_controller_;
  ScopedObserver<ProfileAttributesStorage, AvatarToolbarButton>
      profile_observer_;

  DISALLOW_COPY_AND_ASSIGN(AvatarToolbarButton);
};

#endif  // CHROME_BROWSER_UI_VIEWS_PROFILES_AVATAR_TOOLBAR_BUTTON_H_
