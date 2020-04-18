// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/startup/default_browser_prompt.h"

#include "base/win/windows_version.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/ui/webui/set_as_default_browser_ui_win.h"
#include "components/startup_metric_utils/browser/startup_metric_utils.h"

bool ShowFirstRunDefaultBrowserPrompt(Profile* profile) {
  // The behavior on Windows 10 is no good at the moment, since there is no
  // known way to lead the user directly to a default browser picker.
  if (base::win::GetVersion() >= base::win::VERSION_WIN10)
    return false;

  // If the only available mode of setting the default browser requires
  // user interaction, it means this couldn't have been done yet. Therefore,
  // we launch the dialog and inform the caller of it. Only take this step if
  // neither this browser nor any side-by-side install is default.
  bool show_status = (shell_integration::GetDefaultWebClientSetPermission() ==
                      shell_integration::SET_DEFAULT_INTERACTIVE) &&
                     (shell_integration::GetDefaultBrowser() ==
                      shell_integration::NOT_DEFAULT);

  if (show_status) {
    startup_metric_utils::SetNonBrowserUIDisplayed();
    SetAsDefaultBrowserUI::Show(profile);
  }

  return show_status;
}
