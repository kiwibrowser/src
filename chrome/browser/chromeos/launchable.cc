// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/launchable.h"

#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"

namespace chromeos {

Launchable::Launchable() {}
Launchable::~Launchable() {}

void Launchable::Bind(mash::mojom::LaunchableRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void Launchable::Launch(uint32_t what, mash::mojom::LaunchMode how) {
  bool is_incognito;
  switch (what) {
    case mash::mojom::kWindow:
      is_incognito = false;
      break;
    case mash::mojom::kIncognitoWindow:
      is_incognito = true;
      break;
    default:
      NOTREACHED();
  }

  bool reuse = how != mash::mojom::LaunchMode::MAKE_NEW;
  if (reuse) {
    Profile* profile = ProfileManager::GetActiveUserProfile();
    Browser* browser = chrome::FindTabbedBrowser(
        is_incognito ? profile->GetOffTheRecordProfile() : profile, false);
    if (browser) {
      browser->window()->Show();
      return;
    }
  }

  CreateNewWindowImpl(is_incognito);
}

void Launchable::CreateNewWindowImpl(bool is_incognito) {
  Profile* profile = ProfileManager::GetActiveUserProfile();
  chrome::NewEmptyWindow(is_incognito ? profile->GetOffTheRecordProfile()
                                      : profile);
}

}  // namespace chromeos
