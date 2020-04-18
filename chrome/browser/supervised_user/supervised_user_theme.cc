// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/supervised_user/supervised_user_theme.h"

#include "chrome/browser/themes/theme_properties.h"
#include "chrome/grit/theme_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image.h"

namespace {

const SkColor kDefaultColorFrameSupervisedUser = SkColorSetRGB(165, 197, 225);
const SkColor kDefaultColorFrameSupervisedUserInactive =
    SkColorSetRGB(180, 225, 247);
const SkColor kDefaultColorSupervisedUserLabelBackground =
    SkColorSetRGB(108, 167, 210);

// Map resource ids to the supervised user resource ids.
int MapToSupervisedUserResourceIds(int id) {
  switch (id) {
    case IDR_THEME_FRAME:
      return IDR_SUPERVISED_USER_THEME_FRAME;
    case IDR_THEME_FRAME_INACTIVE:
      return IDR_SUPERVISED_USER_THEME_FRAME_INACTIVE;
    case IDR_THEME_TAB_BACKGROUND:
    case IDR_THEME_TAB_BACKGROUND_V:
      return IDR_SUPERVISED_USER_THEME_TAB_BACKGROUND;
  }
  return id;
}

}  // namespace

SupervisedUserTheme::SupervisedUserTheme()
    : CustomThemeSupplier(SUPERVISED_USER_THEME) {}

SupervisedUserTheme::~SupervisedUserTheme() {}

bool SupervisedUserTheme::GetColor(int id, SkColor* color) const {
  switch (id) {
    case ThemeProperties::COLOR_FRAME:
      *color = kDefaultColorFrameSupervisedUser;
      return true;
    case ThemeProperties::COLOR_FRAME_INACTIVE:
      *color = kDefaultColorFrameSupervisedUserInactive;
      return true;
    case ThemeProperties::COLOR_SUPERVISED_USER_LABEL:
      *color = SK_ColorWHITE;
      return true;
    case ThemeProperties::COLOR_SUPERVISED_USER_LABEL_BACKGROUND:
      *color = kDefaultColorSupervisedUserLabelBackground;
      return true;
  }
  return false;
}

gfx::Image SupervisedUserTheme::GetImageNamed(int id) {
  if (!HasCustomImage(id))
    return gfx::Image();

  id = MapToSupervisedUserResourceIds(id);
  return ui::ResourceBundle::GetSharedInstance().GetNativeImageNamed(id);
}

bool SupervisedUserTheme::HasCustomImage(int id) const {
  return id != MapToSupervisedUserResourceIds(id);
}
