// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/feature_list.h"
#include "chrome/browser/android/shortcut_info.h"

ShortcutInfo::ShortcutInfo(const GURL& shortcut_url)
    : url(shortcut_url),
      display(blink::kWebDisplayModeBrowser),
      orientation(blink::kWebScreenOrientationLockDefault),
      source(SOURCE_ADD_TO_HOMESCREEN_SHORTCUT),
      ideal_splash_image_size_in_px(0),
      minimum_splash_image_size_in_px(0) {}

ShortcutInfo::ShortcutInfo(const ShortcutInfo& other) = default;

ShortcutInfo::~ShortcutInfo() {
}

void ShortcutInfo::UpdateFromManifest(const blink::Manifest& manifest) {
  if (!manifest.short_name.string().empty() ||
      !manifest.name.string().empty()) {
    short_name = manifest.short_name.string();
    name = manifest.name.string();
    if (short_name.empty())
      short_name = name;
    else if (name.empty())
      name = short_name;
  }
  user_title = short_name;

  // Set the url based on the manifest value, if any.
  if (manifest.start_url.is_valid())
    url = manifest.start_url;

  if (manifest.scope.is_valid())
    scope = manifest.scope;

  // Set the display based on the manifest value, if any.
  if (manifest.display != blink::kWebDisplayModeUndefined)
    display = manifest.display;

  if (display == blink::kWebDisplayModeStandalone ||
      display == blink::kWebDisplayModeFullscreen ||
      display == blink::kWebDisplayModeMinimalUi) {
    source = SOURCE_ADD_TO_HOMESCREEN_STANDALONE;
    // Set the orientation based on the manifest value, or ignore if the display
    // mode is different from 'standalone', 'fullscreen' or 'minimal-ui'.
    if (manifest.orientation != blink::kWebScreenOrientationLockDefault) {
      // TODO(mlamouri): Send a message to the developer console if we ignored
      // Manifest orientation because display property is not set.
      orientation = manifest.orientation;
    }
  }

  // Set the theme color based on the manifest value, if any.
  if (manifest.theme_color)
    theme_color = manifest.theme_color;

  // Set the background color based on the manifest value, if any.
  if (manifest.background_color)
    background_color = manifest.background_color;

  // Sets the URL of the HTML splash screen, if any.
  if (manifest.splash_screen_url.is_valid())
    splash_screen_url = manifest.splash_screen_url;

  // Set the icon urls based on the icons in the manifest, if any.
  icon_urls.clear();
  for (const blink::Manifest::Icon& icon : manifest.icons)
    icon_urls.push_back(icon.src.spec());

  if (manifest.share_target)
    share_target_url_template = manifest.share_target->url_template;
}

void ShortcutInfo::UpdateSource(const Source new_source) {
  source = new_source;
}
