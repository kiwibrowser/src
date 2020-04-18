// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_action_icon_factory.h"

#include "chrome/browser/extensions/extension_action.h"
#include "chrome/browser/profiles/profile.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"

using extensions::Extension;
using extensions::IconImage;

ExtensionActionIconFactory::ExtensionActionIconFactory(
    Profile* profile,
    const Extension* extension,
    ExtensionAction* action,
    Observer* observer)
    : action_(action), observer_(observer), icon_image_observer_(this) {
  if (action->default_icon_image())
    icon_image_observer_.Add(action->default_icon_image());
}

ExtensionActionIconFactory::~ExtensionActionIconFactory() {}

// extensions::IconImage::Observer overrides.
void ExtensionActionIconFactory::OnExtensionIconImageChanged(IconImage* image) {
  if (observer_)
    observer_->OnIconUpdated();
}

void ExtensionActionIconFactory::OnExtensionIconImageDestroyed(
    IconImage* image) {
  icon_image_observer_.RemoveAll();
}

gfx::Image ExtensionActionIconFactory::GetIcon(int tab_id) {
  gfx::Image icon = action_->GetExplicitlySetIcon(tab_id);
  if (!icon.IsEmpty())
    return icon;

  icon = action_->GetDeclarativeIcon(tab_id);
  if (!icon.IsEmpty())
    return icon;

  return action_->GetDefaultIconImage();
}
