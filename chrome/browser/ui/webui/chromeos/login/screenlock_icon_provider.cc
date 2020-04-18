// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/screenlock_icon_provider.h"

namespace chromeos {

ScreenlockIconProvider::ScreenlockIconProvider() {}

ScreenlockIconProvider::~ScreenlockIconProvider() {}

void ScreenlockIconProvider::AddIcon(const std::string& username,
                                     const gfx::Image& icon) {
  user_icon_map_[username] = icon;
}

void ScreenlockIconProvider::RemoveIcon(const std::string& username) {
  if (user_icon_map_.find(username) != user_icon_map_.end())
    user_icon_map_.erase(username);
}

gfx::Image ScreenlockIconProvider::GetIcon(const std::string& username) {
  if (user_icon_map_.find(username) == user_icon_map_.end())
    return gfx::Image();
  return user_icon_map_[username];
}

void ScreenlockIconProvider::Clear() {
  user_icon_map_.clear();
}

}  // namespace chromeos
