// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/session_util.h"

#include "ash/content/shell_content_state.h"
#include "build/build_config.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/grit/theme_resources.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/browser_context.h"
#include "ui/aura/window.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image_skia_operations.h"

content::BrowserContext* GetActiveBrowserContext() {
  DCHECK(user_manager::UserManager::Get()->GetLoggedInUsers().size());
  return ProfileManager::GetActiveUserProfile();
}

bool CanShowWindowForUser(
    aura::Window* window,
    const GetActiveBrowserContextCallback& get_context_callback) {
  DCHECK(window);
  if (user_manager::UserManager::Get()->GetLoggedInUsers().size() > 1u) {
    content::BrowserContext* active_browser_context =
        get_context_callback.Run();
    ash::ShellContentState* state = ash::ShellContentState::GetInstance();
    content::BrowserContext* owner_browser_context =
        state->GetBrowserContextForWindow(window);
    content::BrowserContext* shown_browser_context =
        state->GetUserPresentingBrowserContextForWindow(window);

    if (owner_browser_context && active_browser_context &&
        owner_browser_context != active_browser_context &&
        shown_browser_context != active_browser_context) {
      return false;
    }
  }
  return true;
}

gfx::ImageSkia GetAvatarImageForContext(content::BrowserContext* context) {
  return GetAvatarImageForUser(chromeos::ProfileHelper::Get()->GetUserByProfile(
      Profile::FromBrowserContext(context)));
}

gfx::ImageSkia GetAvatarImageForUser(const user_manager::User* user) {
  static const gfx::ImageSkia* holder =
      ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
          IDR_AVATAR_HOLDER);
  static const gfx::ImageSkia* holder_mask =
      ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
          IDR_AVATAR_HOLDER_MASK);

  gfx::ImageSkia user_image = user->GetImage();
  gfx::ImageSkia resized = gfx::ImageSkiaOperations::CreateResizedImage(
      user_image, skia::ImageOperations::RESIZE_BEST, holder->size());
  gfx::ImageSkia masked =
      gfx::ImageSkiaOperations::CreateMaskedImage(resized, *holder_mask);
  return gfx::ImageSkiaOperations::CreateSuperimposedImage(*holder, masked);
}
