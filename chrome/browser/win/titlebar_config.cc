// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/win/titlebar_config.h"

#include "base/command_line.h"
#include "base/win/windows_version.h"
#include "chrome/common/chrome_switches.h"
#include "ui/native_theme/native_theme_win.h"

// Enables custom-drawing the titlebar and tabstrip background so that we can
// paint it various gray colors instead of the default #FFFFFF.
const base::Feature kWindows10CustomTitlebar{"Windows10CustomTitlebar",
                                             base::FEATURE_ENABLED_BY_DEFAULT};

bool ShouldCustomDrawSystemTitlebar() {
  // TODO(bsep): We would like the custom-draw the titlebar in high-contrast
  // mode so that we can correctly draw the caption buttons on the left in RTL
  // mode. But they require a different style and color selection that isn't
  // currently implemented.
  return !ui::NativeTheme::GetInstanceForNativeUi()->UsesHighContrastColors() &&
         base::FeatureList::IsEnabled(kWindows10CustomTitlebar) &&
         base::win::GetVersion() >= base::win::VERSION_WIN10;
}
